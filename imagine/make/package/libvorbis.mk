ifndef inc_pkg_libvorbis
inc_pkg_libvorbis := 1

ifdef noDoubleFloat
 include $(buildSysPath)/package/tremor.mk
else
 configDefs += CONFIG_PACKAGE_LIBVORBIS
 pkgConfigDeps += vorbisfile
endif

endif