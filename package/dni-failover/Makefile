# 
#  Copyright (C) 2007-2009 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=failover
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/failover
	SECTION:=utils
	CATEGORY:=Network
	TITLE:=failover
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/failover/install
	install -d -m0755 $(1)/usr/sbin
	install -d -m0755 $(1)/etc/init.d
	install -d -m0755 $(1)/etc/failover
	install -m0755 ./files/failover.init $(1)/etc/init.d/
	install -m0755 ./files/failover.sh $(1)/etc/failover/
	install -m0755 ./files/failover-test $(1)/etc/failover/
	install -m0755 ./files/failover.common $(1)/etc/failover/
	install -m0755 ./files/failover_dhcpc.script $(1)/etc/failover/
	install -m0755 ./files/net-* $(1)/etc/failover/
	install -m0755 $(PKG_BUILD_DIR)/failover $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,failover))
