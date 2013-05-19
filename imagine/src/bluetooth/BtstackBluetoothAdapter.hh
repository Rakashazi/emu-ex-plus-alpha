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
#import <btstack/btstack.h>

class BtstackBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr BtstackBluetoothAdapter() {}
	static BtstackBluetoothAdapter *defaultAdapter();
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) override;
	void cancelScan() override;
	void close() override;
	void setL2capService(uint psm, bool active, OnStatusDelegate onResult) override;
	State state() override;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) override;
	void requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName);
	void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void processCommands();
private:
	uint state_ = HCI_STATE_OFF, scanResponses = 0;
	bool isOpen = 0;
	static bool cmdActive;
	OnStatusDelegate setL2capServiceOnResult;
	OnStateChangeDelegate onStateChangeD;
	CallResult openDefault();
	bool isInactive();
};

class BluetoothPendingSocket
{
public:
	constexpr BluetoothPendingSocket() {}
	constexpr BluetoothPendingSocket(uint type, BluetoothAddr addr, uint16_t ch, uint16_t localCh):
		type(type), addr(addr), ch(ch), localCh(localCh) {}
	void close();
	uint channel() { return ch; }
	void requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName);

	operator bool() const
	{
		return ch != 0;
	}

	uint type = 0;
	BluetoothAddr addr;
	uint16_t ch = 0;
	uint16_t localCh = 0;
};

class BtstackBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BtstackBluetoothSocket() {}
	CallResult openL2cap(BluetoothAddr addr, uint psm) override;
	CallResult openRfcomm(BluetoothAddr addr, uint channel) override;
	#ifdef CONFIG_BLUETOOTH_SERVER
	CallResult open(BluetoothPendingSocket &pending) override;
	#endif
	void close() override;
	CallResult write(const void *data, size_t size) override;

	const void *pin(uint &size);
	void setPin(const void *pin, uint size);
	static BtstackBluetoothSocket *findSocket(const bd_addr_t addr, uint16_t ch);
	static BtstackBluetoothSocket *findSocket(const bd_addr_t addr);
	static BtstackBluetoothSocket *findSocket(uint16_t localCh);
	static void handleL2capChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void handleRfcommChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

private:
	uint type = 0;
	BluetoothAddr addr;
	uint16_t ch = 0;
	uint16_t localCh = 0;
public:
	uint16_t handle = 0;
private:
	const void *pinCode = nullptr;
	uint pinSize = 0;
};
