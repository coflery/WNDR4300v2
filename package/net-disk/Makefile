# 
#  Copyright (C) 2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=net-disk
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/net-disk
	SECTION:=utils
	CATEGORY:=Network
	DEPENDS:=@TARGET_wndr3700u||TARGET_wndr4500v3
	TITLE:=Disk Management.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
	$(CP) ./file/* $(PKG_BUILD_DIR)
endef

define Package/net-disk/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/net-disk $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/usb_led $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/dongle_led $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/usb_led_stop $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,net-disk))
