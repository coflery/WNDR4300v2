# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-ipsec-alg
PKG_RELEASE:=1
ifeq ($(LINUX_VERSION),2.6.15)
  PKG_GIT_TREEISH:=$(CONFIG_IPSEC_ALG_GIT_TREEISH)
  NF=ip
else
  PKG_GIT_TREEISH:=$(CONFIG_IPSEC_ALG_GIT_TREEISH)
  NF=nf
endif
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ipsec-alg
  SUBMENU:=DNI Kernel Module
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=DNI IPSec ESP ALG
  VERSION:=$(LINUX_VERSION)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/$(NF)_conntrack_proto_esp.$(LINUX_KMOD_SUFFIX)	\
	$(PKG_BUILD_DIR)/$(NF)_nat_proto_esp.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,46,$(NF)_conntrack_proto_esp $(NF)_nat_proto_esp)
endef

define KernelPackage/ipsec-alg/config
	menu "Configurations"
		depends on PACKAGE_kmod-ipsec-alg
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-ipsec-alg.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(BUILDFLAGS)" \
		modules
endef

$(eval $(call KernelPackage,ipsec-alg))
