# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.2.3 2007/03/11 05:08:58 agui Exp $

include $(TOPDIR)/rules.mk

PKG_NAME:=pot
PKG_RELEASE:=4

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

#PKG_GIT_TREEISH=pot-v1.0.4

include $(INCLUDE_DIR)/package.mk

define Package/pot
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=POT
endef

define Package/pot/config
	menu "Configurations"
		depends on PACKAGE_pot
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/pot.git/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	test x$(CONFIG_NOR_FLASH) != x \
		&& $(CP) ./files/config.h $(PKG_BUILD_DIR)/config.h \
		|| $(CP) ./files/nand-config.h $(PKG_BUILD_DIR)/config.h
endef

define Package/pot/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/potd $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/potval $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/ntpst $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/stamac $(1)/usr/sbin/
	install -d -m0755 $(1)/etc/init.d
	install -m0755 ./files/pot.init $(1)/etc/init.d/pot
endef

$(eval $(call BuildPackage,pot))
