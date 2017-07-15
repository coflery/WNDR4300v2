# 
# Copyright (C) 2008 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=proftpd
PKG_VERSION:=1.3.3
PKG_RELEASE:=1

PKG_GIT_TREEISH:=$(CONFIG_PROFTPD_GIT_TREEISH)

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/proftpd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=FTP Daemon
  DEPENDS:=@TARGET_dniwndr3700u2631||TARGET_wndr4500v3
  URL:=http://www.proftpd.org/
endef

define Package/proftpd/config
	menu "Configurations"
		depends on PACKAGE_proftpd
		source "$(SOURCE)/Config.in"
	endmenu
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/proftpd.git/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR) ; \
		echo "ac_cv_func_setgrent_void=yes" > $(GNU_TARGET_NAME)-linux.cache ; \
		./configure \
			--host=$(GNU_TARGET_NAME) \
			--cache-file=$(GNU_TARGET_NAME)-linux.cache \
			--sysconfdir=/etc \
			--localstatedir=/var/run \
			--enable-shadow \
			--disable-cap \
			--disable-auth-pam \
			--disable-curses \
			--enable-largefile \
			--disable-ncurses ; \
		echo "#undef HAVE___PROGNAME" >> config.h ; \
		echo "#undef HAVE_UTMPX_H" >> config.h ; \
		echo "#undef HAVE_STROPTS_H" >> config.h ; \
		echo "#undef HAVE_SYS_ACL_H" >> config.h ; \
		echo "#undef HAVE_EXECINFO_H" >> config.h ; \
		echo "#undef HAVE_FUTIMES" >> config.h ; \
		echo "#define HAVE_NET_IF_H" >> config.h ; \
		echo "#define LAN_IFNAME \"br0\"" >> config.h ; \
		echo "#define USB_NET_ACCESS_LOG" >> config.h ; \
		echo "#define NETGEAR_GUEST_ACCESS" >> config.h ; \
		echo "#define _FILE_OFFSET_BITS  64" >> config.h ; \
		echo "#define _LARGE_FILES" >> config.h \
	);
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS="$(TARGET_CROSS)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(DEP_BUILD_DIR)/include -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/include -I./include" \
		LDFLAGS="-L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/lib -L./lib" \
		all
endef

define Package/proftpd/install
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/proftpd $(1)/usr/sbin/
	install -d -m0755 $(1)/sbin
	install -m0755 ./files/cmdftp $(1)/sbin/
endef

$(eval $(call BuildPackage,proftpd))
