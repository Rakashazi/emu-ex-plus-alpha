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

#include <imagine/config/defs.hh>
#include "BluetoothAdapter.hh"
#include <imagine/base/Error.hh>
#import <btstack/btstack.h>

class BtstackBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr BtstackBluetoothAdapter() {}
	static BtstackBluetoothAdapter *defaultAdapter(Base::ApplicationContext);
	bool startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName) final;
	void cancelScan() final;
	void close() final;
	void setL2capService(uint32_t psm, bool active, OnStatusDelegate onResult) final;
	State state() final;
	void setActiveState(bool on, OnStateChangeDelegate onStateChange) final;
	void requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName);
	void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void processCommands();

private:
	uint32_t state_ = HCI_STATE_OFF, scanResponses = 0;
	bool isOpen = false;
	static bool cmdActive;
	OnStatusDelegate setL2capServiceOnResult;
	OnStateChangeDelegate onStateChangeD;

	IG::ErrorCode openDefault();
	bool isInactive();
};

class BluetoothPendingSocket
{
public:
	uint32_t type = 0;
	BluetoothAddr addr;
	uint16_t ch = 0;
	uint16_t localCh = 0;

	constexpr BluetoothPendingSocket() {}
	constexpr BluetoothPendingSocket(uint32_t type, BluetoothAddr addr, uint16_t ch, uint16_t localCh):
		type(type), addr(addr), ch(ch), localCh(localCh) {}
	void close();
	uint32_t channel() { return ch; }
	void requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName);

	explicit operator bool() const
	{
		return ch != 0;
	}
};

class BtstackBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BtstackBluetoothSocket(Base::ApplicationContext) {}
	IG::ErrorCode openL2cap(BluetoothAdapter &, BluetoothAddr addr, uint32_t psm) final;
	IG::ErrorCode openRfcomm(BluetoothAdapter &, BluetoothAddr addr, uint32_t channel) final;
	#ifdef CONFIG_BLUETOOTH_SERVER
	IG::ErrorCode open(BluetoothAdapter &, BluetoothPendingSocket &pending) final;
	#endif
	void close() final;
	IG::ErrorCode write(const void *data, size_t size) final;
	const void *pin(uint32_t &size);
	void setPin(const void *pin, uint32_t size);
	static BtstackBluetoothSocket *findSocket(const bd_addr_t addr, uint16_t ch);
	static BtstackBluetoothSocket *findSocket(const bd_addr_t addr);
	static BtstackBluetoothSocket *findSocket(uint16_t localCh);
	static void handleL2capChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	static void handleRfcommChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

private:
	uint32_t type = 0;
	BluetoothAddr addr;
	uint16_t ch = 0;
	uint16_t localCh = 0;
public:
	uint16_t handle = 0;
private:
	const void *pinCode = nullptr;
	uint32_t pinSize = 0;
};
