# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.4.3 2007/02/27 03:06:36 ronger Exp $

include $(TOPDIR)/rules.mk

PKG_NAME:=ntpclient
PKG_RELEASE:=4
PKG_BUILD_DEPENDS:=datalib
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH=$(CONFIG_NTPCLIENT_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define Package/dni-ntpclient
  SECTION:=utils
  CATEGORY:=Base system
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=The CGI interface
endef

define Package/dni-ntpclient/config
	menu "Configurations"
		depends on PACKAGE_dni-ntpclient
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Package/dni-ntpclient/description
	The CGI interface
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/ntpclient.git/* $(PKG_BUILD_DIR)/
endef

define Package/dni-ntpclient/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/ntpclient $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,dni-ntpclient))
