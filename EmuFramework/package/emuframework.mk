ifndef inc_pkg_emuframework
inc_pkg_emuframework := 1

ifndef RELEASE
 ifndef imagineLibExt
  imagineLibExt := -debug
 endif
endif

pkgConfigDeps += emuframework$(imagineLibExt)

endif
