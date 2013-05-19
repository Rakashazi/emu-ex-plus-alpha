ifndef inc_bluetooth_btstack
inc_bluetooth_btstack := 1

ifndef iOSAppStore

include $(IMAGINE_PATH)/make/package/btstack.mk

include $(imagineSrcDir)/bluetooth/bluetooth.mk

configDefs += CONFIG_BLUETOOTH_BTSTACK

SRC += bluetooth/BtstackBluetoothAdapter.cc

endif

endif
