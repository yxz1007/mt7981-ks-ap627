#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

#handle release & releease build
if [ -n ${1} ]; then
	if [ "${1}" = "release" ] || [ "${1}" = "release_build" ]; then
		rel_conf=${BUILD_DIR}/../tools/release_conf/${branch_name}/release.conf
		package_conf=${BUILD_DIR}/../tools/release_conf/${branch_name}/package.conf

		if [ ! -f ${rel_conf} ] || [ ! -f ${package_conf} ]; then
			echo "no release or pakcage config. release terminated"
		else
			source ${rel_conf}
			source ${package_conf}
			source ${BUILD_DIR}/../tools/release.sh
		fi
		exit 0;
	fi
fi

for arg in $*; do
	case "$arg" in
	"dbg")
		dbg_en=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

#install patches from mt7988-mt7990-BE19000
#cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/target/ ${BUILD_DIR}/autobuild/${branch_name}/
#cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/package/ ${BUILD_DIR}/autobuild/${branch_name}/
#cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/tools/ ${BUILD_DIR}/autobuild/${branch_name}/
#cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/.config ${BUILD_DIR}/autobuild/${branch_name}/

#Check and merge all config files as final .config

file_def_config=./autobuild/${branch_name}/.config
file_custom_config=./autobuild/${branch_name}/.custom.config
file_ori_config=./autobuild/${branch_name}/.old.config

if [ -f ${file_ori_config} ]; then
	echo "$file_ori_config exist!"
	rm -rf ${file_def_config}
	mv ${file_ori_config} ${file_def_config}
fi

if [ -f ${file_custom_config} ]; then
	echo "$file_custom_config exist!"
	cp -rf ${file_def_config} ${file_ori_config}
	if [ ${file_custom_config} != ${file_def_config} ]; then
		cat ${file_custom_config} >> ${file_def_config}
	fi
fi

[ "$dbg_en" = "1" ] && {
	file_dbg_config=./autobuild/${branch_name}/.dbg.config
	if [ -f ${file_dbg_config} ]; then
		echo "$file_dbg_config exist!"
		cat ${file_dbg_config} >> ${file_def_config}
	fi
}

# cfg80211_on patch code flow
	rm -rf ${BUILD_DIR}/package/network/services/hostapd
	rm -rf ${BUILD_DIR}/package/kernel/mac80211

	rm -rf ${BUILD_DIR}/package/libs/libnl-tiny
	cp -fpR ${BUILD_DIR}/./../mac80211_package/package/libs/libnl-tiny ${BUILD_DIR}/package/libs

	rm -rf ${BUILD_DIR}/package/network/utils/iw
	cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iw ${BUILD_DIR}/package/network/utils

	rm -rf ${BUILD_DIR}/package/network/utils/iwinfo
	cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iwinfo ${BUILD_DIR}/package/network/utils

	sed -i 's/$(LN) iwconfig $(1)\/usr\/sbin\/iwpriv/$(LN) mwctl $(1)\/usr\/sbin\/iwpriv/g' ${BUILD_DIR}/package/network/utils/wireless-tools/Makefile
	#use hostapd master package revision, remove hostapd 2102 patches
	find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete
	find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-hostapd-*.patch" -delete

	#use mt7986_dev2 branch, remove mt76 master patches
	find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-mt76-*.patch" -delete

sed -i '/^[\t ]*\[[ ]*![ ]*-f[ ]*\/etc\/config\/wireless[ ]*\]/a \\t\t\/sbin\/wifi detect' ${BUILD_DIR}/package/base-files/files/etc/init.d/boot

#step1 clean
#clean
#do prepare stuff
prepare

# remove tops master patches and package
rm ${BUILD_DIR}/target/linux/mediatek/patches-5.4/999-40*.patch
rm ${BUILD_DIR}/target/linux/mediatek/patches-5.4/999-41*.patch
rm ${BUILD_DIR}/target/linux/mediatek/patches-5.4/999-45*.patch
rm -rf ${BUILD_DIR}/package/kernel/tops/
rm -rf ${BUILD_DIR}/package/kernel/pce/
rm -rf ${BUILD_DIR}/package/mtk_soc/drivers/crypto-eip/
rm -rf ${BUILD_DIR}/package/feeds/mtk_openwrt_feed/tops-tool/

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

patch -f -p1 -d ${BUILD_DIR}/feeds/luci -i ${BUILD_DIR}/autobuild/${branch_name}/0100_disable_web_rollback.patch

#step2 build
build ${branch_name} || [ "$LOCAL" != "1" ]
