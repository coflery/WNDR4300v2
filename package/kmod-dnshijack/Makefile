#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 6565 2007-03-14 13:42:21Z nbd $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-dnshijack
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_GIT_TREEISH:=$(CONFIG_DNSHIJACK_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dnshijack
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=DNI DNS Hijack firewall module
  VERSION:=$(LINUX_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/ipt_dnshijack.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,46,ipt_dnshijack)
endef

define KernelPackage/dnshijack/config
	menu "Configurations"
		depends on PACKAGE_kmod-dnshijack
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-dnshijack.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define KernelPackage/dnshijack/install
	$(INSTALL_DIR) $(1)/lib/network/
endef

$(eval $(call KernelPackage,dnshijack))
