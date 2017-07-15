# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.2.1 2008-03-22 04:48:39 xinwei.niu Exp $

include $(TOPDIR)/rules.mk

PKG_NAME:=realpath
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/realpath
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=a tool for ReadSHARE Cloud
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS="$(TARGET_CROSS)" \
		CFLAGS="$(TARGET_CFLAGS) -Os -O2 -D_FILE_OFFSET_BITS=64 -I$(DEP_BUILD_DIR)/include -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include -I./include" \
		LDFLAGS="-Wl,-rpath,$(STAGING_DIR)/lib -L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/lib -L./lib" \
		all
endef

define Package/$(PKG_NAME)/install
	install -d -m0755 $(1)/opt/remote/bin
	install -m0755 $(PKG_BUILD_DIR)/realpath $(1)/opt/remote/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
