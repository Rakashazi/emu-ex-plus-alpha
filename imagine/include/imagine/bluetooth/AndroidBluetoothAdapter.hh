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
#include <imagine/base/EventLoop.hh>
#include <imagine/base/ApplicationContext.hh>
#include <jni.h>
#include <semaphore>
#include <system_error>

namespace IG
{

struct SocketStatusMessage;

class BluetoothPendingSocket {};

class AndroidBluetoothAdapter
{
public:
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint32_t psm, bool active);
	#endif
	bool handleScanClass(uint32_t classInt);
	void handleScanName(JNIEnv *env, jstring name, jstring addr);
	void handleScanStatus(BluetoothScanState status);
	void handleTurnOnResult(bool success);
	void sendSocketStatusMessage(const SocketStatusMessage &msg);
	jobject openSocket(JNIEnv *env, const char *addrStr, int channel, bool isL2cap);

protected:
	jobject adapter{};
	BTOnStateChangeDelegate turnOnD;
	int statusPipe[2]{-1, -1};
	bool scanCancelled{};
	bool inDetect{};
};

using BluetoothAdapterImpl = AndroidBluetoothAdapter;

class AndroidBluetoothSocket
{
public:
	constexpr AndroidBluetoothSocket() = default;
	AndroidBluetoothSocket(ApplicationContext ctx):ctx{ctx} {}
	~AndroidBluetoothSocket();
	void close();
	void onStatusDelegateMessage(BluetoothAdapter&, BluetoothSocketState);

protected:
	jobject socket{}, outStream{};
	ApplicationContext ctx{};
	std::binary_semaphore connectSem{0};
	FDEventSource fdSrc{};
	int nativeFd = -1;
	uint32_t channel{};
	bool isClosing{};
	bool isL2cap{};
	bool isConnecting{};
	BluetoothAddrString addrStr{};

	void openSocket(BluetoothAdapter&, BluetoothAddr, uint32_t channel, bool l2cap);
	bool readPendingData(int events);
};

using BluetoothSocketImpl = AndroidBluetoothSocket;

}
