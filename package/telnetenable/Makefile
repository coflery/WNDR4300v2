# 
# Copyright (C) 2008 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=telnetenable
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/telnetenable
	SECTION:=utils
	CATEGORY:=Network
	DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
	TITLE:=unlocking the utelnetd.
endef

define Package/telnetenable/description
 It is used for NTGR's Router Debug mode.
endef

define Package/telnetenable/config
	menu "Configuration"
		depends on PACKAGE_telnetenable
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/telnetenable.git/* $(PKG_BUILD_DIR)/
endef

define Package/telnetenable/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/telnetenable $(1)/usr/sbin/
	install -d -m0755 $(1)/lib/network
	install -m0755 ./files/RtDebug.sh $(1)/lib/network/
endef

$(eval $(call BuildPackage,telnetenable))
