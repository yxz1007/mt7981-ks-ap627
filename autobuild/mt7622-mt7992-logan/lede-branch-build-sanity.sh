#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
cfg80211_on=1
kasan=0
twopcie=1
sku_type=2
args=
#step1 clean
MT7990_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7990-BE19000
MT7992_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7992-BE7200
cp -fpR ${MT7990_BDIR}/package ${MT7992_BDIR}
cp -fpR ${MT7990_BDIR}/target ${MT7992_BDIR}
cp -fpR ${MT7990_BDIR}/tools ${MT7992_BDIR}
# End of Copy Jaguar+Eagle's package, target and tools temporarily
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
	}

	[ "$twopcie" = "1" ] && {
		echo "2pcie"
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=0/CONFIG_MTK_HWIFI_MT7992_OPTION_TYPE=2/g' ${BUILD_DIR}/.config
		sed -i 's/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=0/CONFIG_MTK_HWIFI_MT7992_INTR_OPTION_SET=8/g' ${BUILD_DIR}/.config
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

}

#do prepare stuff
prepare

sed -i 's/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE is not set/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_SINGLE=y/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
sed -i 's/CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG=y/# CONFIG_AQUANTIA_PHY_FW_DOWNLOAD_GANG is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

#hack mt7622 config-5.4
echo "# CONFIG_MEDIATEK_NETSYS_V2 is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_PINCTRL_MT7986 is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_PCIE_MEDIATEK_GEN3 is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_MTK_ICE_DEBUG is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_GPY211_PHY is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_USB_XHCI_MTK_DEBUGFS is not set" >> ./target/linux/mediatek/mt7622/config-5.4
echo "# CONFIG_COMMON_CLK_MT7986 is not set" >> ./target/linux/mediatek/mt7622/config-5.4

echo "RSSMAP0=0" >> .//package/mtk/drivers/wifi-profile/files/mt7992/mt7992.5040.b0.dat
echo "RSSMAP0=0" >> .//package/mtk/drivers/wifi-profile/files/mt7992/mt7992.5040.b1.dat
echo "RSSMAP1=0" >> .//package/mtk/drivers/wifi-profile/files/mt7992/mt7992.5040.b0.dat
echo "RSSMAP1=0" >> .//package/mtk/drivers/wifi-profile/files/mt7992/mt7992.5040.b1.dat
prepare_final ${branch_name}

sed -i 's/CONFIG_PACKAGE_kmod-mt7992=y/# CONFIG_PACKAGE_kmod-mt7992 is not set/g' ${BUILD_DIR}/.config

#step2 build
build ${branch_name} -j1 || [ "$LOCAL" != "1" ]
