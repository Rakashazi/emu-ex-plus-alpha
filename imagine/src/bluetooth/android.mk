ifndef inc_bluetooth_android
inc_bluetooth_android := 1

include $(imagineSrcDir)/bluetooth/bluetooth.mk

configDefs += CONFIG_BLUETOOTH_ANDROID

SRC += bluetooth/AndroidBluetoothAdapter.cc

endif
