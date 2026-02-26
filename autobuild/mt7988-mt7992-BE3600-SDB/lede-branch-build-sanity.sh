#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
kasan=0
twopcie=0
sku_type=0
idx_log=0
args=

# Copy Jaguar+Eagle's package, target and tools temporarily
MT7990_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7990-BE19000
MT7992_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7992-BE7200

cp -fpR ${MT7990_BDIR}/package ${MT7992_BDIR}
cp -fpR ${MT7990_BDIR}/target ${MT7992_BDIR}
cp -fpR ${MT7990_BDIR}/tools ${MT7992_BDIR}

cp -fpR ${MT7992_BDIR}/target/ ${BUILD_DIR}/autobuild/${branch_name}/
cp -fpR ${MT7992_BDIR}/package/ ${BUILD_DIR}/autobuild/${branch_name}/
cp -fpR ${MT7992_BDIR}/tools/ ${BUILD_DIR}/autobuild/${branch_name}/
cp -fpR ${MT7992_BDIR}/.config ${BUILD_DIR}/autobuild/${branch_name}/


# End of Copy Jaguar+Eagle/BE7200 package, target and tools temporarily

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
	"kasan")
		kasan=1
		;;
	"2pcie")
		twopcie=1
		;;
	"6500")
		sku_type=1
		;;
	"5040")
		sku_type=2
		;;
	"idx_log")
		idx_log=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args


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

rm -rf ${BUILD_DIR}/package/network/services/hostapd
rm -rf ${BUILD_DIR}/package/kernel/mac80211

rm -rf ${BUILD_DIR}/package/libs/libnl-tiny
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/libs/libnl-tiny ${BUILD_DIR}/package/libs

rm -rf ${BUILD_DIR}/package/network/utils/iw
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iw ${BUILD_DIR}/package/network/utils

rm -rf ${BUILD_DIR}/package/network/utils/iwinfo
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iwinfo ${BUILD_DIR}/package/network/utils

#use hostapd master package revision, remove hostapd 2102 patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-hostapd-*.patch" -delete
#use mt7986_dev2 branch, remove mt76 master patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-mt76-*.patch" -delete

sed -i 's/$(LN) iwconfig $(1)\/usr\/sbin\/iwpriv/$(LN) mwctl $(1)\/usr\/sbin\/iwpriv/g' ${BUILD_DIR}/package/network/utils/wireless-tools/Makefile

sed -i '/^[\t ]*\[[ ]*![ ]*-f[ ]*\/etc\/config\/wireless[ ]*\]/a \\t\t\/sbin\/wifi detect' ${BUILD_DIR}/package/base-files/files/etc/init.d/boot

change_config_after_defconfig() {

	[ "$kasan" = "1" ] && {
		sed -i 's/# CONFIG_KERNEL_KASAN is not set/CONFIG_KERNEL_KASAN=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_KERNEL_KALLSYMS is not set/CONFIG_KERNEL_KALLSYMS=y/g' ${BUILD_DIR}/.config
		echo "CONFIG_KERNEL_KASAN_OUTLINE=y" >> ${BUILD_DIR}/.config
#		sed -i 's/CONFIG_PACKAGE_kmod-ufsd_driver=y/# CONFIG_PACKAGE_kmod-ufsd_driver is not set/g' ${BUILD_DIR}/.config
		echo "CONFIG_DEBUG_KMEMLEAK=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_AUTO_SCAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_DEBUG_KMEMLEAK_DEFAULT_OFF is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_MEM_POOL_SIZE=16000" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_DEBUG_KMEMLEAK_TEST=m" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KALLSYMS=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_GENERIC=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_KASAN_INLINE is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_OUTLINE=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_KASAN_SHADOW_OFFSET=0xdfffffd000000000" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "# CONFIG_TEST_KASAN is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_SLUB_DEBUG=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		echo "CONFIG_FRAME_WARN=4096" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
	}

	[ "$twopcie" = "0" ] && {
		echo "1pcie"
		sed -i 's/CONFIG_MTK_HWIFI_MT799A=y/CONFIG_MTK_HWIFI_MT799A=n/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt799a=y/# CONFIG_PACKAGE_kmod-mt799a is not set/g' ${BUILD_DIR}/.config
	}

	[ "$twopcie" = "1" ] && {
		echo "2pcie"
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=0/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=1/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=0/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=4/g' ${BUILD_DIR}/.config
	}

	[ "$sku_type" = "1" ] && {
		echo "SKU 6500"
		sed -i 's/CONFIG_MTK_WIFI7_SKU_TYPE="BE7200"/CONFIG_MTK_WIFI7_SKU_TYPE="BE6500"/g' ${BUILD_DIR}/.config
		sed -i 's/mt7992.1.dat/mt7992.6500.1.dat/g' ${BUILD_DIR}/.config
	}

	[ "$sku_type" = "2" ] && {
		echo "SKU 5040"
		sed -i 's/CONFIG_MTK_WIFI7_SKU_TYPE="BE7200"/CONFIG_MTK_WIFI7_SKU_TYPE="BE5040"/g' ${BUILD_DIR}/.config
		sed -i 's/mt7992.1.dat/mt7992.5040.1.dat/g' ${BUILD_DIR}/.config
	}

	[ "$idx_log" = "1" ] && {
		echo "Default FW idx log"
		sed -i 's/CONFIG_MTK_WIFI7_FW_LOG_TYPE="string_log"/CONFIG_MTK_WIFI7_FW_LOG_TYPE="idx_log"/g' ${BUILD_DIR}/.config
	}

}

#do prepare stuff
prepare

sed -i 's/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE is not set/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE=y/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
sed -i 's/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG=y/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

#step2 build
build ${branch_name} || [ "$LOCAL" != "1" ]
