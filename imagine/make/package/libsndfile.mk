ifndef inc_pkg_libsndfile
inc_pkg_libsndfile := 1

ifeq ($(ENV), linux)
 pkgConfigDeps += sndfile
else
 pkgConfigStaticDeps += sndfile
endif

endif