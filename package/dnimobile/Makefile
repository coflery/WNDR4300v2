# 
#  Copyright (C) 2007-2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dnimobile
PKG_VERSION:=0.01
PKG_RELEASE:=1

PKG_GIT_TREEISH:=dnimobile-dni11

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/dnimobile
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=dnimobile
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/dnimobile.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/dnimobile/install
	install -d -m0755 $(1)/bin
	install -m0755 $(PKG_BUILD_DIR)/mobilectl $(1)/bin/
	install -m0755 $(PKG_BUILD_DIR)/mobiled $(1)/bin/
endef

$(eval $(call BuildPackage,dnimobile))
