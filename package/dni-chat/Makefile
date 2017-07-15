# 
#  Copyright (C) 2007 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dni-chat
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/dni-chat
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=send AT command to modem
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/dni-chat/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/dnichat $(1)/usr/sbin/
endef

#call func is to expand BuildPackage with "dnichat" as the param.
$(eval $(call BuildPackage,dni-chat))
