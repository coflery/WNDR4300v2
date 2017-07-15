#
# Copyright (C) 2007-2008 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

REGION_TAG=REGION_WW
ifeq ($(FW_REGION),NA)
  REGION_TAG=REGION_NA
endif
ifeq ($(FW_REGION),GR)
  REGION_TAG=REGION_GR
endif
ifeq ($(FW_REGION),PR)
  REGION_TAG=REGION_PR
endif
ifeq ($(FW_REGION),KO)
  REGION_TAG=REGION_KO
endif

PKG_NAME:=datalib
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_GIT_TREEISH:=$(CONFIG_DNI_DATALIB_GIT_TREEISH)

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
TMP_VAR:=`echo $(CONFIG_CONFIGURATION_PARTITION)`

include $(INCLUDE_DIR)/package.mk

define Package/datalib
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=Small application for saving data on embedded system
endef

define Package/datalib/config
	menu "Configurations"
		depends on PACKAGE_datalib
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/datalib.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libconfig.so $(STAGING_DIR)/usr/lib/
endef

define Build/UninstallDev
	rm -f $(STAGING_DIR)/usr/lib/libconfig.so
endef

define Package/datalib/install
	install -d -m0755 $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libconfig.so $(1)/usr/lib/
	install -d -m0755 $(1)/bin
	install -m0755 $(PKG_BUILD_DIR)/datalib $(1)/bin/
	install -m0755 $(PKG_BUILD_DIR)/config $(1)/bin/
endef

$(eval $(call BuildPackage,datalib))
