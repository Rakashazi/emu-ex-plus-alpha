ifndef inc_pkg_emuframework
inc_pkg_emuframework := 1

ifdef RELEASE
 pkgConfigDeps += emuframework
else
 pkgConfigDeps += emuframework-debug
endif

endif
