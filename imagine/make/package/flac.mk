ifndef inc_pkg_flac
inc_pkg_flac := 1

ifeq ($(ENV), linux)
 pkgConfigDeps += flac
else
 pkgConfigStaticDeps += flac
endif

endif