ifndef inc_pkg_pulseaudio
inc_pkg_pulseaudio := 1

configEnable += CONFIG_PACKAGE_PULSEAUDIO

pkgConfigDeps += libpulse

endif