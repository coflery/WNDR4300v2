include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kcode
PKG_VERSION:=
PKG_RELEASE:=

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/kcode

include $(INCLUDE_DIR)/package.mk

define KernelPackage/kcode
  SUBMENU:= 
  TITLE:=KCode USB Printer
#  DESCRIPTION:=\
#	This package contains KCode USB Printer module
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/NetUSB.$(LINUX_KMOD_SUFFIX)  $(PKG_BUILD_DIR)/GPL_NetUSB.$(LINUX_KMOD_SUFFIX)
#  AUTOLOAD:=$(call AutoLoad,90,GPL_NetUSB)
#  AUTOLOAD:=$(call AutoLoad,91,NetUSB)
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

define KernelPackage/kcode/install
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/kcode.init $(1)/etc/init.d/kcode
endef

$(eval $(call KernelPackage,kcode))
