ifndef inc_pkg_libvorbis
inc_pkg_libvorbis := 1

configEnable += CONFIG_PACKAGE_LIBVORBIS
ifeq ($(ENV), linux)
 pkgConfigDeps += vorbisfile
else
 pkgConfigStaticDeps += vorbisfile
endif

endif