include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-dni-qos
PKG_VERSION:=
PKG_RELEASE:=

PKG_SOURCE:=
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_CAT:=zcat

PKG_GIT_TREEISH=$(CONFIG_DNI_QOS_GIT_TREEISH)
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/kmod-dni-qos

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dni-qos/config
	menu "Configurations"
		depends on PACKAGE_kmod-dni-qos
		source "$(SOURCE)/Config.in"
	endmenu
endef

define KernelPackage/dni-qos
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  TITLE:=Module to support qos for kernel 2.6.31
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/dni-qos.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,39,dni-qos)
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-dni-qos.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
endef

define KernelPackage/dni-qos/install
	#mkdir -p $(1)/etc/init.d
	#$(INSTALL_BIN) ./files/dni-qos.init $(1)/etc/init.d/dni-qos
	mkdir -p $(1)/sbin
	install -m0755 $(PKG_BUILD_DIR)/tools/dni_qos $(1)/sbin/
endef

$(eval $(call KernelPackage,dni-qos))
