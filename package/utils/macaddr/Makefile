include $(TOPDIR)/rules.mk

PKG_NAME:=macaddr
PKG_VERSION:=1.0
PKG_RELEASE:=5

PKG_MAINTAINER:=Carson <leon215@163.com>
PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define Package/macaddr
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Factory MAC address management for MTK MT7981
  DEPENDS:=+busybox +mtd
endef

define Build/Compile
endef

define Package/macaddr/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/macaddr.sh $(1)/usr/sbin/macaddr
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/macaddr.init $(1)/etc/init.d/macaddr
endef

$(eval $(call BuildPackage,macaddr))