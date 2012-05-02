ifndef inc_pkg_bluez
inc_pkg_bluez := 1

ifneq ($(ENV), android)
LDLIBS += -lbluetooth
endif

configDefs += CONFIG_BLUEZ

endif