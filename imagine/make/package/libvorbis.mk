ifndef inc_pkg_libvorbis
inc_pkg_libvorbis := 1

ifndef vorbisLib
 ifeq ($(ARCH), arm)
  vorbisLib = tremor
 endif
endif

ifeq ($(vorbisLib), tremor)
 include $(buildSysPath)/package/tremor.mk
else
 configDefs += CONFIG_PACKAGE_LIBVORBIS
 ifeq ($(ENV), linux)
  pkgConfigDeps += vorbisfile
 else
  pkgConfigStaticDeps += vorbisfile
 endif
endif

endif