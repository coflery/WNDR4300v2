# 
#  Copyright (C) 2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ookla
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ookla
	SECTION:=utils
	CATEGORY:=Network
	DEPENDS:=@TARGET_wndr4500v3||TARGET_wndr3700u||TARGET_wndr3700v4
	TITLE:=ookla daemon for support speedtest
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/ookla/install
	install -d -m0755 $(1)/bin
	install -m0755 $(PKG_BUILD_DIR)/ookla $(1)/bin/
endef

$(eval $(call BuildPackage,ookla))
