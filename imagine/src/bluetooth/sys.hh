#pragma once

#include <config.h>

#ifdef CONFIG_BTSTACK
	#include "BtstackBluetoothAdapter.hh"
	#define BluetoothAdapterSys BtstackBluetoothAdapter
	#define BluetoothSocketSys BtstackBluetoothSocket
#else
	#include "BluezBluetoothAdapter.hh"
	#define BluetoothAdapterSys BluezBluetoothAdapter
	#define BluetoothSocketSys BluezBluetoothSocket
#endif
