ifndef inc_bluetooth
inc_bluetooth := 1

configDefs += CONFIG_BLUETOOTH

SRC += bluetooth/BluetoothAdapter.cc bluetooth/BluetoothInputDevScanner.cc \
bluetooth/Wiimote.cc bluetooth/IControlPad.cc bluetooth/Zeemote.cc

endif