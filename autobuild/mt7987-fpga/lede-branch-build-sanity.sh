#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

# replace original do_patch with quilt do_patch 
do_patch() {
	quilt init
	# quilt import is a stack, we should reverse the order here
	files=`find $1 -name "*.patch" | sort | tac`
	for file in $files
	do
		quilt import "${file}"
	done
	echo "--- list openwrt patches ---"
	quilt series
	echo "quilt push -a"
	quilt push -a || exit 1
}

# force remove secure boot and firmware encryption patches to resolve patch confilct
rm  ${BUILD_DIR}/autobuild/openwrt_patches-21.02/mtk_soc/09*.patch
# copy dt-overlay backport patches to openwrt_patches-21.02
cp -fpR ${BUILD_DIR}/autobuild/mt7987-fpga/openwrt_patches-21.02/mtk_soc/ ${BUILD_DIR}/autobuild/openwrt_patches-21.02

prepare

prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

build ${branch_name} -j1 || [ "$LOCAL" != "1" ]
