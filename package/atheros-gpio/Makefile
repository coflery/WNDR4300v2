include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=atheros-gpio
PKG_VERSION:=
PKG_RELEASE:=

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/atheros-gpio
  SUBMENU:=
  TITLE:=Driver for Atheros ar9344 GPIO
  URL:=http://www.atheros.com/
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/atherosgpio.$(LINUX_KMOD_SUFFIX)
  #AUTOLOAD:=$(call AutoLoad,40,atherosgpio)
endef

ENET_MAKEOPTS= \
                ENETDIR="$(PKG_BUILD_DIR)" \
                PATH="$(KERNEL_STAGING_DIR)/bin:$(TARGET_PATH)" \
                ARCH="$(LINUX_KARCH)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                TOOLPREFIX="mips-linux-" \
                TOOLPATH="mips-linux-"

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
endef

define KernelPackage/atheros-gpio/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/init-gpio $(1)/etc/init.d
endef

$(eval $(call KernelPackage,atheros-gpio))
