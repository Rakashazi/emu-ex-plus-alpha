ifndef inc_pkg_imagine
inc_pkg_imagine := 1

ifndef RELEASE
 ifndef imagineLibExt
  imagineLibExt := -debug
 endif
endif

pkgConfigDeps += imagine$(imagineLibExt)

endif