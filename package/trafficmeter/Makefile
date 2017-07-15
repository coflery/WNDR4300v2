# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.2.2 2007/02/06 13:52:33 anderson Exp $

include $(TOPDIR)/rules.mk

PKG_GIT_TREEISH=$(CONFIG_TRAFFICMETER_GIT_TREEISH)

PKG_NAME:=traffic_meter
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
TMP_VAR:=`echo $(CONFIG_TRAFFIC_METER_PARTITION) | sed 's/.*\///'`

include $(INCLUDE_DIR)/package.mk

define Package/trafficmeter
  SECTION:=utils
  CATEGORY:=Base system
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=Update utility for trx firmware images
endef

define Package/trafficmeter/config
	menu "Configurations"
		depends on PACKAGE_trafficmeter
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/trafficmeter.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/trafficmeter/install
	install -d -m0755 $(1)/sbin
	install -d -m0755 $(1)/etc
	install -d -m0755 $(1)/etc/init.d
	install -d -m0755 $(1)/usr
	install -d -m0755 $(1)/usr/traffic_meter
	$(INSTALL_BIN) ./files/sbin/generate_traffic_meter_conf $(1)/sbin/generate_traffic_meter_conf
	$(CP) ./files/last_* $(1)/usr/traffic_meter/
	$(CP) ./files/network_status $(1)/usr/traffic_meter/
	$(INSTALL_BIN) ./files/traffic_meter $(1)/usr/traffic_meter/
	$(INSTALL_BIN) ./files/traffic_statistics $(1)/usr/traffic_meter/
	$(INSTALL_BIN) ./files/traffic_meter.init $(1)/etc/init.d/
	install -m0755 $(PKG_BUILD_DIR)/traffic_meter $(1)/sbin/
	install -m0755 $(PKG_BUILD_DIR)/cmd_traffic_meter $(1)/sbin/
	install -m0755 $(PKG_BUILD_DIR)/tfm_led $(1)/sbin/
endef

$(eval $(call BuildPackage,trafficmeter))
