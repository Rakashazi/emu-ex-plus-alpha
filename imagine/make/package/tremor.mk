ifndef inc_pkg_tremor
inc_pkg_tremor := 1

configDefs += CONFIG_PACKAGE_TREMOR
LDLIBS += -lvorbisidec -logg

endif