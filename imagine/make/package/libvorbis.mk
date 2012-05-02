ifndef inc_pkg_libvorbis
inc_pkg_libvorbis := 1

ifdef noDoubleFloat
include $(IMAGINE_PATH)/make/package/tremor.mk
else
configDefs += CONFIG_PACKAGE_LIBVORBIS
LDLIBS += -lvorbisfile -lvorbis -logg
endif

endif