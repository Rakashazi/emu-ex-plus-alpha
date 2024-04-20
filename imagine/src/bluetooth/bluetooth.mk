ifndef inc_bluetooth
inc_bluetooth := 1

SRC += bluetooth/BluetoothInputDevScanner.cc \
bluetooth/Wiimote.cc bluetooth/IControlPad.cc bluetooth/Zeemote.cc

ifeq ($(ENV), linux)
 bluetooth_ps3Controller := 1
endif
ifeq ($(ENV), ios)
 bluetooth_ps3Controller := 1
endif
ifeq ($(bluetooth_ps3Controller), 1)
 SRC += bluetooth/PS3Controller.cc
endif

endif