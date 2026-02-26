#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}

#sepcify the branch to align
import_branch_name=mt7988-mt7990-BE19000

if [ "$1" == "run_cfg80211" ]; then
echo "run cfg80211 build"
rm -rf ${BUILD_DIR}/package/network/services/hostapd
rm -rf ${BUILD_DIR}/package/kernel/mac80211

rm -rf ${BUILD_DIR}/package/libs/libnl-tiny
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/libs/libnl-tiny ${BUILD_DIR}/package/libs

rm -rf ${BUILD_DIR}/package/network/utils/iw
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iw ${BUILD_DIR}/package/network/utils

rm -rf ${BUILD_DIR}/package/network/utils/iwinfo
cp -fpR ${BUILD_DIR}/./../mac80211_package/package/network/utils/iwinfo ${BUILD_DIR}/package/network/utils

sed -i '/^[\t ]*\[[ ]*![ ]*-f[ ]*\/etc\/config\/wireless[ ]*\]/a \\t\t\/sbin\/wifi detect' ${BUILD_DIR}/package/base-files/files/etc/init.d/boot

#use hostapd master package revision, remove hostapd 2102 patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-hostapd-*.patch" -delete

#use mt7986_dev2 branch, remove mt76 master patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-mt76-*.patch" -delete

#enable hostapd/wpa_supplicant related packages
sed -i 's/# CONFIG_PACKAGE_mwctl is not set/CONFIG_PACKAGE_mwctl=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_PACKAGE_kmod-cfg80211 is not set/CONFIG_PACKAGE_kmod-cfg80211=y/g' autobuild/${branch_name}/.config
#sed -i 's/# CONFIG_PACKAGE_wapp is not set/CONFIG_PACKAGE_wapp=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_SUPPORT_LSDK_NVRAM_CMD is not set/CONFIG_SUPPORT_LSDK_NVRAM_CMD=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_PACKAGE_kmod-mapfilter is not set/CONFIG_PACKAGE_kmod-mapfilter=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_MTK_WIFI7_CFG80211_SUPPORT is not set/CONFIG_MTK_WIFI7_CFG80211_SUPPORT=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_PACKAGE_hostapd-common is not set/CONFIG_PACKAGE_hostapd-common=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_PACKAGE_wpad-openssl is not set/CONFIG_PACKAGE_wpad-openssl=y/g' autobuild/${branch_name}/.config
sed -i 's/# CONFIG_PACKAGE_iperf is not set/CONFIG_PACKAGE_iperf=y/g' autobuild/${branch_name}/.config
echo "CONFIG_PACKAGE_hostapd-utils=y" >> autobuild/${branch_name}/.config
echo "CONFIG_PACKAGE_wpa-cli=y" >> autobuild/${branch_name}/.config
echo "CONFIG_WPA_MSG_MIN_PRIORITY=0" >> autobuild/${branch_name}/.config
echo "CONFIG_MTK_WIFI7_HOSTAPD_MBSS_SUPPORT=y" >> autobuild/${branch_name}/.config
echo "CONFIG_MTK_WIFI7_APCLI_SUPPLICANT_SUPPORT=y" >> autobuild/${branch_name}/.config
fi

#step1 clean
#clean
#do prepare stuff
prepare

#install mtk feed target
#./scripts/feeds install mtk
#copy U3 FPGA test patch from mt7986
cp -fpR ${BUILD_DIR}/autobuild/mt7986-FPGA/target/linux/mediatek/files-5.4/drivers/phy/ ${BUILD_DIR}/target/linux/mediatek/files-5.4/drivers/
cp -fpR ${BUILD_DIR}/autobuild/mt7986-FPGA/target/linux/mediatek/patches-5.4/800*.patch ${BUILD_DIR}/target/linux/mediatek/patches-5.4/
#cp eth driver from mtk_feed file-5.4 
#cp -fpR ${BUILD_DIR}/target/linux/mediatek/files-5.4/drivers/net/ethernet/ ${BUILD_DIR}/target/linux/mediatek/files-5.10/drivers/net/
#copy necessary files from main autolbuild folder
cp -fpR ${BUILD_DIR}/autobuild/${import_branch_name}/target/ ${BUILD_DIR}
cp -fpR ${BUILD_DIR}/autobuild/${import_branch_name}/package/ ${BUILD_DIR}
cp -fpR ${BUILD_DIR}/autobuild/${import_branch_name}/tools/ ${BUILD_DIR}

if [ "$1" == "run_cfg80211" ]; then
	sed -i 's/$(LN) iwconfig $(1)\/usr\/sbin\/iwpriv/$(LN) mwctl $(1)\/usr\/sbin\/iwpriv/g' ${BUILD_DIR}/package/network/utils/wireless-tools/Makefile
fi

#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

#step2 build
build ${branch_name} -pb || [ "$LOCAL" != "1" ]
