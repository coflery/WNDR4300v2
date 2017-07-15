# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.2.3 2007/01/12 02:45:49 ronger Exp $

include $(TOPDIR)/rules.mk

PKG_NAME:=lltd
PKG_VERSION:=0.1
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=$(DNI_DL_WEB) http://gentoo.osuosl.org/distfiles @SF/utelnetd
#PKG_MD5SUM:=45fc2b66b3d2dade396ed122a41d5b23
#PKG_CAT:=zcat

PKG_GIT_TREEISH:=$(CONFIG_LLTD_GIT_TREEISH)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/native-linux

include $(INCLUDE_DIR)/package.mk

define Package/lltd
  SECTION:=net
  CATEGORY:=Base system
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=Link Layer Topology Discovery Daemon
  URL:=http://lltd.sourceforge.net/
endef

define Package/lltd/config
	menu "Configurations"
		depends on PACKAGE_lltd
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/lltd.git/* $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/
endef

define Build/Compile
endef

define Build/Configure
endef

define Package/lltd/install
	install -m0755 -d $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/lld2d $(1)/usr/sbin/
	install -m0755 -d $(1)/etc
	install -m0755 $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/files/lld2d.conf  $(1)/etc/
	install -m0755 ./files/*_large.ico  $(1)/etc/
	install -m0755 ./files/*_icon.ico $(1)/etc/
	ln -s /tmp/icon.ico $(1)/etc/icon.ico
	ln -s /tmp/large.ico $(1)/etc/large.ico
	#install -m0755 -d $(1)/etc/init.d
	#install -m0755 $(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)/files/lltd.init $(1)/etc/init.d/lltd
endef

$(eval $(call BuildPackage,lltd))
