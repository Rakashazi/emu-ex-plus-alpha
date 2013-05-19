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
#include <util/jni.hh>
#include <base/Base.hh>
#include <base/android/private.hh>

class AndroidBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr AndroidBluetoothAdapter() { }
	static AndroidBluetoothAdapter *defaultAdapter();
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) override;
	void cancelScan() override;
	void close() override;
	#ifdef CONFIG_BLUETOOTH_SERVER
	void setL2capService(uint psm, bool active) override;
	#endif
	State state() override;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) override;
//private:
	bool openDefault();
	bool handleScanClass(int classInt);
	void handleScanName(JNIEnv* env, jstring name, jstring addr);
	void handleScanStatus(int status);
	void handleTurnOnResult(bool success);
	bool scanCancelled = 0;
	jobject adapter = nullptr;
	OnStateChangeDelegate turnOnD;
};

class AndroidBluetoothSocket : public BluetoothSocket
{
public:
	AndroidBluetoothSocket() { }
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	#ifdef CONFIG_BLUETOOTH_SERVER
	CallResult open(BluetoothPendingSocket &socket) override;
	#endif
	void close() override;
	CallResult write(const void *data, size_t size) override;
	void onStatusDelegateMessage(int arg);

	jobject socket = nullptr, outStream = nullptr;
	ptrsize readThreadFunc(ThreadPThread &thread);
	ThreadPThread readThread;
	ptrsize connectThreadFunc(ThreadPThread &thread);
	ThreadPThread connectThread;
	Base::PollEventDelegate pollEvDel
	{
		[this](int events)
		{
			return readPendingData(events);
		}
	};
	int nativeFd = -1;
	uint channel = 0;
	bool isClosing = 0;
	bool isL2cap = 0;
	char addrStr[18] {0};

	CallResult openSocket(BluetoothAddr addr, uint channel, bool l2cap);
	int readPendingData(int events);
};
