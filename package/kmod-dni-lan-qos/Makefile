include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-dni-lan-qos
PKG_VERSION:=
PKG_RELEASE:=

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/kmod-dni-lan-qos

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dni-lan-qos
  SUBMENU:=Other modules
  TITLE:=Module to support qos for kernel 2.6.31
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/dni-lan-qos.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,39,dni-lan-qos)
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
endef

define KernelPackage/dni-lan-qos/install
	mkdir -p $(1)/sbin
	install -m0755 $(PKG_BUILD_DIR)/tools/dni_lan_qos $(1)/sbin/
endef

$(eval $(call KernelPackage,dni-lan-qos))
