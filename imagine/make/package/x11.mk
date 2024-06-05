ifndef inc_pkg_x11
inc_pkg_x11 := 1

configEnable += CONFIG_PACKAGE_X11

pkgConfigDeps += xcb xcb-xfixes xcb-xinput xcb-icccm xkbcommon xkbcommon-x11

endif