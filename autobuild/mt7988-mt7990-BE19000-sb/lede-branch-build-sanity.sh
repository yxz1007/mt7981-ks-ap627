#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
cfg80211_on=1
kasan=0
wifi7_wfa=0
map=0
args=

#handle release & releease build
if [ -n ${1} ]; then
	if [ "${1}" = "release" ] || [ "${1}" = "release_build" ]; then
		rel_conf=${BUILD_DIR}/../tools/release_conf/${branch_name}/release.conf

		case "${2}" in
		"map")
			package_conf=${BUILD_DIR}/../tools/release_conf/${branch_name}/package_map.conf
			;;
		*)
			package_conf=${BUILD_DIR}/../tools/release_conf/${branch_name}/package.conf
			;;
		esac

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

# Remove cfg80211_off from input parameters
for arg in $*; do
	case "$arg" in
	"cfg80211_off")
		cfg80211_on=0
		;;
	"kasan")
		kasan=1
		;;
	"wifi7_wfa")
		wifi7_wfa=1
		;;
	"map")
		map=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

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

[ "$cfg80211_on" = "1" ] && {
	sed -i 's/$(LN) iwconfig $(1)\/usr\/sbin\/iwpriv/$(LN) mwctl $(1)\/usr\/sbin\/iwpriv/g' ${BUILD_DIR}/package/network/utils/wireless-tools/Makefile
}

[ "$wifi7_wfa" = "1" ] && {
	echo ".config compile with additional files"
	patch -f -p1 -i ./autobuild/mt7988-mt7990-BE19000/set_plufest_ip.patch
	sed -i '/MloV1=/d' ${BUILD_DIR}/package/mtk/drivers/wifi-profile/files/mt7990/mt7990.1.dat
}

if [ "$1" = "1pcie" ]; then
	echo "1pcie"
	sed -i 's/CONFIG_MTK_HWIFI_MT7990_OPTION_TYPE=1/CONFIG_MTK_HWIFI_MT7990_OPTION_TYPE=0/g' autobuild/${branch_name}/.config
fi

sed -i '/^[\t ]*\[[ ]*![ ]*-f[ ]*\/etc\/config\/wireless[ ]*\]/a \\t\t\/sbin\/wifi detect' ${BUILD_DIR}/package/base-files/files/etc/init.d/boot

change_config_after_defconfig() {
	[ "$cfg80211_on" = "1" ] || {
		sed -i 's/CONFIG_PACKAGE_mwctl=y/# CONFIG_PACKAGE_mwctl is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-cfg80211=y/# CONFIG_PACKAGE_kmod-cfg80211 is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt_cfg80211=y/# CONFIG_PACKAGE_kmod-mt_cfg80211 is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_WIFI7_CFG80211_SUPPORT=y/# CONFIG_MTK_WIFI7_CFG80211_SUPPORT is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_hostapd-common=y/# CONFIG_PACKAGE_hostapd-common is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_wpad-openssl=y/# CONFIG_PACKAGE_wpad-openssl is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_hostapd-utils=y/# CONFIG_PACKAGE_hostapd-utils is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_wpa-cli=y/# CONFIG_PACKAGE_wpa-cli is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_WPA_MSG_MIN_PRIORITY=0/# CONFIG_WPA_MSG_MIN_PRIORITY is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_WIFI7_HOSTAPD_MBSS_SUPPORT=y/# CONFIG_MTK_WIFI7_HOSTAPD_MBSS_SUPPORT is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_WIFI7_APCLI_SUPPLICANT_SUPPORT=y/# CONFIG_MTK_WIFI7_APCLI_SUPPLICANT_SUPPORT is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_libwpactrl=y/# CONFIG_PACKAGE_libwpactrl is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_WAPP_WPACTRL_SUPPORT=y/# CONFIG_WAPP_WPACTRL_SUPPORT is not set/g' ${BUILD_DIR}/.config
	}

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

	[ "$wifi7_wfa" = "1" ] && {
		sed -i '/CONFIG_DRIVER_WEXT=$(CONFIG_DRIVER_WEXT_SUPPORT) /a\\tCONFIG_TESTING_OPTIONS=y \\' ${BUILD_DIR}/package/network/services/hostapd/Makefile
	}

	[ "$map" = "1" ] && {
		sed -i 's/# CONFIG_PACKAGE_1905daemon is not set/CONFIG_PACKAGE_1905daemon=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_PACKAGE_apcli_detectd is not set/CONFIG_PACKAGE_apcli_detectd=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_PACKAGE_mapd is not set/CONFIG_PACKAGE_mapd=y/g' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MAP_R2_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MAP_R3_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MAP_R4_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MAP_R5_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_PACKAGE_mapd=y/a CONFIG_MAP_R5_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_PACKAGE_mapd=y/a CONFIG_MAP_R4_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_PACKAGE_mapd=y/a CONFIG_MAP_R3_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_PACKAGE_mapd=y/a CONFIG_MAP_R2_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R2_VER_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R3_VER_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R4_VER_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R5_VER_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R2_6E_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_R3_6E_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_HOSTAPD_SUPPORT/d' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_HOSTAPD_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R3_6E_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R2_6E_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R5_VER_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R4_VER_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R3_VER_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i '/CONFIG_MTK_WIFI7_MAP_SUPPORT=y/a CONFIG_MTK_WIFI7_MAP_R2_VER_SUPPORT=y' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_PACKAGE_miniupnpd is not set/CONFIG_PACKAGE_miniupnpd=y/g' ${BUILD_DIR}/.config
	}
}

#do prepare stuff
prepare

#cp autobuild(mt7988-mt7990-BE19000) customized file into SDK
cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/package/ ${BUILD_DIR}
#skip image subdirectory
rsync -av --exclude image ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/target/ ${BUILD_DIR}/target/
cp -fpR ${BUILD_DIR}/autobuild/mt7988-mt7990-BE19000/tools/ ${BUILD_DIR}

#cp autobuild(mt7986-AX6000-sb) customized file into SDK
cp -fpR ${BUILD_DIR}/autobuild/mt7986-AX6000-sb/target/ ${BUILD_DIR}
cp -fpR ${BUILD_DIR}/autobuild/mt7986-AX6000-sb/package/ ${BUILD_DIR}

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

patch -f -p1 -d ${BUILD_DIR}/feeds/luci -i ${BUILD_DIR}/autobuild/${branch_name}/0100_disable_web_rollback.patch

#step2 build
build ${branch_name} -pb || [ "$LOCAL" != "1" ]
