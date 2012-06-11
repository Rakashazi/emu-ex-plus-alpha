#pragma once

#include <engine-globals.h>
#include "BluetoothAdapter.hh"
#import <btstack/btstack.h>

class BtstackBluetoothAdapter : public BluetoothAdapter
{
public:
	//constexpr BtstackBluetoothAdapter() { }
	static BtstackBluetoothAdapter *defaultAdapter();
	fbool startScan();
	void close();

	void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
private:
	uint state = HCI_STATE_OFF, scanResponses = 0;
	bool isOpen = 0;
	static bool cmdActive;
	CallResult openDefault();
	bool isInactive();
	static void processCommands();
};

class BtstackBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BtstackBluetoothSocket() { }
	CallResult openL2cap(BluetoothAddr addr, uint psm);
	CallResult openRfcomm(BluetoothAddr addr, uint channel);
	void close();
	CallResult write(const void *data, size_t size);

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
	uint16_t handle = 0;
	const void *pinCode = nullptr;
	uint pinSize = 0;
};
