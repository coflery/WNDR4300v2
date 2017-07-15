#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 6565 2007-03-14 13:42:21Z nbd $

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-spidos
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_GIT_TREEISH:=$(CONFIG_SPIDOS_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/spidos
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=DNI SPI DOS firewall module
  VERSION:=$(LINUX_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/ipt_spiDoS.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,46,ipt_spiDoS)
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-spidos.git/* $(PKG_BUILD_DIR)/
endef

define KernelPackage/spidos/config
	menu "Configurations"
		depends on PACKAGE_kmod-spidos
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Compile
endef

define KernelPackage/spidos/install
	$(INSTALL_DIR) $(1)/lib/network/
endef

$(eval $(call KernelPackage,spidos))
