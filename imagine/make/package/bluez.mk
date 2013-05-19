ifndef inc_pkg_bluez
inc_pkg_bluez := 1

ifneq ($(ENV), android)
 pkgConfigDeps += bluez
else
# use dlopen wrapper
SRC += bluetooth/bluezDl.cc
endif

configDefs += CONFIG_BLUEZ

endif