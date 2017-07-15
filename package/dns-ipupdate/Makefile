# 
#  Copyright (C) 2007-2008 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dns-ipupdate
PKG_RELEASE:=2
PKG_BUILD_DEPENDS:=datalib
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH:=$(CONFIG_DNS_IPUPDATE_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define Package/dns-ipupdate
	SECTION:=net
	CATEGORY:=Base system
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
	TITLE:=Dynamic DNS application.
endef

define Package/dns-ipupdate/config
	menu "Configurations"
		depends on PACKAGE_dns-ipupdate
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/dns-ipupdate.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/dns-ipupdate/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/updatednsip $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,dns-ipupdate))
