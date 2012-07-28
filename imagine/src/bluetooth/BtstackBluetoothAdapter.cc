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

#define thisModuleName "btstack"
#include "BtstackBluetoothAdapter.hh"
#include <util/collection/DLList.hh>

static BtstackBluetoothAdapter defaultBtstackAdapter;
uint scanSecs = 4;

struct BtstackCmd
{
	enum { CREATE_L2CAP, CREATE_RFCOMM, INQUIRY, REMOTE_NAME_REQ, WRITE_AUTH_ENABLE };
	uint cmd;
	union
	{
		struct
		{
			bd_addr_t address;
			uint channel;
		} createChannelData;
		struct
		{
			uint length;
		} inquiryData;
		struct
		{
			bd_addr_t address;
			uint8_t pageScanRepetitionMode;
			uint16_t clockOffset;
		} remoteNameRequestData;
		struct
		{
			uint on;
		} writeAuthEnableData;
	};

	void exec()
	{
		switch(cmd)
		{
			bcase CREATE_L2CAP:
				bt_send_cmd(&l2cap_create_channel, createChannelData.address, createChannelData.channel);
			bcase CREATE_RFCOMM:
				bt_send_cmd(&rfcomm_create_channel, createChannelData.address, createChannelData.channel);
			bcase INQUIRY:
				bt_send_cmd(&hci_inquiry, HCI_INQUIRY_LAP, inquiryData.length, 0);
			bcase REMOTE_NAME_REQ:
				bt_send_cmd(&hci_remote_name_request, remoteNameRequestData.address,
					remoteNameRequestData.pageScanRepetitionMode, 0, remoteNameRequestData.clockOffset);
			bcase WRITE_AUTH_ENABLE:
				bt_send_cmd(&hci_write_authentication_enable, writeAuthEnableData.on);
		}
	}

	static BtstackCmd l2capCreateChannel(bd_addr_t address, uint channel)
	{
		BtstackCmd cmd = { CREATE_L2CAP };
		memcpy(cmd.createChannelData.address, address, sizeof(bd_addr_t));
		cmd.createChannelData.channel = channel;
		return cmd;
	}

	static BtstackCmd rfcommCreateChannel(bd_addr_t address, uint channel)
	{
		BtstackCmd cmd = { CREATE_RFCOMM };
		memcpy(cmd.createChannelData.address, address, sizeof(bd_addr_t));
		cmd.createChannelData.channel = channel;
		return cmd;
	}

	static BtstackCmd inquiry(uint length)
	{
		BtstackCmd cmd = { INQUIRY };
		cmd.inquiryData.length = length;
		return cmd;
	}

	static BtstackCmd remoteNameRequest(bd_addr_t address, uint8_t pageScanRepetitionMode, uint16_t clockOffset)
	{
		BtstackCmd cmd = { REMOTE_NAME_REQ };
		memcpy(cmd.remoteNameRequestData.address, address, sizeof(bd_addr_t));
		cmd.remoteNameRequestData.pageScanRepetitionMode = pageScanRepetitionMode;
		cmd.remoteNameRequestData.clockOffset = clockOffset;
		return cmd;
	}

	static BtstackCmd writeAuthenticationEnable(uint on)
	{
		BtstackCmd cmd = { WRITE_AUTH_ENABLE };
		cmd.writeAuthEnableData.on = on;
		return cmd;
	}
};

static StaticDLList<BtstackCmd, 8> pendingCmdList;

bool BtstackBluetoothAdapter::cmdActive = 0;

void BtstackBluetoothAdapter::processCommands()
{
	if(pendingCmdList.size && !cmdActive)
	{
		BtstackCmd cmd = *pendingCmdList.first();
		pendingCmdList.removeFirst();
		cmd.exec();
		cmdActive = 1;
	}
}

class BTDevice
{
public:
	bd_addr_t address;
	//uint32_t classOfDevice;
	uint16_t clockOffset;
	uint8_t pageScanRepetitionMode;
	//uint8_t rssi;

	void requestName()
	{
		logMsg("requesting name");
		pendingCmdList.addToEnd(BtstackCmd::remoteNameRequest(address, pageScanRepetitionMode, clockOffset | 0x8000));
	}
};

static void sprintBTAddr(char *addrStr, bd_addr_t &addr)
{
	strcpy(addrStr, "");
	iterateTimes(6, i)
	{
		if(i != 0)
			strcat(addrStr, ":");
		char byteHex[5];
		snprintf(byteHex, sizeof(byteHex), "%02X", addr[i]);
		strcat(addrStr, byteHex);
	}
}

static StaticDLList<BTDevice, 10> scanDevList;
static StaticDLList<BtstackBluetoothSocket*, 16> socketList;

static void btHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	defaultBtstackAdapter.packetHandler(packet_type, channel, packet, size);
}

void BtstackBluetoothAdapter::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	//logMsg("got packet type: %s", btstackPacketTypeToString(packet_type));
	switch (packet_type)
	{
		bcase L2CAP_DATA_PACKET:
		case RFCOMM_DATA_PACKET:
		{
			/*logMsg("%s ch 0x%02X, size %d",
					packet_type == L2CAP_DATA_PACKET ? "L2CAP_DATA_PACKET" : "RFCOMM_DATA_PACKET",
					(int)channel, size);*/
			var_copy(sock, BtstackBluetoothSocket::findSocket(channel));
			if(!sock)
			{
				bug_exit("can't find socket");
				return;
			}
			sock->onDataDelegate().invoke(packet, size);
			//debugPrintL2CAPPacket(channel, packet, size);
		}

		bcase HCI_EVENT_PACKET:
		{
			switch (packet[0])
			{
				bcase BTSTACK_EVENT_STATE:
				{
					state = (HCI_STATE)packet[2];
					logMsg("got BTSTACK_EVENT_STATE: %d", state);

					if(state == HCI_STATE_WORKING)
					{
						//printAddrs();
						BtstackBluetoothAdapter::processCommands();
					}
					else if(state == HCI_STATE_OFF)
					{
						if(inDetect)
						{
							onStatus.invoke(SCAN_FAILED, 0);
							inDetect = 0;
							cmdActive = 0;
						}
					}
				}

				bcase BTSTACK_EVENT_POWERON_FAILED:
				{
					if(inDetect)
					{
						//onStatus.invoke(SCAN_FAILED);
						inDetect = 0;
						cmdActive = 0;
					}
					state = HCI_STATE_OFF;
					//logMsg("Bluetooth not accessible, Make sure you have turned off Bluetooth in the System Settings.");
					onStatus.invoke(INIT_FAILED, 0);
				}

				bcase BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
				{
					logMsg("got BTSTACK_EVENT_NR_CONNECTIONS_CHANGED");
				}

				bcase BTSTACK_EVENT_DISCOVERABLE_ENABLED:
				{
					logMsg("got BTSTACK_EVENT_DISCOVERABLE_ENABLED");
				}

				bcase HCI_EVENT_COMMAND_STATUS:
				{
					//logMsg("got HCI_EVENT_COMMAND_STATUS");
				}

				bcase HCI_EVENT_CONNECTION_COMPLETE:
				{
					//logMsg("got HCI_EVENT_CONNECTION_COMPLETE");
				}

				bcase HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
				{
					//logMsg("got HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS");
				}

				bcase L2CAP_EVENT_CREDITS:
				{
					//logMsg("got L2CAP_EVENT_CREDITS");
				}

				bcase HCI_EVENT_QOS_SETUP_COMPLETE:
				{
					//logMsg("got HCI_EVENT_QOS_SETUP_COMPLETE");
				}

				bcase HCI_EVENT_PIN_CODE_REQUEST:
				{
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					logMsg("got HCI_EVENT_PIN_CODE_REQUEST from %s", bd_addr_to_str(addr));
					var_copy(sock, BtstackBluetoothSocket::findSocket(addr));
					if(!sock)
					{
						logWarn("can't find socket");
						return;
					}
					uint size;
					const void *pin = sock->pin(size);
					if(pin)
					{
						// TODO: print non c-string pin
						logMsg("sending %s pin, size %d", (const char*)pin, size);
						bt_send_cmd(&hci_pin_code_request_reply, &addr, size, pin);
					}
					else
					{
						logMsg("sending default 1234 pin");
						bt_send_cmd(&hci_pin_code_request_reply, &addr, 4, "1234");
					}
				}

				bcase HCI_EVENT_INQUIRY_RESULT:
				case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
				{
					uint responses = scanResponses = packet[2];
					logMsg("got HCI_EVENT_INQUIRY_RESULT, %d responses", responses);
					iterateTimes(responses, i)
					{
						bd_addr_t addr;
						bt_flip_addr(addr, &packet[3+i*6]);

						uchar *devClass = &packet[3 + responses*(6+1+1+1) + i*3];
						if(!onScanDeviceClass.invoke(devClass))
						{
							logMsg("skipping device #%d due to class %X:%X:%X", i, devClass[0], devClass[1], devClass[2]);
							continue;
						}
						logMsg("new device #%d, COD: %X %X %X", i, devClass[0], devClass[1], devClass[2]);
						BTDevice dev;
						BD_ADDR_COPY(dev.address, addr);
						dev.pageScanRepetitionMode = packet[3 + responses*(6) + i*1];
						//dev.classOfDevice = READ_BT_24(packet, 3 + numResponses*(6+1+1+1) + i*3);
						dev.clockOffset = READ_BT_16(packet, 3 + responses*(6+1+1+1+3) + i*2) & 0x7fff;
						//dev.rssi = 0;
						logMsg("pageScan %u, clock offset 0x%04x", dev.pageScanRepetitionMode,  dev.clockOffset);
						if(!scanDevList.add(dev))
						{
							logMsg("max devices reached");
							break;
						}
					}
				}

				bcase HCI_EVENT_INQUIRY_COMPLETE:
				{
					cmdActive = 0;
					logMsg("got HCI_EVENT_INQUIRY_COMPLETE");
					if(scanDevList.size)
					{
						logMsg("starting name requests");
						scanDevList.first()->requestName();
						onStatus.invoke(SCAN_PROCESSING, scanDevList.size);
					}
					else
					{
						inDetect = 0;
						if(!scanResponses)
						{
							onStatus.invoke(SCAN_NO_DEVS, 0);
						}
					}
					BtstackBluetoothAdapter::processCommands();
					scanResponses = 0;
				}

				bcase BTSTACK_EVENT_REMOTE_NAME_CACHED:
					if(!BluetoothAdapter::useScanCache)
					{
						logMsg("ignoring cached name");
						break;
					}
				case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
				{
					//logMsg("got HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE ch %d", (int)channel);
					bool cached = packet[0] == BTSTACK_EVENT_REMOTE_NAME_CACHED;
					if(!cached)
						cmdActive = 0;
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[3]);

					if (packet[2] == 0)
					{
						// assert max length
						packet[9+255] = 0;

						char* name = (char*)&packet[9];
						logMsg("Name: '%s', Addr: %s", name, bd_addr_to_str(addr));
						BluetoothAddr a;
						memcpy(a.b, addr, 6);
						onScanDeviceName.invoke(name, a);
					}
					else
					{
						onStatus.invoke(SCAN_NAME_FAILED, 0);
						logMsg("Failed to get name: page timeout");
					}

					scanDevList.removeFirst();
					if(scanDevList.size)
					{
						scanDevList.first()->requestName();
					}
					else
					{
						inDetect = 0;
						onStatus.invoke(SCAN_COMPLETE, 0);
					}
					BtstackBluetoothAdapter::processCommands();
				}

				bcase HCI_EVENT_LINK_KEY_NOTIFICATION:
				{
					logMsg("got HCI_EVENT_LINK_KEY_NOTIFICATION");
				}

				bcase HCI_EVENT_LINK_KEY_REQUEST:
				{
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					logMsg("got HCI_EVENT_LINK_KEY_REQUEST from %s", bd_addr_to_str(addr));
					bt_send_cmd(&hci_link_key_request_negative_reply, &addr);
				}

				bcase L2CAP_EVENT_TIMEOUT_CHECK:
				{
					//logMsg("got L2CAP_EVENT_TIMEOUT_CHECK");
				}

				bcase HCI_EVENT_ENCRYPTION_CHANGE:
				{
					logMsg("got HCI_EVENT_ENCRYPTION_CHANGE");
				}

				bcase HCI_EVENT_MAX_SLOTS_CHANGED:
				{
					logMsg("got HCI_EVENT_MAX_SLOTS_CHANGED");
				}

				bcase HCI_EVENT_COMMAND_COMPLETE:
				{
					if (COMMAND_COMPLETE_EVENT(packet, hci_inquiry_cancel))
					{
						logMsg("inquiry canceled");
					}
					else if (COMMAND_COMPLETE_EVENT(packet, hci_remote_name_request_cancel))
					{
						logMsg("remote name request canceled");
					}
					else if(COMMAND_COMPLETE_EVENT(packet, hci_write_authentication_enable))
					{
						logMsg("write authentication changed");
					}
					else
						logMsg("got HCI_EVENT_COMMAND_COMPLETE");
					cmdActive = 0;
					BtstackBluetoothAdapter::processCommands();
				}

				bcase RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
				{
					logMsg("got RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE ch 0x%02X", (int)channel);
					cmdActive = 0;
					BtstackBluetoothSocket::handleRfcommChannelOpened(packet_type, channel, packet, size);
					BtstackBluetoothAdapter::processCommands();
				}

				bcase L2CAP_EVENT_CHANNEL_OPENED:
				{
					logMsg("got L2CAP_EVENT_CHANNEL_OPENED ch 0x%02X", (int)channel);
					cmdActive = 0;
					BtstackBluetoothSocket::handleL2capChannelOpened(packet_type, channel, packet, size);
					BtstackBluetoothAdapter::processCommands();
				}

				bcase RFCOMM_EVENT_CHANNEL_CLOSED:
				case L2CAP_EVENT_CHANNEL_CLOSED:
				{
					logMsg("got %s for 0x%02X",
						packet[0] == L2CAP_EVENT_CHANNEL_CLOSED ? "L2CAP_EVENT_CHANNEL_CLOSED" : "RFCOMM_EVENT_CHANNEL_CLOSED",
						channel);
					var_copy(sock, BtstackBluetoothSocket::findSocket(channel));
					if(!sock)
					{
						logMsg("socket already removed from list");
						return;
					}
					sock->onStatusDelegate().invoke(*sock, BluetoothSocket::STATUS_ERROR);
				}

				bdefault:
					logMsg("unhandled HCI event type 0x%X", packet[0]);
				break;
			}
		}

		bdefault:
			logMsg("unhandled packet type 0x%X", packet_type);
			break;
	}
	//logMsg("end packet");
}

fbool BtstackBluetoothAdapter::startScan()
{
	if(!inDetect)
	{
		inDetect = 1;
		pendingCmdList.addToEnd(BtstackCmd::inquiry(scanSecs));
		if(isInactive())
		{
			logMsg("BTStack power on, starting inquiry");
			bt_send_cmd(&btstack_set_power_mode, HCI_POWER_ON);
		}
		else if(!cmdActive)
		{
			logMsg("BTStack is on, starting inquiry");
			BtstackBluetoothAdapter::processCommands();
		}
		return 1;
	}
	else
	{
		logMsg("previous bluetooth detection still running");
		return 0;
	}
}

bool BtstackBluetoothAdapter::isInactive()
{
	return state != HCI_STATE_INITIALIZING && state != HCI_STATE_WORKING;
}

CallResult BtstackBluetoothAdapter::openDefault()
{
	if(isOpen)
	{
		return OK;
	}

	logMsg("opening BT adapter");
	scanDevList.init();
	socketList.init();
	pendingCmdList.init();

	static bool runLoopInit = 0;
	if(!runLoopInit)
	{
		run_loop_init(RUN_LOOP_COCOA);
		runLoopInit = 1;
	}

	if(bt_open())
	{
		logWarn("Failed to open connection to BTdaemon");
		return INVALID_PARAMETER;
	}
	bt_register_packet_handler(btHandler);
	isOpen = 1;
	logMsg("BTStack init");
	return OK;
}

void BtstackBluetoothAdapter::close()
{
	if(isOpen)
	{
		logMsg("closing BTstack");
		bt_close();
		isOpen = 0;
		inDetect = 0;
		state = HCI_STATE_OFF;
	}
}

BtstackBluetoothAdapter *BtstackBluetoothAdapter::defaultAdapter()
{
	if(defaultBtstackAdapter.openDefault() == OK)
		return &defaultBtstackAdapter;
	else
		return nullptr;
}

void BtstackBluetoothAdapter::constructSocket(void *mem)
{
	new(mem) BtstackBluetoothSocket();
}

CallResult BtstackBluetoothSocket::openRfcomm(BluetoothAddr addr, uint channel)
{
	type = 1;
	if(!socketList.add(this))
	{
		logMsg("no space left in socket list");
		return NO_FREE_ENTRIES;
	}
	logMsg("creating RFCOMM channel %d socket", channel);
	pendingCmdList.addToEnd(BtstackCmd::writeAuthenticationEnable(1));
	pendingCmdList.addToEnd(BtstackCmd::rfcommCreateChannel(addr.b, channel));
	var_selfs(addr);
	ch = channel;
	return OK;
}

CallResult BtstackBluetoothSocket::openL2cap(BluetoothAddr addr, uint psm)
{
	type = 0;
	if(!socketList.add(this))
	{
		logMsg("no space left in socket list");
		return NO_FREE_ENTRIES;
	}
	pendingCmdList.addToEnd(BtstackCmd::writeAuthenticationEnable(0));
	pendingCmdList.addToEnd(BtstackCmd::l2capCreateChannel(addr.b, psm));
	var_selfs(addr);
	ch = psm;
	return OK;
}

static bool btAddrIsEqual(const BluetoothAddr addr1, const bd_addr_t addr2)
{
	return memcmp(addr1.b, addr2, 6) == 0;
}

BtstackBluetoothSocket *BtstackBluetoothSocket::findSocket(uint16_t localCh)
{
	forEachInDLList(&socketList, e)
	{
		if(e->localCh == localCh)
		{
			return e;
		}
	}
	return nullptr;
}

BtstackBluetoothSocket *BtstackBluetoothSocket::findSocket(const bd_addr_t addr, uint16_t ch)
{
	forEachInDLList(&socketList, e)
	{
		if(e->ch == ch && btAddrIsEqual(e->addr, addr))
		{
			return e;
		}
	}
	return nullptr;
}

BtstackBluetoothSocket *BtstackBluetoothSocket::findSocket(const bd_addr_t addr)
{
	forEachInDLList(&socketList, e)
	{
		if(btAddrIsEqual(e->addr, addr))
		{
			return e;
		}
	}
	return nullptr;
}

const void *BtstackBluetoothSocket::pin(uint &size)
{
	size = pinSize;
	return pinCode;
}

void BtstackBluetoothSocket::setPin(const void *pin, uint size)
{
	pinCode = pin;
	pinSize = size;
}

void BtstackBluetoothSocket::handleRfcommChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	int ch = packet[11];
	bd_addr_t addr;
	bt_flip_addr(addr, &packet[3]);
	logMsg("handle RFCOMM channel open ch %d from %s", ch, bd_addr_to_str(addr));
	var_copy(sock, findSocket(addr, ch));
	if(!sock)
	{
		bug_exit("can't find socket");
		return;
	}
	if(packet[2] == 0)
	{
		uint16_t rfcommCh = READ_BT_16(packet, 12);
		uint16_t handle = READ_BT_16(packet, 9);
		logMsg("rfcomm ch %d, handle %d", rfcommCh, handle);
		sock->localCh = rfcommCh;
		sock->handle = handle;
		if(sock->onStatus.invoke(*sock, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		{

		}
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus.invoke(*sock, STATUS_ERROR);
		BtstackBluetoothAdapter::defaultAdapter()->statusDelegate().invoke(BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
	}
}

void BtstackBluetoothSocket::handleL2capChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	uint16_t psm = READ_BT_16(packet, 11);
	bd_addr_t addr;
	bt_flip_addr(addr, &packet[3]);
	logMsg("handle L2CAP channel open psm %d from %s", psm, bd_addr_to_str(addr));
	var_copy(sock, findSocket(addr, psm));
	if(!sock)
	{
		bug_exit("can't find socket");
		return;
	}
	if(packet[2] == 0)
	{
		uint16_t sourceCid = READ_BT_16(packet, 13);
		uint16_t handle = READ_BT_16(packet, 9);
		logMsg("source cid %d, handle %d", sourceCid, handle);
		sock->localCh = sourceCid;
		sock->handle = handle;
		if(sock->onStatus.invoke(*sock, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		{

		}
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus.invoke(*sock, STATUS_ERROR);
		BtstackBluetoothAdapter::defaultAdapter()->statusDelegate().invoke(BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
	}
}

void BtstackBluetoothSocket::close()
{
	if(localCh)
	{
		logMsg("closing BT handle %d", handle);
		bt_send_cmd(&hci_disconnect, handle, 0x13);
		/*if(type)
			bt_send_cmd(&rfcomm_disconnect, localCh, (uint8)0);
		else
			bt_send_cmd(&l2cap_disconnect, localCh, (uint8)0);*/
		handle = 0;
		localCh = 0;
	}
	socketList.remove(this);
}

CallResult BtstackBluetoothSocket::write(const void *data, size_t size)
{
	assert(localCh);
	if(type)
		bt_send_rfcomm(localCh, (uint8_t*)data, size);
	else
		bt_send_l2cap(localCh, (uint8_t*)data, size);
	return OK;
}
