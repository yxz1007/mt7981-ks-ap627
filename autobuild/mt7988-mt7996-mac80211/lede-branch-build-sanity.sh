#!/bin/bash
source ./autobuild/lede-build-sanity.sh

#get the brach_name
temp=${0%/*}
branch_name=${temp##*/}
swpath=0
backport_new=1
hostapd_new=1
release=0
release_folder=${BUILD_DIR}/feeds/mtk_openwrt_feed/autobuild_mac80211_release
args=

for arg in $*; do
	case "$arg" in
	"swpath")
		swpath=1
		;;
	"kasan")
		kasan=1
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
	[ "$swpath" = "1" ] && {
		echo "==========SW PATH========="
		sed -i 's/CONFIG_BRIDGE_NETFILTER=y/# CONFIG_BRIDGE_NETFILTER is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_NETFILTER_FAMILY_BRIDGE=y/# CONFIG_NETFILTER_FAMILY_BRIDGE is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i 's/CONFIG_SKB_EXTENSIONS=y/# CONFIG_SKB_EXTENSIONS is not set/g' ${BUILD_DIR}/target/linux/mediatek/mt7988/config-5.4
		sed -i '/AUTOLOAD:=$(call AutoProbe,mt7996e)/a\  MODPARAMS.mt7996e:=wed_enable=0' ${BUILD_DIR}/package/kernel/mt76/Makefile
	}
	[ "$backport_new" = "1" ] && {
		rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*revert-for-backports*.patch
	}
	[ "$kasan" = "1" ] && {
		sed -i 's/# CONFIG_KERNEL_KASAN is not set/CONFIG_KERNEL_KASAN=y/g' ${BUILD_DIR}/.config
		sed -i 's/# CONFIG_KERNEL_KALLSYMS is not set/CONFIG_KERNEL_KALLSYMS=y/g' ${BUILD_DIR}/.config
		echo "CONFIG_KERNEL_KASAN_OUTLINE=y" >> ${BUILD_DIR}/.config
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
	[ "$release" = "1" ] && {
		cp -rfa ${release_folder}/package/kernel/mt76/src/firmware ${BUILD_DIR}/package/kernel/mt76/src
	}
}

#step1 clean
#clean
#do prepare stuff
prepare

prepare_flowoffload

#wed3.0 patches from mtk_openwrt_feeds
cp -rf ${release_folder}/mt7988_mt7996_mac80211/target/* ${BUILD_DIR}/target/

#prepare mac80211 mt76 wifi stuff
prepare_mac80211 ${backport_new} ${hostapd_new}

copy_mt76_firmware() {
	#hack mt76 firmware/eeprom
	#===================firmware bin name format=========================
	#define MT7996_FIRMWARE_WA		"mediatek/mt7996/mt7996_wa.bin"
	#define MT7996_FIRMWARE_WM		"mediatek/mt7996/mt7996_wm.bin"
	#define MT7996_FIRMWARE_DSP		"mediatek/mt7996/mt7996_dsp.bin"
	#define MT7996_ROM_PATCH		"mediatek/mt7996/mt7996_rom_patch.bin"
	#define MT7996_FIRMWARE_WM_TM		"mediatek/mt7996/mt7996_wm_tm.bin"

	#define MT7992_FIRMWARE_WA		"mediatek/mt7996/mt7992_wa.bin"
	#define MT7992_FIRMWARE_WM		"mediatek/mt7996/mt7992_wm.bin"
	#define MT7992_FIRMWARE_DSP		"mediatek/mt7996/mt7992_dsp.bin"
	#define MT7992_ROM_PATCH		"mediatek/mt7996/mt7992_rom_patch.bin"
	#define MT7992_FIRMWARE_WM_TM		"mediatek/mt7996/mt7992_wm_tm.bin"

	#define MT7992_FIRMWARE_WA_23		"mediatek/mt7996/mt7992_wa_23.bin"
	#define MT7992_FIRMWARE_WM_23		"mediatek/mt7996/mt7992_wm_23.bin"
	#define MT7992_FIRMWARE_DSP_23		"mediatek/mt7996/mt7992_dsp_23.bin"
	#define MT7992_ROM_PATCH_23		"mediatek/mt7996/mt7992_rom_patch_23.bin"
	#define MT7992_FIRMWARE_WM_TM_23	"mediatek/mt7996/mt7992_wm_tm_23.bin"

	#define MT7992_FIRMWARE_WA_24		"mediatek/mt7996/mt7992_wa_24.bin"
	#define MT7992_FIRMWARE_WM_24		"mediatek/mt7996/mt7992_wm_24.bin"
	#define MT7992_FIRMWARE_DSP_24		"mediatek/mt7996/mt7992_dsp_24.bin"
	#define MT7992_ROM_PATCH_24		"mediatek/mt7996/mt7992_rom_patch_24.bin"
	#define MT7992_FIRMWARE_WM_TM_24	"mediatek/mt7996/mt7992_wm_tm_24.bin"

	FW_SOURCE_DIR=${BUILD_DIR}/package/kernel/mt76

	cp -rfa ${release_folder}/package/kernel/mt76/src/firmware ${FW_SOURCE_DIR}/src

	#hack up-to-date internal IMP firmware
	mkdir -p ${FW_SOURCE_DIR}/src/firmware/mt7996
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/WIFI_MT7990_WACPU_RAM_CODE_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wa.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/WIFI_RAM_CODE_MT7990_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wm.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/WIFI_MT7990_PATCH_MCU_1_1_hdr.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_rom_patch.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/WIFI_MT7990_PHY_RAM_CODE_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_dsp.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/WIFI_RAM_CODE_MT7990_1_1_TESTMODE.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wm_tm.bin

	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/2adie_triband/WIFI_MT7990_WACPU_RAM_CODE_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wa_233.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/2adie_triband/WIFI_RAM_CODE_MT7990_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wm_233.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/2adie_triband/WIFI_MT7990_PATCH_MCU_1_1_hdr.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_rom_patch_233.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7990/rebb/2adie_triband/WIFI_RAM_CODE_MT7990_1_1_TESTMODE.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7996_wm_tm_233.bin

	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_WACPU_RAM_CODE_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wa.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PATCH_MCU_1_1_hdr.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_rom_patch.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PHY_RAM_CODE_1_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_dsp.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_1_1_TESTMODE.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm_tm.bin

	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_WACPU_RAM_CODE_2_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wa_23.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_2_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm_23.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PATCH_MCU_2_1_hdr.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_rom_patch_23.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PHY_RAM_CODE_2_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_dsp_23.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_2_1_TESTMODE.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm_tm_23.bin

	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_WACPU_RAM_CODE_3_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wa_24.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_3_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm_24.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PATCH_MCU_3_1_hdr.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_rom_patch_24.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_MT7992_PHY_RAM_CODE_3_1.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_dsp_24.bin
	cp -rf ${FW_SOURCE_DIR}/firmware/mt7992/rebb/WIFI_RAM_CODE_MT7992_3_1_TESTMODE.bin ${FW_SOURCE_DIR}/src/firmware/mt7996/mt7992_wm_tm_24.bin
}

# find ${BUILD_DIR}/package/kernel/mt76/patches -name "*-mt76-*.patch" -delete
rm -rf ${BUILD_DIR}/package/kernel/mt76/patches/*

# remove crypto-eip package since it not support at mt76 yet
rm -rf ${BUILD_DIR}/package/mtk_soc/drivers/crypto-eip/

# ========== specific modification on mt7996 autobuild for EHT support ==========
# patch hostapd to use latest version and add 11BE config
patch -p1 < ${release_folder}/mt7988_mt7996_mac80211/0002-add-EHT-config-for-hostapd.patch || exit 1

# remove some iw patches to let EHT work normally
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/001-nl80211_h_sync.patch
rm -rf ${BUILD_DIR}/package/network/utils/iw/patches/120-antenna_gain.patch
# ===========================================================

if [ "$release" = '1' ]; then
	# For Kite/MT7992 firmware because git01 repo doesn't include MT7992 firmware, will remove it once it MP
	copy_mt76_firmware
	# ========== follow mt7988_mt7996_mac80211 git01 patches ========================
	rm -rf ${BUILD_DIR}/autobuild/${branch_name}/package
	cp -rf ${release_folder}/mt7988_mt7996_mac80211/package/* ${BUILD_DIR}/package/
	rm -rf ${BUILD_DIR}/autobuild/${branch_name}/target
	cp -rf ${release_folder}/mt7988_mt7996_mac80211/target/* ${BUILD_DIR}/target/
	# ===========================================================
else
	# Remove mtk internal patches and follow gateway/WiFi7/mac80211/mt76 source folder
	WIFI7_MT76_SOURCE_DIR=${BUILD_DIR}/package/kernel/mt76/src; rm -rf ${WIFI7_MT76_SOURCE_DIR};
	git clone -q ${BUILD_DIR}/./../mac80211_package/wifi7_mt76_src ${WIFI7_MT76_SOURCE_DIR}
	cd ${WIFI7_MT76_SOURCE_DIR}; git remote add gerrit https://gerrit.mediatek.inc/gateway/WiFi7/mac80211/mt76; git fetch gerrit --unshallow; cd -;
	mv ${WIFI7_MT76_SOURCE_DIR}/../patches ${WIFI7_MT76_SOURCE_DIR}/../patches_bak
	copy_mt76_firmware

	# Remove mtk internal patches and follow gateway/WiFi7/mac80211/backport source folder
	WIFI7_MAC80211_SOURCE_DIR=${BUILD_DIR}/package/kernel/mac80211/src; rm -rf ${WIFI7_MAC80211_SOURCE_DIR};
	git clone -q ${BUILD_DIR}/./../mac80211_package/wifi7_backport_src ${WIFI7_MAC80211_SOURCE_DIR}
	cd ${WIFI7_MAC80211_SOURCE_DIR}; git remote add gerrit https://gerrit.mediatek.inc/gateway/WiFi7/mac80211/backport; git fetch gerrit --unshallow; cd -;

 	# Remove mtk internal patches and follow gateway/WiFi7/mac80211/hostapd source folder
	WIFI7_Hostapd_SOURCE_DIR=${BUILD_DIR}/package/network/services/hostapd/src; rm -rf ${WIFI7_Hostapd_SOURCE_DIR};
	git clone -q ${BUILD_DIR}/./../mac80211_package/wifi7_hostapd_src ${WIFI7_Hostapd_SOURCE_DIR}
	cd ${WIFI7_Hostapd_SOURCE_DIR}; git remote add gerrit https://gerrit.mediatek.inc/gateway/WiFi7/mac80211/hostapd; git fetch gerrit --unshallow; cd -;
	mv ${WIFI7_Hostapd_SOURCE_DIR}/../patches ${WIFI7_Hostapd_SOURCE_DIR}/../patches_bak
fi

prepare_final ${branch_name}

change_dot_config

#step2 build
if [ -z ${1} ]; then
	build_log ${branch_name} -j1 || [ "$LOCAL" != "1" ]
fi
