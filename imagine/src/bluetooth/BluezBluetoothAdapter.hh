#pragma once

#include <engine-globals.h>
#include "BluetoothAdapter.hh"
#include <util/thread/pthread.hh>
#include <bluetooth/bluetooth.h>
#include <base/Base.hh>

class BluezBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr BluezBluetoothAdapter(): devId(-1), socket(-1) { }
	static BluezBluetoothAdapter *defaultAdapter();
	fbool startScan();
	void close();

	CallResult doScan();
private:
	int devId, socket;
private:
	ThreadPThread runThread;
	bool openDefault();
};

class BluezBluetoothSocket : public Base::PollHandler, public BluetoothSocket
{
public:
	constexpr BluezBluetoothSocket(): fd(0) { }
	CallResult openL2cap(BluetoothAddr addr, uint psm);
	CallResult openRfcomm(BluetoothAddr addr, uint channel);
	void close();
	CallResult write(const void *data, size_t size);
	bool readPendingData(uint events);
private:
	int fd;
};
