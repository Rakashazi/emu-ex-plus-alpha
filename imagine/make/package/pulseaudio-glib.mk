ifndef inc_pkg_pulseaudio_glib
inc_pkg_pulseaudio_glib := 1

configEnable += CONFIG_PACKAGE_PULSEAUDIO CONFIG_PACKAGE_PULSEAUDIO_GLIB

pkgConfigDeps += libpulse-mainloop-glib

endif