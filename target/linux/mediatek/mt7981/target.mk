ARCH:=aarch64
SUBTARGET:=mt7981
BOARDNAME:=MT7981
CPU_TYPE:=cortex-a53
FEATURES:=squashfs nand ramdisk

KERNELNAME:=Image dtbs

DEFAULT_PACKAGES += \
    uboot-envtools \
    kmod-mt7981-firmware \
    macaddr

define Target/Description
	Build firmware images for MediaTek MT7981 ARM based boards.
endef
