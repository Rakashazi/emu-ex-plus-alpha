ifndef inc_bluetooth_android
inc_bluetooth_android := 1

include $(imagineSrcDir)/bluetooth/bluetooth.mk

SRC += bluetooth/AndroidBluetoothAdapter.cc

endif
