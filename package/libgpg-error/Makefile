#
# Copyright (C) 2005-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libgpg-error
PKG_VERSION:=1.10
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=ftp://ftp.gnupg.org/gcrypt/libgpg-error/ \
	http://mirrors.rootmode.com/ftp.gnupg.org/libgpg-error/ \
	http://gulus.usherbrooke.ca/pub/appl/GnuPG/libgpg-error/ \
	http://gnupg.unixmexico.org/ftp/libgpg-error/
PKG_MD5SUM:=736a03daa9dc5873047d4eb4a9c22a16
PKG_FIXUP:=libtool

include $(INCLUDE_DIR)/package.mk

PKG_INSTALL=1

define Package/libgpg-error
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=GnuPG error handling helper library
  URL:=http://www.gnupg.org/related_software/libgpg-error/
endef

define Package/libgpg-error/description
	An helper library for common error codes and descriptions.
	This is a library that defines common error values for all GnuPG 
	components. Among these are GPG, GPGSM, GPGME, GPG-Agent, libgcrypt, 
	Libksba, DirMngr, Pinentry, SmartCard Daemon and possibly more in the 
	future.
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static \
	--disable-rpath \

#define Build/Configure
#	$(call Build/Configure/Default, \
#		--enable-shared \
#		--enable-static \
#		--disable-rpath \
#	)
#endef

define Build/InstallDev
	echo "PARAM 1 = $1 PARAM 2 = $2 PARAM 3 = $3"
	mkdir -p $(STAGING_DIR)/usr/bin
	mkdir -p $(STAGING_DIR)/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/gpg-error-config $(STAGING_DIR)/bin/
	$(SED) \
		's,^\(prefix\|exec_prefix\)=.*,\1=$(STAGING_DIR)/target-powerpc_eglibc-2.13/usr,g' \
		$(STAGING_DIR)/bin/gpg-error-config
	ln -sf  $(STAGING_DIR)/bin/gpg-error-config $(STAGING_DIR)/usr/bin/gpg-error-config 

	mkdir -p $(STAGING_DIR)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/gpg-error.h $(STAGING_DIR)/usr/include/

	mkdir -p $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libgpg-error.{la,a,so*} $(STAGING_DIR)/usr/lib/

	mkdir -p $(STAGING_DIR)/usr/share/aclocal
	$(CP) $(PKG_INSTALL_DIR)/usr/share/aclocal/gpg-error.m4 $(STAGING_DIR)/usr/share/aclocal/
endef

define Package/libgpg-error/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		${PKG_INSTALL_DIR}/usr/lib/libgpg-error.so.* \
		$(1)/usr/lib/
	$(INSTALL_DIR) $(1)/bin
	$(CP) \
		${PKG_INSTALL_DIR}/usr/bin/gpg-error-config \
		$(1)/bin/
endef

$(eval $(call BuildPackage,libgpg-error))
#		$(PKG_INSTALL_DIR)/usr/bin/gpg-error-config \
#		$(PKG_INSTALL_DIR)/usr/include/gpg-error.h \
#		$(PKG_INSTALL_DIR)/usr/lib/libgpg-error.{la,a,so*} \
#		$(PKG_INSTALL_DIR)/usr/share/aclocal/gpg-error.m4 \
#		$(PKG_INSTALL_DIR)/usr/lib/libgpg-error.so.* \
