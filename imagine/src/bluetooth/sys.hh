#pragma once

#include <config.h>

#if defined CONFIG_BLUETOOTH_ANDROID
	#include "AndroidBluetoothAdapter.hh"
	using BluetoothSocketSys = AndroidBluetoothSocket;
#elif defined CONFIG_BLUETOOTH_BTSTACK
	#include "BtstackBluetoothAdapter.hh"
	using BluetoothSocketSys = BtstackBluetoothSocket;
#elif defined CONFIG_BLUETOOTH_BLUEZ
	#include "BluezBluetoothAdapter.hh"
	using BluetoothSocketSys = BluezBluetoothSocket;
#endif
