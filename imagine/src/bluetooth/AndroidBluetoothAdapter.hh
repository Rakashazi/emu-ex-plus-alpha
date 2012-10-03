#pragma once

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
	fbool startScan() override;
	void cancelScan() override;
	void close() override;
	void constructSocket(void *mem) override;
//private:
	bool openDefault();
	bool scanCancelled = 0;
	jobject adapter = nullptr;
};

class AndroidBluetoothSocket : public BluetoothSocket
{
public:
	constexpr AndroidBluetoothSocket() { }
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	void close() override;
	CallResult write(const void *data, size_t size) override;

	jobject socket = nullptr, outStream = nullptr;
	static void* readThreadFunc(void *);
	ThreadPThread readThread;
	bool isClosing = 0;
	CallResult openSocket(BluetoothAddr addr, uint channel, bool l2cap);
};

namespace Base
{

void sendBTSocketData(BluetoothSocket &socket, int len, jbyte *data);

static void sendBTSocketStatusDelegate(BluetoothSocket &socket, uint status)
{
	sendMessageToMain(MSG_BT_SOCKET_STATUS_DELEGATE, 0, status, (int)&socket);
}

static void sendBTScanStatusDelegate(uint status, int arg = 0)
{
	sendMessageToMain(MSG_BT_SCAN_STATUS_DELEGATE, 0, status, arg);
}

}
