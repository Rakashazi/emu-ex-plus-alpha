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
#include <imagine/base/ApplicationContext.hh>
#import <btstack/btstack.h>

namespace IG
{

class BtstackBluetoothAdapter
{
public:
	void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void processCommands();

protected:
	uint32_t state_ = HCI_STATE_OFF, scanResponses{};
	static bool cmdActive;
	bool inDetect{};
	BTOnStatusDelegate setL2capServiceOnResult;
	BTOnStateChangeDelegate onStateChange;

	bool isInactive();
};

using BluetoothAdapterImpl = BtstackBluetoothAdapter;

class BluetoothPendingSocket
{
public:
	uint32_t type = 0;
	BluetoothAddr addr;
	uint16_t ch = 0;
	uint16_t localCh = 0;

	constexpr BluetoothPendingSocket() = default;
	constexpr BluetoothPendingSocket(uint32_t type, BluetoothAddr addr, uint16_t ch, uint16_t localCh):
		type(type), addr(addr), ch(ch), localCh(localCh) {}
	void close();
	uint32_t channel() { return ch; }
	void requestName(BluetoothAdapter&, BTOnScanDeviceNameDelegate);

	explicit operator bool() const
	{
		return ch != 0;
	}
};

class BtstackBluetoothSocket
{
public:
	constexpr BtstackBluetoothSocket() = default;
	BtstackBluetoothSocket(ApplicationContext) {}
	~BtstackBluetoothSocket();
	void close();
	const void *pin(uint32_t &size);
	void setPin(const void *pin, uint32_t size);
	static BluetoothSocket *findSocket(const bd_addr_t addr, uint16_t ch);
	static BluetoothSocket *findSocket(const bd_addr_t addr);
	static BluetoothSocket *findSocket(uint16_t localCh);
	static void handleL2capChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void handleRfcommChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

protected:
	uint32_t type = 0;
	BluetoothAddr addr{};
	uint16_t ch = 0;
	uint16_t localCh = 0;
public:
	uint16_t handle = 0;
protected:
	const void *pinCode = nullptr;
	uint32_t pinSize = 0;
};

using BluetoothSocketImpl = BtstackBluetoothSocket;

}
