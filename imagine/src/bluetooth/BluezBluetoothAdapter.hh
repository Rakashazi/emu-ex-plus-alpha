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

#include <engine-globals.h>
#include "BluetoothAdapter.hh"
#include <util/thread/pthread.hh>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <base/Base.hh>
#include <util/collection/DLList.hh>

class BluetoothPendingSocket
{
public:
	constexpr BluetoothPendingSocket() {}
	void close();
	uint channel() { return addr.l2_psm; }
	void requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName);

	operator bool() const
	{
		return fd != -1;
	}

	int fd = -1;
	sockaddr_l2 addr {};
};

class BluezBluetoothAdapter : public BluetoothAdapter
{
public:
	static BluezBluetoothAdapter *defaultAdapter();
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) override;
	void cancelScan() override;
	void close() override;
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint psm, bool active, OnStatusDelegate onResult) override;
	//bool l2capServiceRegistered(uint psm) override;
	#endif
	void requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName);
	State state() override;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) override;
private:
	int devId = -1, socket = -1;
private:
	ThreadPThread runThread;
	bool scanCancelled = 0;
	#ifdef CONFIG_BLUETOOTH_SERVER
	struct L2CapServer
	{
		constexpr L2CapServer() {}
		constexpr L2CapServer(uint psm, int fd, Base::PollEventDelegate onConnect): psm(psm), fd(fd), onConnect(onConnect) {}
		uint psm = 0;
		int fd = -1;
		Base::PollEventDelegate onConnect;
	};
	StaticDLList<L2CapServer, 2> serverList;
	#endif
	bool openDefault();
	CallResult doScan(const OnScanDeviceClassDelegate &onDeviceClass, const OnScanDeviceNameDelegate &onDeviceName);
};

class BluezBluetoothSocket : public BluetoothSocket
{
public:
	BluezBluetoothSocket() { }
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	#ifdef CONFIG_BLUETOOTH_SERVER
	CallResult open(BluetoothPendingSocket &socket) override;
	#endif
	void close() override;
	CallResult write(const void *data, size_t size) override;
	int readPendingData(int events);
private:
	Base::PollEventDelegate pollEvDel
	{
		[this](int events)
		{
			return readPendingData(events);
		}
	};
	int fd = -1;
};
