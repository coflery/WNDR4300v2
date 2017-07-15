# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
PKG_NAME:=atheros-aquila-madwifi-11n-scripts
PKG_VERSION:=0
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH=$(CONFIG_ATHEROS_AQUILA_SCRIPT_GIT_TREEISH)

PKG_BUILD_DEPENDS:=atheros-aquila-madwifi

include $(INCLUDE_DIR)/package.mk

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_TYPE_DNI
  BOARD_TYPE=dni
else
ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_TYPE_SINGLE
  BOARD_TYPE=single
else
  BOARD_TYPE=dual
endif
endif

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_DNI_CONFIG
  SCRIPT_CONFIG_SOURCE=DNI_CONFIG
endif

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_OPENWRT_CONFIG
  SCRIPT_CONFIG_SOURCE=OPENWRT_CONFIG
endif

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_READONLY_FS
  ATH_SUFFIX=.orig
endif

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_IPTV_SUPPORT
  IPTV_SUPPORT=1
endif

define Package/atheros-aquila-madwifi-11n-scripts
  TITLE:=Atheros Madwifi 11n scripts for Aquila (LSDK 9.2.x)
  DEPENDS:=kmod-atheros-aquila-madwifi
  SECTION:=net
  CATEGORY:=Network
endef

ifdef CONFIG_ATHEROS_AQUILA_PLATFORM_PATCH
  PLATFORM_PATCH_DIR:=./patches-$(BOARD)
endif

define Build/Prepare
	$(CP) $(GIT_HOME)/madwifi-11n-scripts.git/* $(PKG_BUILD_DIR)/
endef

define Package/atheros-aquila-madwifi-11n-scripts/config
	menu "Configurations"
		depends on PACKAGE_atheros-aquila-madwifi-11n-scripts
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Compile
endef

ifneq ($(CONFIG_TARGET_wndr4500v3)$(CONFIG_TARGET_wnr1000v4),)
  NUMBER_AHB=1
else
  NUMBER_AHB=0
endif

ifdef CONFIG_ATHEROS_AQUILA_WPA_ATHR_HOSTAP
  ATHR_HOSTAP_CONF=1
endif

define Package/atheros-aquila-madwifi-11n-scripts/install
	mkdir -p $(1)/etc/init.d
	mkdir -p $(1)/sbin
	mkdir -p $(1)/etc/ath$(ATH_SUFFIX)
	mkdir -p $(1)/etc/rc.d
	mkdir -p $(1)/lib/wifi
	mkdir -p $(1)/sbin
	mkdir -p $(1)
	$(CP) $(PKG_BUILD_DIR)/cgiCommon/etc/ath/* $(1)/etc/ath$(ATH_SUFFIX)/
	$(CP) $(PKG_BUILD_DIR)/cgiCommon/etc/ath.$(BOARD_TYPE)/* $(1)/etc/ath$(ATH_SUFFIX)/
	if [ "$(ATHR_HOSTAP_CONF)" = "1" ]; then \
		$(CP) $(PKG_BUILD_DIR)/cgiCommon/etc/ath/hostapd0.7.0_conf/* $(1)/etc/ath$(ATH_SUFFIX)/; \
	fi
	$(CP) $(PKG_BUILD_DIR)/cgiCommon/etc/rc.d/* $(1)/etc/rc.d/
	$(CP) $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/etc/ath/* $(1)/etc/ath$(ATH_SUFFIX)/
	chmod +x $(1)/etc/ath$(ATH_SUFFIX)/*
	if [ -f $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/etc/dni-wifi-config ]; then \
		$(CP) $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/etc/dni-wifi-config $(1)/etc/; \
		sed -i "s/^${SCRIPT_CONFIG_SOURCE}=.*/${SCRIPT_CONFIG_SOURCE}=1/" $(1)/etc/dni-wifi-config; \
	fi
	if [ -d $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/sbin ]; then \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/sbin/* $(1)/sbin; \
		ln -sf /tmp/sbin/dni_apup $(1)/sbin/dni_apup; \
	fi
	if [ -f $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/lib/wifi/atheros11n.sh ]; then \
		$(CP) $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/lib/wifi/atheros11n.sh $(1)/lib/wifi ; \
	fi
	if [ -d $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/usr/lib ]; then \
		mkdir -p $(1)/usr/lib; \
		$(CP) $(PKG_BUILD_DIR)/$(CONFIG_ATHEROS_AQUILA_SCRIPT_USE_BOARD)/usr/lib/* $(1)/usr/lib ; \
	fi
	if [ -f $(PKG_BUILD_DIR)/madwifi-11n-scripts.init ]; then \
		if [ -n $(CONFIG_CALDATA_PARTITION) ]; then \
			sed -i '/cal_block/s#/dev/caldata#$(call qstrip,$(CONFIG_CALDATA_PARTITION))#g' $(PKG_BUILD_DIR)/madwifi-11n-scripts.init; \
			sed -i "s#^CAL_BLOCK_NUM=#CAL_BLOCK_NUM=$(subst /dev/mtd,,$(CONFIG_CALDATA_PARTITION))#g" $(PKG_BUILD_DIR)/madwifi-11n-scripts.init; \
		fi; \
		$(INSTALL_BIN) $(PKG_BUILD_DIR)/madwifi-11n-scripts.init $(1)/etc/init.d/madwifi-11n-scripts; \
	fi
	if [ -f $(PKG_BUILD_DIR)/conf/sample.conf ]; then \
		$(CP) $(PKG_BUILD_DIR)/conf/sample.conf $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		sed -i 's/wsc_manufactuer=.*/wsc_manufactuer=$(CONFIG_ATHEROS_AQUILA_SCRIPT_WSC_MANUFACTURER)/g' \
			 $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		sed -i 's/wsc_model_name=.*/wsc_model_name=$(CONFIG_ATHEROS_AQUILA_SCRIPT_WSC_MODEL_NAME)/g' \
			$(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		sed -i 's/wsc_model_number=.*/wsc_model_number=$(CONFIG_ATHEROS_AQUILA_SCRIPT_WSC_MODEL_NUMBER)/g' \
			$(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		sed -i 's/WLAN_LED_CONTROL=0/WLAN_LED_CONTROL=1/g' $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		sed -i 's/ebtables_exist=.*/ebtables_exist="on"/g' $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		if [ -n "$(IPTV_SUPPORT)" ]; then \
			sed -i 's/WLAN_IPTV_SUPPORT=0/WLAN_IPTV_SUPPORT=$(IPTV_SUPPORT)/g' $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
		fi; \
		sed -i 's/revert_wds_activate_order=.*/revert_wds_activate_order="on"/g' $(1)/etc/ath$(ATH_SUFFIX)/board.conf; \
	fi
	sed -i 's/export NUMRADIO_AHB=0/export NUMRADIO_AHB=$(NUMBER_AHB)/g' $(1)/etc/ath$(ATH_SUFFIX)/apcfg
	if [ -n "$(ATH_SUFFIX)" ]; then \
		 ln -sf /tmp/etc/ath $(1)/etc/ath; \
	fi
endef

define Package/atheros-aquila-madwifi-11n-scripts/uninstall
	rm -rf $(STAGING_DIR)/sbin/update-wifi
	rm -rf $(STAGING_DIR)/etc/ath*
	rm -rf $(STAGING_DIR)/etc/rc.d/rc.wlan
endef

$(eval $(call BuildPackage,atheros-aquila-madwifi-11n-scripts))
