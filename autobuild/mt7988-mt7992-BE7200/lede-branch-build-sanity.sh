#!/bin/bash
source ./autobuild/lede-build-sanity.sh
source ./autobuild/filogic/prepare.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
cfg80211_on=1
kasan=0
twopcie=0
sku_type=0
idx_log=0
CASAN=0
wifi_rss=0
args=

# Copy Jaguar+Eagle's package, target and tools temporarily
MT7990_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7990-BE19000
MT7992_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7992-BE7200

cp -fpRn ${MT7990_BDIR}/package ${MT7992_BDIR}
cp -fpRn ${MT7990_BDIR}/target ${MT7992_BDIR}
cp -fpRn ${MT7990_BDIR}/tools ${MT7992_BDIR}
# End of Copy Jaguar+Eagle's package, target and tools temporarily

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

# Remove cfg80211_off from input parameters
for arg in $*; do
	case "$arg" in
	"cfg80211_off")
		cfg80211_on=0
		;;
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
	"CASAN")
		CASAN=1
		;;
	"rss")
		wifi_rss=1
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


sed -i '/^[\t ]*\[[ ]*![ ]*-f[ ]*\/etc\/config\/wireless[ ]*\]/a \\t\t\/sbin\/wifi detect' ${BUILD_DIR}/package/base-files/files/etc/init.d/boot

change_config_before_defconfig() {
	[ "$wifi_rss" = "1" ] && {
		echo "Build RSS w/o wed"
		sed -i 's/CONFIG_PACKAGE_kmod-warp=y/# CONFIG_PACKAGE_kmod-warp is not set/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_RRO_MODE=4/CONFIG_MTK_HWIFI_MT7992_RRO_MODE=5/g' ${BUILD_DIR}/.config
		[ "$twopcie" = "0" ] && {
			echo "CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-1pcie-spim-nand=y" >> ${BUILD_DIR}/.config
		}
		[ "$twopcie" = "1" ] && {
			echo "CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-2pcie-spim-nand=y" >> ${BUILD_DIR}/.config
		}
	}

	generate_sb_targets ${branch_name}
}

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

	[ "$twopcie" = "0" ] && {
		echo "1pcie"
		sed -i 's/CONFIG_MTK_HWIFI_MT799A=y/CONFIG_MTK_HWIFI_MT799A=n/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_PACKAGE_kmod-mt799a=y/# CONFIG_PACKAGE_kmod-mt799a is not set/g' ${BUILD_DIR}/.config
		[ "$wifi_rss" = "1" ] && {
			echo "Configure RSS with 1-PCIE"
			sed -i 's/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=0/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=3/g' ${BUILD_DIR}/.config
			sed -i 's/CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-1pcie-spim-nand=""/CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-1pcie-spim-nand="nand-utils"/g' ${BUILD_DIR}/.config
		}
	}

	[ "$twopcie" = "1" ] && {
		echo "2pcie"
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=0/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=1/g' ${BUILD_DIR}/.config

		if [[ $wifi_rss -eq 0 ]]; then
			sed -i 's/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=0/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=4/g' ${BUILD_DIR}/.config
		else
			echo "Configure RSS with 2-PCIE"
			sed -i 's/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=0/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=7/g' ${BUILD_DIR}/.config
			sed -i 's/CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-2pcie-spim-nand=""/CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-msi-2pcie-spim-nand="nand-utils"/g' ${BUILD_DIR}/.config
		fi
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

	[ "$CASAN" = "1" ] && {
	echo "For CASAN FW image"
    sed -i 's/# CONFIG_MTK_WIFI7_FW_CASAN_SUPPORT is not set/CONFIG_MTK_WIFI7_FW_CASAN_SUPPORT=y/g' ${BUILD_DIR}/.config
    sed -i 's/# CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand-CASAN is not set/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand-CASAN=y/g' ${BUILD_DIR}/.config
    sed -i '/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand-CASAN=y/a CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand-CASAN="nand-utils"' ${BUILD_DIR}/.config
    sed -i 's/# CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand-CASAN is not set/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand-CASAN=y/g' ${BUILD_DIR}/.config
    sed -i '/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand-CASAN=y/a CONFIG_TARGET_DEVICE_PACKAGES_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand-CASAN="nand-utils"' ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-88d-10g-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-88d-10g-spim-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988a-dsa-10g-spim-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-emmc=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-emmc=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-sd=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-sd=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-snfi-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-snfi-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nor=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-10g-spim-nor=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-e2p5g-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-dsa-e2p5g-spim-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-gsw-10g-sfp-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-gsw-10g-sfp-spim-nand=y/g'  ${BUILD_DIR}/.config
    sed -i 's/CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-gsw-10g-spim-nand=y/#CONFIG_TARGET_DEVICE_mediatek_mt7988_DEVICE_mediatek_mt7988d-gsw-10g-spim-nand=y/g'  ${BUILD_DIR}/.config
	}
}

#do prepare stuff
prepare

sed -i 's/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE is not set/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE=y/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
sed -i 's/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG=y/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_security ${branch_name}

prepare_final ${branch_name}


#step2 build
build ${branch_name} -pb || [ "$LOCAL" != "1" ]
