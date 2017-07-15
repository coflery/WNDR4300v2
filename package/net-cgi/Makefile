# 
# Copyright (C) 2009 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=net-cgi
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

WEB_PATH:=www
TMP_VAR1:=`echo $(CONFIG_LANGUAGE_PARTITION) | sed 's/.*\///'`
TMP_VAR2:=`echo $(CONFIG_POT_PARTITION) | sed 's/.*\///'`
TMP_VAR3:=`echo $(CONFIG_BOOTLOADER_PARTITION) | sed 's/.*\///'`
TMP_VAR4:=`echo $(CONFIG_FIRMWARE_PARTITION) | sed 's/.*\///'`

include $(INCLUDE_DIR)/package.mk
include ./files/net-cgi.config

PKG_GIT_TREEISH:=$(CONFIG_NET_CGI_GIT_TREEISH)

define Package/net-cgi
  SECTION:=utils
  CATEGORY:=Base system
  DEFAULT:=y
  TITLE:=The mini_httpd CGI interface
endef

define Package/net-cgi/config
	menu "Configurations"
		depends on PACKAGE_net-cgi
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/net-cgi.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Package/net-cgi/install
	install -d -m0755 $(1)/usr/sbin
	#install -m0755 $(PKG_BUILD_DIR)/support_soap_list $(TOPDIR)/bin/
	install -m0755 $(PKG_BUILD_DIR)/net-cgi $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/inetd $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/files/wget_netgear $(1)/usr/sbin/
ifeq ($(HAVE_USB_STORAGE), 1)
	install -m0755 $(PKG_BUILD_DIR)/usb_cfg $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/files/select_partition $(1)/usr/sbin/
endif
ifeq ($(HAVE_PLC), 1)
	install -m0755 $(PKG_BUILD_DIR)/detplc $(1)/usr/sbin/
endif
	install -d -m0755 $(1)/$(WEB_PATH)
	install -d -m0755 $(1)/$(WEB_PATH)/image
	install -d -m0755 $(1)/$(WEB_PATH)/style
	install -d -m0755 $(1)/$(WEB_PATH)/script
	install -d -m0755 $(1)/$(WEB_PATH)/help
	install -d -m0755 $(1)/$(WEB_PATH)/cgi-bin
	install -d -m0755 $(1)/$(WEB_PATH)/language
	install -m0644 $(PKG_BUILD_DIR)/$(WEB_PATH)/*.* $(1)/$(WEB_PATH)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/image/* $(1)/$(WEB_PATH)/image
	install -m0644 $(PKG_BUILD_DIR)/$(WEB_PATH)/style/*.* $(1)/$(WEB_PATH)/style
	install -m0644 $(PKG_BUILD_DIR)/$(WEB_PATH)/script/*.* $(1)/$(WEB_PATH)/script
	install -m0644 $(PKG_BUILD_DIR)/$(WEB_PATH)/help/*.* $(1)/$(WEB_PATH)/help
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/cgi-bin/*.* $(1)/$(WEB_PATH)/cgi-bin
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/language/*.* $(1)/$(WEB_PATH)/language

ifeq ($(READYSHARE_REMOTE), 1)
	install -m0755 $(PKG_BUILD_DIR)/remote_smb_conf $(1)/usr/sbin/
endif
endef

$(eval $(call BuildPackage,net-cgi))
