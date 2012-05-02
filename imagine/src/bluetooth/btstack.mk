ifndef inc_bluetooth_btstack
inc_bluetooth_btstack := 1

ifndef iOSAppStore

include $(IMAGINE_PATH)/make/package/btstack.mk

configDefs += CONFIG_BLUETOOTH CONFIG_BTSTACK

SRC += bluetooth/BtstackBluetoothAdapter.cc bluetooth/BluetoothInputDevScanner.cc \
bluetooth/Wiimote.cc bluetooth/IControlPad.cc bluetooth/Zeemote.cc

endif

endif
