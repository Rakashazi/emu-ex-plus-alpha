#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/bluetooth/defs.hh>

#if defined CONFIG_BLUETOOTH_ANDROID
#include "AndroidBluetoothAdapter.hh"
#elif defined CONFIG_BLUETOOTH_BTSTACK
#include "BtstackBluetoothAdapter.hh"
#elif defined CONFIG_BLUETOOTH_BLUEZ
#include "BluezBluetoothAdapter.hh"
#endif

#include <imagine/util/DelegateFunc.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/used.hh>
#include <system_error>

namespace IG
{

class BluetoothAdapter : public BluetoothAdapterImpl
{
public:
	using State = BluetoothState;
	using ScanState = BluetoothScanState;
	using OnStateChangeDelegate = BTOnStateChangeDelegate;
	using OnStatusDelegate = BTOnStatusDelegate;
	using OnScanDeviceClassDelegate = BTOnScanDeviceClassDelegate;
	using OnScanDeviceNameDelegate = BTOnScanDeviceNameDelegate;
	using OnIncomingL2capConnectionDelegate = BTOnIncomingL2capConnectionDelegate;

	OnStatusDelegate onScanStatus;
	OnScanDeviceClassDelegate onScanDeviceClass;
	OnScanDeviceNameDelegate onScanDeviceName;
	ConditionalMember<Config::Bluetooth::server, OnIncomingL2capConnectionDelegate> onIncomingL2capConnection;
	ConditionalMember<Config::Bluetooth::scanTime, int> scanSecs{4};
	ConditionalMember<Config::Bluetooth::scanCache, bool> useScanCache{true};

	BluetoothAdapter(ApplicationContext ctx):ctx{ctx} {}
	bool openDefault();
	bool isOpen() const;
	bool startScan(OnStatusDelegate, OnScanDeviceClassDelegate, OnScanDeviceNameDelegate);
	void cancelScan();
	bool isInScan() const;
	void close();
	State state();
	void setActiveState(bool on, OnStateChangeDelegate);
	void setL2capService(uint32_t psm, bool active, OnStatusDelegate);
	void requestName(BluetoothPendingSocket&, OnScanDeviceNameDelegate);
	ApplicationContext appContext() const { return ctx; }

protected:
	ApplicationContext ctx{};
};

class BluetoothSocket : public BluetoothSocketImpl
{
public:
	using OnDataDelegate = DelegateFunc<bool (const char* data, size_t size)>;
	using OnStatusDelegate = DelegateFunc<uint32_t (BluetoothSocket&, BluetoothSocketState)>;
	using State = BluetoothSocketState;

	OnDataDelegate onData;
	OnStatusDelegate onStatus;

	using BluetoothSocketImpl::BluetoothSocketImpl;
	std::system_error openL2cap(BluetoothAdapter&, BluetoothAddr, uint32_t psm);
	std::system_error openRfcomm(BluetoothAdapter&, BluetoothAddr, uint32_t channel);
	std::system_error open(BluetoothAdapter&, BluetoothPendingSocket& socket);
	ssize_t write(const void* data, size_t size);
};

}
