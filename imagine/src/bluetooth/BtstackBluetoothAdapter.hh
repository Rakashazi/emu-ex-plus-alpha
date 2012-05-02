#pragma once

#include <engine-globals.h>
#include "BluetoothAdapter.hh"
#import <btstack/btstack.h>

class BtstackBluetoothAdapter : public BluetoothAdapter
{
public:
	constexpr BtstackBluetoothAdapter():
		state(HCI_STATE_OFF), scanResponses(0), isOpen(0) { }
	static BtstackBluetoothAdapter *defaultAdapter();
	fbool startScan();
	void close();

	void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
private:
	uint state, scanResponses;
	bool isOpen;
	static bool cmdActive;
	CallResult openDefault();
	bool isInactive();
	static void processCommands();
};

class BtstackBluetoothSocket : public BluetoothSocket
{
public:
	constexpr BtstackBluetoothSocket(): type(0), ch(0), localCh(0), handle(0),
		pinCode(nullptr), pinSize(0) { }
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
	uint type;
	BluetoothAddr addr;
	uint16_t ch;
	uint16_t localCh;
	uint16_t handle;
	const void *pinCode;
	uint pinSize;
};
