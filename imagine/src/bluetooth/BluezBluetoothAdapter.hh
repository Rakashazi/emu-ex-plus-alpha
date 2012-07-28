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
	fbool startScan() override;
	void close() override;
	void constructSocket(void *mem) override;
	CallResult doScan();
private:
	int devId = -1, socket = -1;
private:
	ThreadPThread runThread;
	bool openDefault();
};

class BluezBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BluezBluetoothSocket():
		pollEvDel(Base::PollEventDelegate::create<BluezBluetoothSocket, &BluezBluetoothSocket::readPendingData>(this)) { }
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	void close() override;
	CallResult write(const void *data, size_t size) override;
	int readPendingData(int events);
private:
	Base::PollEventDelegate pollEvDel;
	int fd = 0;
};
