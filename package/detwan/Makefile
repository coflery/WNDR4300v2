# 
#  Copyright (C) 2007-2008 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=detwan
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH:=$(CONFIG_DNI_DETWAN_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define Package/detwan
	SECTION:=utils
	CATEGORY:=Network
	DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
	TITLE:=Detection of Internet Connection Type
endef

define Package/detwan/config
	menu "Configurations"
		depends on PACKAGE_detwan
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/wan-detection.git/* $(PKG_BUILD_DIR)/
endef

define Package/detwan/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/detwan $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,detwan))
