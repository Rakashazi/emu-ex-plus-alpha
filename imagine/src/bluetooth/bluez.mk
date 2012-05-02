ifndef inc_bluetooth_bluez
inc_bluetooth_bluez := 1

include $(IMAGINE_PATH)/make/package/bluez.mk

configDefs += CONFIG_BLUETOOTH CONFIG_BLUEZ

SRC += bluetooth/BluezBluetoothAdapter.cc bluetooth/BluetoothInputDevScanner.cc \
bluetooth/Wiimote.cc bluetooth/IControlPad.cc bluetooth/Zeemote.cc

endif
