# 
#  Copyright (C) 2007-2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=acl
PKG_RELEASE:=1

PKG_GIT_TREEISH:=$(CONFIG_DNI_ACL_GIT_TREEISH)

PKG_BUILD_DEPENDS:=datalib
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/acl
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=Manage ACL.
endef

define Package/acl/config
	menu "Configurations"
		depends on PACKAGE_acl
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/dni-acl.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/acl/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/acld $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/acl_update_name $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/aclctl $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/aclhijackdns $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/files/acl $(1)/etc/init.d/acl
endef

$(eval $(call BuildPackage,acl))
