include $(TOPDIR)/rules.mk

PKG_NAME:=anld
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/anld
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=airoha navigation layer
  DEPENDS:=+libmbedtls +libcurl
  MENU:=1
endef

define Package/anld/description
  airoha navigation library daemon
endef

TARGET_LDFLAGS += -lmbedtls -lmbedx509 -lmbedcrypto -lcurl

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/anld/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/anld $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/default/anld/epo
	$(INSTALL_BIN) ./files/anld.conf $(1)/etc/default/anld/anld.conf
	$(INSTALL_DIR) $(1)/etc/default/anld/da
	$(INSTALL_BIN) ./files/da/{ag3352_da.bin,ag3352_da_spi.bin} $(1)/etc/default/anld/da
	$(INSTALL_DIR) $(1)/etc/default/anld/load
	$(INSTALL_BIN) ./files/load/{bootloader.bin,gnss_config.bin,gnss_demo.bin,partition_table.bin,flash_download.cfg} $(1)/etc/default/anld/load
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/anld.init $(1)/etc/init.d/anld
endef

$(eval $(call BuildPackage,anld))
