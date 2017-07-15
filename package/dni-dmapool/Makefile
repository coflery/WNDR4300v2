include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=dni-dmapool
PKG_VERSION:=
PKG_RELEASE:=

#PKG_SOURCE:=
#PKG_SOURCE_URL:=
#PKG_MD5SUM:=
#PKG_CAT:=zcat

PKG_GIT_TREEISH=$(CONFIG_DNIDMAPOOL_GIT_TREEISH)
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/dni-dmapool

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dni-dmapool
  SUBMENU:= 
  DEPENDS:=@TARGET_WNDR3700U||TARGET_wndr4500v3
  TITLE:=Module to pre-allocated dma pool
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(BOARD)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/dni-dmapool.$(LINUX_KMOD_SUFFIX)
  #AUTOLOAD:=$(call AutoLoad,40,dni-dmapool)
endef

define KernelPackage/dni-dmapool/config
	menu "Configurations"
		depends on PACKAGE_kmod-dni-dmapool
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/dni-dmapool.git/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
endef

define Build/InstallDev
endef

define Build/UninstallDev
endef

define KernelPackage/dni-dmapool/install
	mkdir -p $(1)/etc/init.d
	$(INSTALL_BIN) ./files/dni-dmapool.init $(1)/etc/init.d/dni-dmapool
endef

$(eval $(call KernelPackage,dni-dmapool))
