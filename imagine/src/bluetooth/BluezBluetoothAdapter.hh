#pragma once

#include <engine-globals.h>
#include "BluetoothAdapter.hh"
#include <util/thread/pthread.hh>
#include <bluetooth/bluetooth.h>
#include <base/Base.hh>

class BluezBluetoothAdapter : public BluetoothAdapter
{
public:
	static BluezBluetoothAdapter *defaultAdapter();
	bool startScan() override;
	void cancelScan() override;
	void close() override;
	void constructSocket(void *mem) override;
private:
	int devId = -1, socket = -1;
private:
	ThreadPThread runThread;
	bool scanCancelled = 0;
	bool openDefault();
	CallResult doScan();
	ptrsize runScan(ThreadPThread &thread);
};

class BluezBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BluezBluetoothSocket() { }
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	void close() override;
	CallResult write(const void *data, size_t size) override;
	int readPendingData(int events);
private:
	Base::PollEventDelegate pollEvDel {Base::PollEventDelegate::create<BluezBluetoothSocket, &BluezBluetoothSocket::readPendingData>(this)};
	int fd = -1;
};
