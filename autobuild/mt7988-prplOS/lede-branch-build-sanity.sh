#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
hwpath=0
backport_new=1
release=1
release_folder=${BUILD_DIR}/feeds/mtk_openwrt_feed/autobuild_mac80211_release
args=

for arg in $*; do
	case "$arg" in
	"hwpath")
		hwpath=1
		;;
	"release")
		release=1
		;;
	*)
		args="$args $arg"
		;;
	esac
done
set -- $args

change_dot_config() {
	[ "$hwpath" = "0" ] && {
		echo "==========SW PATH========="
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
	}
	[ "$backport_new" = "1" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*revert-for-backports*.patch
        }
	[ "$release" = "1" ] && {
		cp -rfa ${release_folder}/package/kernel/mt76/src/firmware ${BUILD_DIR}/package/kernel/mt76/src
        }
}

#prplos bypass some feed patch
rm -f ${BUILD_DIR}/autobuild/openwrt_patches-21.02/mtk_soc/09*.patch
rm -f ${BUILD_DIR}/autobuild/openwrt_patches-21.02/mtk_soc/8000-uboot-mediatek-makefile.patch

#step1 clean
#clean
#do prepare stuff
prepare

#hack mt7988 config5.4
echo "CONFIG_NETFILTER=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_NETFILTER_ADVANCED=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
echo "CONFIG_RELAY=y" >> ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4

prepare_flowoffload

#prepare mac80211 mt76 wifi stuff
prepare_mac80211 ${backport_new}

#prepare_final ${branch_name}
rm -rf ${BUILD_DIR}/target/linux/mediatek/patches-5.4/0303-mtd-spinand-disable-on-die-ECC.patch

#prepare prplOS
#refactor feed config
echo "src-git packages https://gerrit.mediatek.inc/openwrt/feeds/packages;openwrt-21.02" > ${BUILD_DIR}/feeds.conf.default
echo "src-git luci https://gerrit.mediatek.inc/openwrt/feeds/luci;openwrt-21.02" >> ${BUILD_DIR}/feeds.conf.default
echo "src-git routing https://gerrit.mediatek.inc/openwrt/feeds/routing;openwrt-21.02" >> ${BUILD_DIR}/feeds.conf.default
echo "src-git telephony https://git.openwrt.org/feed/telephony.git;openwrt-21.02" >> ${BUILD_DIR}/feeds.conf.default
echo "src-git mtk_openwrt_feed https://git01.mediatek.com/openwrt/feeds/mtk-openwrt-feeds" >> ${BUILD_DIR}/feeds.conf.default
#Official latest prplos
#git clone https://gitlab.com/prpl-foundation/prplos/prplos.git

#Internal good tag prplos yml/script
cp ${BUILD_DIR}/autobuild/${branch_name}/scripts_20220518/gen_config.py ${BUILD_DIR}/scripts
cp ${BUILD_DIR}/autobuild/${branch_name}/scripts_20220518/feeds ${BUILD_DIR}/scripts

# prplos + pWHM build (new)
cp -r ${BUILD_DIR}/autobuild/${branch_name}/profiles_prplOS ${BUILD_DIR}/profiles
cp ${BUILD_DIR}/autobuild/${branch_name}/mtk_profiles/*.* ${BUILD_DIR}/profiles
${BUILD_DIR}/scripts/gen_config.py prpl webui mt7988
do_patch ${BUILD_DIR}/autobuild/${branch_name}/patches_pwhm || exit 1

mkdir -p ${BUILD_DIR}/feeds/feed_prpl/ambiorix/libs/libsahtrace/patches
mkdir -p ${BUILD_DIR}/feeds/feed_prpl/prplmesh/patches
mkdir -p ${BUILD_DIR}/feeds/feed_prpl/network/plugins/tr181-bridging/patches
cp ${BUILD_DIR}/autobuild/${branch_name}/999-tr181-bridging-remove-old-type-br.patch ${BUILD_DIR}/feeds/feed_prpl/network/plugins/tr181-bridging/patches
rm ${BUILD_DIR}/package/network/config/netifd/files/etc/udhcpc.user
echo "CONFIG_PACKAGE_wpad-openssl=y" >> ${BUILD_DIR}/.config
echo "CONFIG_PACKAGE_hostapd-utils=y" >> ${BUILD_DIR}/.config
echo "CONFIG_PACKAGE_prplmesh=y" >> ${BUILD_DIR}/.config
#endif prplos patch

#end prplOs


# find ${BUILD_DIR}/package/kernel/mt76/patches -name "*-mt76-*.patch" -delete
rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*

# ========== specific modification on mt7996 autobuild for EHT support ==========
# patch mac80211.sh script
patch -p1 < ${release_folder}/mt7988_mt7996_mac80211/0001-support-EHT-for-mac80211.sh.patch
# patch hostapd to use latest version and add 11BE config
patch -p1 < ${release_folder}/mt7988_mt7996_mac80211/0002-add-EHT-config-for-hostapd.patch
# copy DSP & testmode firmware
patch -p1 < ${release_folder}/mt7988_mt7996_mac80211/0003-mt76-mt7996-copy-dsp-and-testmode-firmware.patch

# remove some iw patches to let EHT work normally
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/001-nl80211_h_sync.patch
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/120-antenna_gain.patch
# ===========================================================

cp -rf ${release_folder}/mt7988_mt7996_mac80211/package/kernel/mt76 ${BUILD_DIR}/package/kernel

change_dot_config

make defconfig
make -j8
