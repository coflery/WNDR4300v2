# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 6501 2007-03-04 04:41:46Z pavlov $

include $(TOPDIR)/rules.mk

PKG_NAME:=usb-modeswitch
PKG_VERSION:=1.2.1

include $(INCLUDE_DIR)/package.mk

define Package/usb-modeswitch
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Utility for switch USB device mode
  DEPENDS:=+libusb
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Package/usb-modeswitch/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/usb_modeswitch $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/lib/usb_modeswitch.d/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/usb_modeswitch.d/* $(1)/lib/usb_modeswitch.d/
endef

$(eval $(call BuildPackage,usb-modeswitch))
