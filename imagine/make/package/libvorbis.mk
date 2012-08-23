ifndef inc_pkg_libvorbis
inc_pkg_libvorbis := 1

ifdef noDoubleFloat
 include $(IMAGINE_PATH)/make/package/tremor.mk
else
 configDefs += CONFIG_PACKAGE_LIBVORBIS
 ifneq ($(ENV), linux)
  CPPFLAGS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include pkg-config vorbisfile --cflags --static --define-variable=prefix=$(system_externalSysroot))
  LDLIBS += $(shell PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config vorbisfile --libs --static --define-variable=prefix=$(system_externalSysroot))
 else
  CPPFLAGS += $(shell pkg-config vorbisfile --cflags)
  LDLIBS += $(shell pkg-config vorbisfile --libs)
 endif
endif

endif