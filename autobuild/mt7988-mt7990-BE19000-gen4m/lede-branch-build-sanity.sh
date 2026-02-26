#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
#step1 clean
#clean

MT7990_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7990-BE19000
MT7990_GEN4M_BDIR=${BUILD_DIR}/./autobuild/mt7988-mt7990-BE19000-gen4m


cp -fpR ${MT7990_BDIR}/package ${MT7990_GEN4M_BDIR}
cp -fpR ${MT7990_BDIR}/target ${MT7990_GEN4M_BDIR}
cp -fpR ${MT7990_BDIR}/tools ${MT7990_GEN4M_BDIR}

# setup wifi.cfg
echo "EfuseBufferModeCal 1" >> ${BUILD_DIR}/../ko_module/wlan_driver/unified/bin/bellwether/mobile/wifi.cfg

rm -rf ${BUILD_DIR}/package/network/services/hostapd
rm -rf ${BUILD_DIR}/package/kernel/mac80211


#use hostapd master package revision, remove hostapd 2102 patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-2102-hostapd-*.patch" -delete
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-hostapd-*.patch" -delete

#use mt7986_dev2 branch, remove mt76 master patches
find ../mtk-openwrt-feeds/openwrt_patches-21.02 -name "*-master-mt76-*.patch" -delete


echo "# CONFIG_AQUANTIA_PHY_MDI_SWAP is not set" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

prepare
#prepare mtk jedi wifi stuff
prepare_mtwifi ${branch_name}

prepare_final ${branch_name}

patch -f -p1 -d ${BUILD_DIR}/feeds/luci -i ${BUILD_DIR}/autobuild/${branch_name}/0100_disable_web_rollback.patch
#step2 build
build ${branch_name} -pb || [ "$LOCAL" != "1" ]

