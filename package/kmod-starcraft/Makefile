# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-starcraft
PKG_VERSION:=
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_GIT_TREEISH:=$(CONFIG_DNISTARCRAFT_ALG_GIT_TREEISH)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dnistarcraft-alg
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=Starcraft ALG linux kernel module for 2.6.31
  VERSION:=$(LINUX_VERSION)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/nf_nat_STARCRAFT.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,45,nf_nat_STARCRAFT)
endef

define KernelPackage/dnistarcraft-alg/config
	menu "Configurations"
		depends on PACKAGE_kmod-dnistarcraft-alg
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-starcraft.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

$(eval $(call KernelPackage,dnistarcraft-alg))
