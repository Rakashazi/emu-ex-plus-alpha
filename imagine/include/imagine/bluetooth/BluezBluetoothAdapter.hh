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
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <imagine/base/EventLoop.hh>
#include <imagine/base/Pipe.hh>
#include <imagine/base/ApplicationContext.hh>
#ifdef CONFIG_BLUETOOTH_SERVER
#include <imagine/util/container/ArrayList.hh>
#endif

namespace IG
{

class BluetoothPendingSocket
{
public:
	int fd = -1;
	sockaddr_l2 addr{};

	constexpr BluetoothPendingSocket() = default;
	void close();
	uint32_t channel() { return addr.l2_psm; }
	void requestName(BluetoothAdapter&, BTOnScanDeviceNameDelegate);

	explicit operator bool() const
	{
		return fd != -1;
	}
};

class BluezBluetoothAdapter
{
protected:
	int devId = -1, socket = -1;
	Pipe statusPipe{"BluezBluetoothAdapter::statusPipe"};
	bool scanCancelled{};
	bool inDetect{};
	#ifdef CONFIG_BLUETOOTH_SERVER
	struct L2CapServer
	{
		L2CapServer() = default;
		L2CapServer(uint32_t psm, int fd): psm(psm), fd(fd) {}
		uint32_t psm = 0;
		int fd = -1;
		FDEventSource connectSrc{};
	};
	StaticArrayList<L2CapServer, 2> serverList;
	#endif

	bool doScan(const BTOnScanDeviceClassDelegate&, const BTOnScanDeviceNameDelegate&);
	void sendBTScanStatusDelegate(BluetoothScanState, uint8_t arg);
};

using BluetoothAdapterImpl = BluezBluetoothAdapter;

class BluezBluetoothSocket
{
public:
	BluezBluetoothSocket(ApplicationContext) {}
	~BluezBluetoothSocket();
	void close();
	bool readPendingData(PollEventFlags);

protected:
	FDEventSource fdSrc{};
	int fd = -1;
	void setupFDEvents(PollEventFlags);
};

using BluetoothSocketImpl = BluezBluetoothSocket;

}
