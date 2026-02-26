include $(TOPDIR)/rules.mk

PKG_NAME:=tclient
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/tclient
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=test client
  DEPENDS:=
  MENU:=1
endef

define Package/tclient/description
  client for communication
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -Wall" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/tclient/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tclient $(1)/usr/bin
endef

$(eval $(call BuildPackage,tclient))
