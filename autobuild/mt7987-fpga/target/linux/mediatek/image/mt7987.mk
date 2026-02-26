KERNEL_LOADADDR := 0x48080000

#define Device/mediatek_mt7987a-sfp-spim-nand
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-sfp-spim-nand
#  DEVICE_DTS := mt7987a-sfp-spim-nand
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  SUPPORTED_DEVICES := mediatek,mt7987a-sfp-spim-snand
#  UBINIZE_OPTS := -E 5
#  BLOCKSIZE := 128k
#  PAGESIZE := 2048
#  IMAGE_SIZE := 65536k
#  KERNEL_IN_UBI := 1
#  IMAGES += factory.bin
#  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
#  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
#endef
#TARGET_DEVICES += mediatek_mt7987a-sfp-spim-nand

define Device/mediatek_mt7987a-spim-nand
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand
  DEVICE_DTS := mt7987a-spim-nand
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  DEVICE_DTS_OVERLAY := mt7987-net-eth1-i2p5g-eth2-e2p5g mt7987-net-eth1-e2p5g-eth2-e2p5g
  DEVICE_DTC_FLAGS := -@ --pad 4096
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand

define Device/mediatek_mt7987-fpga
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987-fpga
  DEVICE_DTS := mt7987-fpga
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987-fpga
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987-fpga

define Device/mediatek_mt7987a-spim-nand-eth1-i2p5g-eth2-e2p5g
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-eth1-i2p5g-eth2-e2p5g
  DEVICE_DTS := mt7987a-spim-nand-eth1-i2p5g-eth2-e2p5g
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-eth1-i2p5g-eth2-e2p5g

define Device/mediatek_mt7987a-spim-nand-eth1-e2p5g-eth2-e2p5g
  DEVICE_VENDOR := MediaTek
  DEVICE_MODEL := mt7987a-spim-nand-eth1-e2p5g-eth2-e2p5g
  DEVICE_DTS := mt7987a-spim-nand-eth1-e2p5g-eth2-e2p5g
  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
  SUPPORTED_DEVICES := mediatek,mt7987a-spim-snand
  UBINIZE_OPTS := -E 5
  BLOCKSIZE := 128k
  PAGESIZE := 2048
  IMAGE_SIZE := 65536k
  KERNEL_IN_UBI := 1
  IMAGES += factory.bin
  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
endef
TARGET_DEVICES += mediatek_mt7987a-spim-nand-eth1-e2p5g-eth2-e2p5g


#define Device/mediatek_mt7987a-e2p5g-spim-nand
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-e2p5g-spim-nand
#  DEVICE_DTS := mt7987a-e2p5g-spim-nand
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  SUPPORTED_DEVICES := mediatek,mt7987a-e2p5g-spim-nand
#  UBINIZE_OPTS := -E 5
#  BLOCKSIZE := 128k
#  PAGESIZE := 2048
#  IMAGE_SIZE := 65536k
#  KERNEL_IN_UBI := 1
#  IMAGES += factory.bin
#  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
#  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
#endef
#TARGET_DEVICES += mediatek_mt7987a-e2p5g-spim-nand
#
#define Device/mediatek_mt7987a-i2p5g-spim-nand
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-i2p5g-spim-nand
#  DEVICE_DTS := mt7987a-i2p5g-spim-nand
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  SUPPORTED_DEVICES := mediatek,mt7987a-i2p5g-spim-nand
#  UBINIZE_OPTS := -E 5
#  BLOCKSIZE := 128k
#  PAGESIZE := 2048
#  IMAGE_SIZE := 65536k
#  KERNEL_IN_UBI := 1
#  IMAGES += factory.bin
#  IMAGE/factory.bin := append-ubi | check-size $$$$(IMAGE_SIZE)
#  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
#endef
#TARGET_DEVICES += mediatek_mt7987a-i2p5g-spim-nand

#define Device/mediatek_mt7987a-spim-nor
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-spim-nor
#  DEVICE_DTS := mt7987a-spim-nor
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  SUPPORTED_DEVICES := mediatek,mt7987a-spim-nor
#endef
#TARGET_DEVICES += mediatek_mt7987a-spim-nor

#define Device/mediatek_mt7987a-emmc
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-emmc
#  DEVICE_DTS := mt7987a-emmc
#  SUPPORTED_DEVICES := mediatek,mt7987a-emmc
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
#		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
#		     kmod-nls-iso8859-1
#  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
#endef
#TARGET_DEVICES += mediatek_mt7987a-emmc

#define Device/mediatek_mt7987a-sd
#  DEVICE_VENDOR := MediaTek
#  DEVICE_MODEL := mt7987a-sd
#  DEVICE_DTS := mt7987a-sd
#  SUPPORTED_DEVICES := mediatek,mt7987a-sd
#  DEVICE_DTS_DIR := $(DTS_DIR)/mediatek
#  DEVICE_PACKAGES := mkf2fs e2fsprogs blkid blockdev losetup kmod-fs-ext4 \
#		     kmod-mmc kmod-fs-f2fs kmod-fs-vfat kmod-nls-cp437 \
#		     kmod-nls-iso8859-1
#  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata
#endef
#TARGET_DEVICES += mediatek_mt7987a-sd
