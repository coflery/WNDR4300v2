# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 5196 2006-10-17 20:52:40Z nbd $
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
PKG_NAME:=nat2.5
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_GIT_TREEISH=$(CONFIG_NAT_GIT_TREEISH)
GIT_REPOSITORY=$(CONFIG_NAT_GIT_REPOSITORY)

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/nat2.5

include $(INCLUDE_DIR)/package.mk

define KernelPackage/nat2.5
  SUBMENU:=Other modules
  TITLE:=NAT 2.5 (client bridge)
  VERSION:=none
  FILES:= \
		$(PKG_BUILD_DIR)/clbr.$(LINUX_KMOD_SUFFIX)
endef

define KernelPackage/nat2.5/config
	menu "Configurations"
		depends on PACKAGE_kmod-nat2.5
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/dni-nat2.5.git/* $(PKG_BUILD_DIR)/
endef

MADWIFI_MAKEOPTS= PATH="$(KERNEL_STAGING_DIR)/bin:$(TARGET_PATH)" \
		ARCH="$(LINUX_KARCH)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		TARGET="$(HAL_TARGET)" \
		MADWIFITARGET=mipsisa32-be-elf \
		FUSIONTARGET=mipsisa32-be-elf \
		TOOLPREFIX="$(AR531X_KERNEL_CROSS)" \
		TOOLPATH="$(AR531X_KERNEL_CROSS)" \
		KERNELPATH="$(LINUX_DIR)"

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
	rm -rf	$(LINUX_DIR)/include/net/if_ath_clbr.h
	rm -rf	$(LINUX_DIR)/include/net/clbr_queue.h
endef

define KernelPackage/nat2.5/install
	mkdir -p $(1)/etc/init.d
	mkdir -p $(1)/lib/modules/$(LINUX_VERSION)
	install -m0755 ./clbr.init $(1)/etc/init.d/clbr
endef

$(eval $(call KernelPackage,nat2.5))
