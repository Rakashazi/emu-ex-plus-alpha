ifndef inc_pkg_bluez
inc_pkg_bluez := 1

ifneq ($(ENV), android)
LDLIBS += -lbluetooth
else
# use dlopen wrapper
SRC += bluetooth/bluezDl.cc
endif

configDefs += CONFIG_BLUEZ

endif