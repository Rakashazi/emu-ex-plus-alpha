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

#include <imagine/config/defs.hh>
#include "BluetoothAdapter.hh"
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <imagine/base/EventLoop.hh>
#include <imagine/base/Pipe.hh>
#include <imagine/base/Error.hh>
#ifdef CONFIG_BLUETOOTH_SERVER
#include <imagine/util/container/ArrayList.hh>
#endif

class BluetoothPendingSocket
{
public:
	int fd = -1;
	sockaddr_l2 addr {};

	constexpr BluetoothPendingSocket() {}
	void close();
	uint32_t channel() { return addr.l2_psm; }
	void requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName);

	explicit operator bool() const
	{
		return fd != -1;
	}
};

class BluezBluetoothAdapter : public BluetoothAdapter
{
public:
	BluezBluetoothAdapter() {}
	static BluezBluetoothAdapter *defaultAdapter(Base::ApplicationContext);
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) final;
	void cancelScan() final;
	void close() final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint32_t psm, bool active, OnStatusDelegate onResult) final;
	//bool l2capServiceRegistered(uint32_t psm) final;
	#endif
	void requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName);
	State state() final;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) final;

private:
	int devId = -1, socket = -1;
	Base::Pipe statusPipe{"BluezBluetoothAdapter::statusPipe"};
	bool scanCancelled = false;
	#ifdef CONFIG_BLUETOOTH_SERVER
	struct L2CapServer
	{
		L2CapServer() {}
		L2CapServer(uint32_t psm, int fd): psm(psm), fd(fd) {}
		uint32_t psm = 0;
		int fd = -1;
		Base::FDEventSource connectSrc;
	};
	StaticArrayList<L2CapServer, 2> serverList;
	#endif

	bool openDefault();
	IG::ErrorCode doScan(const OnScanDeviceClassDelegate &onDeviceClass, const OnScanDeviceNameDelegate &onDeviceName);
	void sendBTScanStatusDelegate(uint8_t type, uint8_t arg);
};

class BluezBluetoothSocket : public BluetoothSocket
{
public:
	BluezBluetoothSocket(Base::ApplicationContext) {}
	IG::ErrorCode openL2cap(BluetoothAdapter &, BluetoothAddr, uint32_t psm) final;
	IG::ErrorCode openRfcomm(BluetoothAdapter &, BluetoothAddr, uint32_t channel) final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	IG::ErrorCode open(BluetoothAdapter &, BluetoothPendingSocket &) final;
	#endif
	void close() final;
	IG::ErrorCode write(const void *data, size_t size) final;
	int readPendingData(int events);

private:
	Base::FDEventSource fdSrc;
	int fd = -1;
	void setupFDEvents(int events);
};
