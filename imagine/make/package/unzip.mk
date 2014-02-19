ifndef inc_pkg_unzip
inc_pkg_unzip := 1

ifndef minizipStatic
 ifdef binStatic
  minizipStatic := 1
 endif
endif

ifeq ($(minizipStatic), 1)
 pkgConfigStaticDeps += minizip
else
 pkgConfigDeps += minizip
endif

endif