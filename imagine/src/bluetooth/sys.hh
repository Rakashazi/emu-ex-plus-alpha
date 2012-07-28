#pragma once

#include <config.h>

#if defined CONFIG_BLUEZ && defined CONFIG_ANDROIDBT
	#include "BluezBluetoothAdapter.hh"
	#include "AndroidBluetoothAdapter.hh"
	#include "util/number.h"
	class BluetoothSocketSys
	{
	public:
		constexpr BluetoothSocketSys() { }
		uchar ATTRS(aligned (4)) obj[IG::max(sizeof(BluezBluetoothSocket), sizeof(AndroidBluetoothSocket))] {0};
		BluetoothSocket *sock = (BluetoothSocket*)obj;
		CallResult openL2cap(BluetoothAddr addr, uint psm) { return sock->openL2cap(addr, psm); }
		CallResult openRfcomm(BluetoothAddr addr, uint channel) { return sock->openRfcomm(addr, channel); }
		void close() { sock->close(); }
		CallResult write(const void *data, size_t size) { return sock->write(data, size); }
		BluetoothSocket::OnDataDelegate &onDataDelegate() { return sock->onDataDelegate(); }
		BluetoothSocket::OnStatusDelegate &onStatusDelegate() { return sock->onStatusDelegate(); }
	};
#elif defined CONFIG_BTSTACK
	#include "BtstackBluetoothAdapter.hh"
	#define BluetoothSocketSys BtstackBluetoothSocket
#elif defined CONFIG_BLUEZ
	#include "BluezBluetoothAdapter.hh"
	#define BluetoothSocketSys BluezBluetoothSocket
#else
	#include "AndroidBluetoothAdapter.hh"
	#define BluetoothSocketSys AndroidBluetoothSocket
#endif
