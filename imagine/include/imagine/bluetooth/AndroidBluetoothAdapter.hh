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

#include <jni.h>
#include <semaphore.h>
#include <imagine/config/defs.hh>
#include "BluetoothAdapter.hh"
#include <imagine/base/EventLoop.hh>
#include <imagine/base/Error.hh>

struct SocketStatusMessage;

class AndroidBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr AndroidBluetoothAdapter() {}
	static AndroidBluetoothAdapter *defaultAdapter(Base::ApplicationContext);
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) final;
	void cancelScan() final;
	void close() final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint32_t psm, bool active) final;
	#endif
	State state() final;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) final;
	bool handleScanClass(uint32_t classInt);
	void handleScanName(JNIEnv *env, jstring name, jstring addr);
	void handleScanStatus(int status);
	void handleTurnOnResult(bool success);
	void sendSocketStatusMessage(const SocketStatusMessage &msg);
	jobject openSocket(JNIEnv *env, const char *addrStr, int channel, bool isL2cap);

private:
	jobject adapter{};
	OnStateChangeDelegate turnOnD;
	int statusPipe[2]{-1, -1};
	bool scanCancelled = false;

	bool openDefault(Base::ApplicationContext);
};

class AndroidBluetoothSocket : public BluetoothSocket
{
public:
	AndroidBluetoothSocket(Base::ApplicationContext ctx):ctx{ctx} {}
	IG::ErrorCode openL2cap(BluetoothAdapter &, BluetoothAddr, uint32_t psm) final;
	IG::ErrorCode openRfcomm(BluetoothAdapter &, BluetoothAddr, uint32_t channel) final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	IG::ErrorCode open(BluetoothAdapter &, BluetoothPendingSocket &socket) final;
	#endif
	void close() final;
	IG::ErrorCode write(const void *data, size_t size) final;
	void onStatusDelegateMessage(int arg);

private:
	jobject socket{}, outStream{};
	Base::ApplicationContext ctx{};
	sem_t connectSem;
	Base::FDEventSource fdSrc;
	int nativeFd = -1;
	uint32_t channel = 0;
	bool isClosing = false;
	bool isL2cap = false;
	bool isConnecting = false;
	char addrStr[18]{};

	IG::ErrorCode openSocket(BluetoothAdapter &, BluetoothAddr, uint32_t channel, bool l2cap);
	int readPendingData(int events);
};
