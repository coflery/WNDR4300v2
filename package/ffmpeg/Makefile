# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=ffmpeg
PKG_VERSION:=0.5
PKG_RELEASE:=1


PKG_BUILD_DIR:= $(BUILD_DIR)/ffmpeg
PKG_FFMPEG_GIT_TREEISH:=cf98fdb8cb9b26f6bc478ced0b584d7d638599b5
PKG_SWSCALE_GIT_TREEISH:=80907f266db5d8960f91a8ec6da9ff942e999789

include $(INCLUDE_DIR)/package.mk

define Package/ffmpeg
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=ffmpeg
  URL:=http://ffmpeg.org/
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); mkdir -p ipkg-install; \
		CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
		CXXFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
		CPPFLAGS="$(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		PKG_CONFIG_PATH="$(STAGING_DIR)/usr/lib/pkgconfig" \
		PKG_CONFIG_LIBDIR="$(STAGING_DIR)/usr/lib/pkgconfig" \
		./configure \
		--prefix=$(PKG_INSTALL_DIR) \
		--enable-cross-compile \
		--enable-shared \
		--enable-gpl \
		--target-os=linux \
		--arch=mips \
		--disable-altivec \
		--disable-muxers \
		--disable-encoders \
		--disable-filters \
		--disable-vhook \
		--disable-ffmpeg \
		--disable-ffserver \
		--disable-ffplay \
		--disable-bsfs \
		--disable-devices \
		--disable-armv5te \
		--disable-armv6 \
		--disable-vis \
		--disable-protocol=udp \
		--disable-protocol=tcp \
		--disable-protocol=rtp \
		--disable-protocol=pipe \
		--disable-protocol=http \
		--disable-mpegaudio-hp \
		--enable-demuxer=matroska \
		--cc=$(TOOLCHAIN_DIR)/usr/bin/mips-openwrt-linux-uclibc-gcc;)
endef

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
	CXXFLAGS="$(TARGET_CFLAGS) $(EXTRA_CFLAGS)" \
	LDFLAGS="$(EXTRA_LDFLAGS) " \
	$(MAKE) -C $(PKG_BUILD_DIR)/ \
	$(TARGET_CONFIGURE_OPTS) \
	CROSS="$(TARGET_CROSS)" \
	ARCH="$(ARCH)" \
	all install
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/include/* $(1)/usr/include/
	mkdir -p $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/lib/libav*.so* $(1)/usr/lib/
	mkdir -p $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/lib/pkgconfig/*.pc $(1)/usr/lib/pkgconfig/
endef

define Package/ffmpeg/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/lib/libavcodec.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/lib/libavformat.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/lib/libavutil.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,ffmpeg))
