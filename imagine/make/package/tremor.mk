ifndef inc_pkg_tremor
inc_pkg_tremor := 1

configEnable += CONFIG_PACKAGE_TREMOR
ifeq ($(ENV), linux)
 pkgConfigDeps += vorbisidec
else
 pkgConfigStaticDeps += vorbisidec
endif

endif