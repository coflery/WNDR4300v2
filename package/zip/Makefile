# 
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=zip
PKG_VERSION:=3.0
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://sourceforge.net/projects/infozip/files/3.0/zip30.tar.gz/
PKG_MD5SUM:=7b74551e63f8ee6aab6fbc86676c0d37

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL_DIR:=$(PKG_BUILD_DIR)/ipkg-install

include $(INCLUDE_DIR)/package.mk

define Package/zip
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=@TARGET_wndr4500v3
  TITLE:=zip utils. 
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		-f unix/Makefile generic
endef

define Package/zip/install
	$(INSTALL_DIR) -d -m0775 $(1)/usr/sbin
	$(INSTALL_BIN) -m0775 $(PKG_BUILD_DIR)/zip $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,zip))
