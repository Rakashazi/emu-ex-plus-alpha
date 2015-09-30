ifndef inc_pkg_libarchive
inc_pkg_libarchive := 1

include $(IMAGINE_PATH)/make/package/zlib.mk

pkgConfigStaticDeps += libarchive liblzma

endif