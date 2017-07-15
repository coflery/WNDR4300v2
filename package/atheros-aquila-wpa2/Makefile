# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 4855 2006-09-24 20:49:31Z nico $

include $(TOPDIR)/rules.mk

PKG_NAME:=atheros-aquila-wpa2
PKG_VERSION:=0.5.9
PKG_RELEASE:=1
PKG_GIT_TREEISH:=$(CONFIG_ATHEROS_AQUILA_WPA_GIT_TREEISH)
PKG_GIT_DIR:=hostap-0.5.x.git
PKG_NEW_GIT_DIR:=hostap.git

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

ifdef CONFIG_ATHEROS_AQUILA_WPA_ATHR_HOSTAP
  # Override git treeish if using Atheros hostap package (> 0.7.0)
  PKG_GIT_TREEISH=$(CONFIG_ATHEROS_AQUILA_WPA_GIT_TREEISH)
endif

define Package/atheros-aquila-wpa2/Default
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=kmod-atheros-aquila-madwifi
  TITLE:=Atheros WPA2 package
  URL:=http://hostap.epitest.fi/
endef

define Package/atheros-aquila-wpa2
  $(call Package/atheros-aquila-wpa2/Default)
  TITLE+= (hostapd + library)
endef

define Package/atheros-aquila-wpa2-wpatalk
  $(call Package/atheros-aquila-wpa2/Default)
  DEPENDS:=@PACKAGE_atheros-aquila-wpa2||PACKAGE_atheros-aquila-wpa2-mini
  TITLE+= (wpatalk)
endef

ifdef CONFIG_ATHEROS_AQUILA_SCRIPT_READONLY_FS
  ATH_SUFFIX=.orig
endif

ifdef CONFIG_ATHEROS_AQUILA_WPA_HOSTAPD
  ATHR_HOSTAPD=1
endif

ifdef CONFIG_ATHEROS_AQUILA_WPA_WPASUPPLICANT
  ATHR_WPA_SUPP=1
endif

HOSTAPD_CFLAGS="-DNETGEAR_RANDOM_SSID -DNETGEAR_ROUTER_PIN_HACK -DNETGEAR_ANYSSID -DNETGEAR_WPS_AUTO_LOCK_DOWN -DNETGEAR_SINGLE_RADIO_SSID_PREFIX -DNETGEAR_WPS_UPNP_SETTING -DNTG_DUT_NOT_SUPPORT_WEP_OPEN_AUTH -DNTGR_WPS_SET_MIX_MODE_IF_UNCFG -DNO_OPTIONAL_OPTION_IN_WPS_IE -DNO_TRIM_TRAILING_WHITESPACE -DWPS_CONF_RELOAD_TIME -DWPS_DISABLE_IE -DDNI_HOTPLUG_CMD -DDNI_WPS_CANCEL -DWPS_GENERATE_RANDOM_PASSPHRASE -DWPS_GENERATE_UUID -DDNI_DETECT_DUALBAND_MODE -DNTGR_DEL_M1_VIRT_DISPLAY"

define Package/atheros-aquila-wpa2/config
	menu "Configurations"
		depends on PACKAGE_atheros-aquila-wpa2
		source "$(SOURCE)/Config.in"
	endmenu
endef

TARGET_CPPFLAGS := \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(PKG_BUILD_DIR)/src/crypto \
	$(TARGET_CPPFLAGS) \
	-I$(STAGING_DIR)/usr/include/madwifi \
	-I$(STAGING_DIR)/usr/include/mac80211 \
	-DCONFIG_LIBNL20 \
	-D_GNU_SOURCE

ifdef CONFIG_ATHEROS_AQUILA_WPA_ATHR_HOSTAP
  define Build/Configure/rebuild
  endef

  define Build/Configure
	$(Build/Configure/rebuild)
	$(CP) ./files/athr_hostapd.conf $(PKG_BUILD_DIR)/hostapd/.config
	$(CP) ./files/athr_supplicant.conf $(PKG_BUILD_DIR)/wpa_supplicant/.config
  endef

  define Build/Compile
  endef

  define Package/atheros-aquila-wpa2/install
	if [ \! -f "$(PKG_BUILD_DIR)/hostapd" ]; then \
		rm -f $(PKG_BUILD_DIR)/.built; \
		touch $(PKG_BUILD_DIR)/.built; \
	fi
	install -m0755 -d $(1)/usr/sbin
	if [ "$(ATHR_HOSTAPD)" = "1" ]; then \
		install -m0755 $(PKG_BUILD_DIR)/hostapd/{hostapd,hostapd_cli} $(1)/usr/sbin/; \
	fi
	if [ "$(ATHR_WPA_SUPP)" = "1" ]; then \
		install -m0755 $(PKG_BUILD_DIR)/wpa_supplicant/{wpa_supplicant,wpa_cli} $(1)/usr/sbin/; \
	fi
	install -m0755 -d $(1)/etc/wpa2
	if [ -n "$(ATH_SUFFIX)" ]; then \
		mv $(1)/etc/wpa2 $(1)/etc/wpa2$(ATH_SUFFIX); \
		ln -sf /tmp/etc/wpa2 $(1)/etc/wpa2; \
	fi
  endef
else
  define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/common \
		$(TARGET_CONFIGURE_OPTS) \
		EXTRA_CFLAGS=$(HOSTAPD_CFLAGS) \
		OPTFLAGS="$(TARGET_CFLAGS)" \
		CPPFLAGS="-I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include" \
		LDFLAGS="-L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/lib" \
		ATHEROSPATH="$(STAGING_DIR)/usr/include/madwifi" \
		CONFIG_ETHERSTA=y \
		BUILD_OPTIMIZED=1 \
		INSTALL_ROOT="$(STAGING_DIR)" \
		all install
	$(MAKE) -C $(PKG_BUILD_DIR)/hostapd \
		$(TARGET_CONFIGURE_OPTS) \
		EXTRA_CFLAGS=$(HOSTAPD_CFLAGS) \
		OPTFLAGS="$(TARGET_CFLAGS)" \
		CPPFLAGS="-I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include" \
		LDFLAGS="-L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/lib" \
		ATHEROSPATH="$(STAGING_DIR)/usr/include/madwifi" \
		CONFIG_ETHERSTA=y \
		BUILD_OPTIMIZED=1 \
		INSTALL_ROOT="$(STAGING_DIR)" \
		hostapd
  endef

  define Package/atheros-aquila-wpa2/install
	if [ \! -f "$(PKG_BUILD_DIR)/hostapd" ]; then \
		rm -f $(PKG_BUILD_DIR)/.built; \
		touch $(PKG_BUILD_DIR)/.built; \
	fi
	install -m0755 -d $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/hostapd/hostapd $(1)/usr/sbin/
	install -m0755 -d $(1)/lib
	# install -m0755 $(PKG_BUILD_DIR)/common/libwpa_common.a $(1)/lib
	install -m0755 $(PKG_BUILD_DIR)/common/libwpa_common.so $(1)/lib
	# install -m0755 $(PKG_BUILD_DIR)/common/libwpa_ctrl.a $(1)/lib
	install -m0755 $(PKG_BUILD_DIR)/common/libwpa_ctrl.so $(1)/lib
	install -m0755 -d $(1)/etc/wpa2
	install -m0644 $(PKG_BUILD_DIR)/examples/*.conf $(1)/etc/wpa2
	install -m0644 $(PKG_BUILD_DIR)/examples/*.eap_user $(1)/etc/wpa2
	install -m0644 $(PKG_BUILD_DIR)/examples/*.ap_radio $(1)/etc/wpa2
	install -m0644 $(PKG_BUILD_DIR)/examples/*.ap_bss $(1)/etc/wpa2
	install -m0644 $(PKG_BUILD_DIR)/examples/*.sta $(1)/etc/wpa2
	if [ -n "$(ATH_SUFFIX)" ]; then \
		mv $(1)/etc/wpa2 $(1)/etc/wpa2$(ATH_SUFFIX); \
		ln -sf /tmp/etc/wpa2 $(1)/etc/wpa2; \
	fi
  endef
endif

define Build/Prepare
	$(CP) $(GIT_HOME)/hostap-0.5.x.git/* $(PKG_BUILD_DIR)/
endef

define Package/atheros-aquila-wpa2-wpatalk/install
	install -m0755 -d $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/wpatalk/wpatalk $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,atheros-aquila-wpa2))
$(eval $(call BuildPackage,atheros-aquila-wpa2-wpatalk))
