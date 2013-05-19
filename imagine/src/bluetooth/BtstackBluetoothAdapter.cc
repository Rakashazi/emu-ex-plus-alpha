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
#include <util/branch2.h>

static BtstackBluetoothAdapter defaultBtstackAdapter;
static int writeAuthEnable = -1;
static bool inL2capSocketOpenHandler = 0;

struct BtstackCmd
{
	enum { NOOP, CREATE_L2CAP, CREATE_RFCOMM, INQUIRY, REMOTE_NAME_REQ, WRITE_AUTH_ENABLE,
		L2CAP_REGISTER_SERVICE, L2CAP_ACCEPT_CONNECTION };
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
		struct
		{
			uint16_t psm;
			uint16_t mtu;
		} l2capRegisterServiceData;
		struct
		{
			uint16_t localCh;
		} l2capAcceptConnectionData;
	};

	bool exec()
	{
		switch(cmd)
		{
			bcase CREATE_L2CAP:
			{
				logMsg("l2cap_create_channel");
				bt_send_cmd(&l2cap_create_channel, createChannelData.address, createChannelData.channel);
			}
			bcase CREATE_RFCOMM:
			{
				logMsg("rfcomm_create_channel");
				bt_send_cmd(&rfcomm_create_channel, createChannelData.address, createChannelData.channel);
			}
			bcase INQUIRY:
			{
				logMsg("hci_inquiry");
				bt_send_cmd(&hci_inquiry, HCI_INQUIRY_LAP, inquiryData.length, 0);
			}
			bcase REMOTE_NAME_REQ:
			{
				logMsg("hci_remote_name_request");
				bt_send_cmd(&hci_remote_name_request, remoteNameRequestData.address,
					remoteNameRequestData.pageScanRepetitionMode, 0, remoteNameRequestData.clockOffset);
			}
			bcase WRITE_AUTH_ENABLE:
			{
				/*if(writeAuthEnable == (int)writeAuthEnableData.on)
				{
					logMsg("skipping hci_write_authentication_enable: %d", writeAuthEnable);
					return 0;
				}*/
				logMsg("hci_write_authentication_enable");
				bt_send_cmd(&hci_write_authentication_enable, writeAuthEnableData.on);
				writeAuthEnable = writeAuthEnableData.on;
			}
			bcase L2CAP_REGISTER_SERVICE:
			{
				logMsg("l2cap_register_service");
				bt_send_cmd(&l2cap_register_service, l2capRegisterServiceData.psm, l2capRegisterServiceData.mtu);
			}
			bcase L2CAP_ACCEPT_CONNECTION:
			{
				logMsg("l2cap_accept_connection");
				bt_send_cmd(&l2cap_accept_connection, l2capAcceptConnectionData.localCh);
			}
			bcase NOOP:
				break;
		}
		return 1;
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

	static BtstackCmd l2capRegisterService(uint16_t psm, uint16_t mtu)
	{
		BtstackCmd cmd = { L2CAP_REGISTER_SERVICE };
		cmd.l2capRegisterServiceData.psm = psm;
		cmd.l2capRegisterServiceData.mtu = mtu;
		return cmd;
	}

	static BtstackCmd l2capAcceptConnection(uint16_t localCh)
	{
		BtstackCmd cmd = { L2CAP_ACCEPT_CONNECTION };
		cmd.l2capAcceptConnectionData.localCh = localCh;
		return cmd;
	}
};

static StaticDLList<BtstackCmd, 8> pendingCmdList;
static BtstackCmd activeCmd = { BtstackCmd::NOOP };

bool BtstackBluetoothAdapter::cmdActive = 0;

void BtstackBluetoothAdapter::processCommands()
{
	while(pendingCmdList.size && !cmdActive)
	{
		BtstackCmd cmd = *pendingCmdList.first();
		pendingCmdList.removeFirst();
		if(cmd.exec())
		{
			cmdActive = 1;
			activeCmd = cmd;
			break;
		}
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
static StaticDLList<BTDevice, 2> incomingDevList;
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
			auto sock = BtstackBluetoothSocket::findSocket(channel);
			if(!sock)
			{
				bug_exit("can't find socket");
				return;
			}
			sock->onData()(packet, size);
			//debugPrintL2CAPPacket(channel, packet, size);
		}

		bcase HCI_EVENT_PACKET:
		{
			switch (packet[0])
			{
				bcase BTSTACK_EVENT_STATE:
				{
					state_ = (HCI_STATE)packet[2];
					logMsg("got BTSTACK_EVENT_STATE: %d", state_);
					if(state_ == HCI_STATE_WORKING)
					{
						if(onStateChangeD)
						{
							onStateChangeD(*this, STATE_ON);
							onStateChangeD = {};
						}
						//printAddrs();
						//BtstackBluetoothAdapter::processCommands();
					}
					else if(state_ == HCI_STATE_OFF)
					{
						if(inDetect)
						{
							onScanStatusD(*this, SCAN_FAILED, 0);
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
					state_ = HCI_STATE_OFF;
					//logMsg("Bluetooth not accessible, Make sure you have turned off Bluetooth in the System Settings.");
					//onScanStatusD(INIT_FAILED, 0);
					if(onStateChangeD)
					{
						onStateChangeD(*this, STATE_ERROR);
						onStateChangeD = {};
					}
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
					uint16_t handle = READ_BT_16(packet, 3);
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[5]);
					uint status = packet[2];
					logMsg("got HCI_EVENT_CONNECTION_COMPLETE: addr: %s, handle: %d, status: %d", bd_addr_to_str(addr), handle, status);
					if(cmdActive && activeCmd.cmd == BtstackCmd::CREATE_L2CAP && BD_ADDR_CMP(activeCmd.createChannelData.address, addr) == 0)
					{
						// update handle
						auto sock = BtstackBluetoothSocket::findSocket(addr, activeCmd.createChannelData.channel);
						if(!sock)
						{
							bug_exit("can't find socket");
							break;
						}
						sock->handle = handle;
					}
				}

				bcase HCI_EVENT_DISCONNECTION_COMPLETE:
				{
					uint16_t handle = READ_BT_16(packet, 3);
					logMsg("got HCI_EVENT_DISCONNECTION_COMPLETE: handle: %d", handle);
					if(cmdActive && activeCmd.cmd == BtstackCmd::CREATE_L2CAP)
					{
						auto sock = BtstackBluetoothSocket::findSocket(activeCmd.createChannelData.address, activeCmd.createChannelData.channel);
						if(!sock)
						{
							bug_exit("can't find socket");
							break;
						}
						if(sock->handle != handle)
							break;
						logMsg("disconnection while l2cap open in progress");
						cmdActive = 0;
						sock->onStatus()(*sock, BluetoothSocket::STATUS_ERROR);
						if(defaultBtstackAdapter.onScanStatus())
							defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
					}
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

				bcase RFCOMM_EVENT_CREDITS:
				{
					//logMsg("got RFCOMM_EVENT_CREDITS");
				}

				bcase HCI_EVENT_PIN_CODE_REQUEST:
				{
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					logMsg("got HCI_EVENT_PIN_CODE_REQUEST from %s", bd_addr_to_str(addr));
					auto sock = BtstackBluetoothSocket::findSocket(addr);
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
						if(!onScanDeviceClassD(*this, devClass))
						{
							logMsg("skipping device #%d due to class %X:%X:%X", i, devClass[0], devClass[1], devClass[2]);
							continue;
						}
						logMsg("new device #%d, addr: %s, COD: %X %X %X", i, bd_addr_to_str(addr), devClass[0], devClass[1], devClass[2]);
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
						// scan will complete after name requests
						logMsg("starting name requests");
						forEachInDLList(&scanDevList, e)
						{
							e.requestName();
						}
						onScanStatusD(*this, SCAN_PROCESSING, scanDevList.size);
					}
					else
					{
						inDetect = 0;
						if(!scanResponses)
						{
							onScanStatusD(*this, SCAN_NO_DEVS, 0);
						}
						else
						{
							logMsg("no name requests needed, scan complete");
							onScanStatusD(*this, SCAN_COMPLETE, 0);
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

					if(packet[2] == 0)
					{
						// assert max length
						packet[9+255] = 0;
						char* name = (char*)&packet[9];
						logMsg("Name: '%s', Addr: %s, cached: %d", name, bd_addr_to_str(addr), cached);
						onScanDeviceNameD(*this, name, addr);
					}
					else
					{
						onScanDeviceNameD(*this, nullptr, addr);
						logMsg("Failed to get name: page timeout");
					}

					// TODO: decouple from Bluetooth inquiry operations
					if(!inDetect)
					{
						cmdActive = 0;
						scanDevList.removeAll();
						if(onScanStatusD)
							onScanStatusD(*this, SCAN_CANCELLED, 0);
						BtstackBluetoothAdapter::processCommands();
						break;
					}

					bool removedFromScanList = 0;
					forEachInDLList(&scanDevList, e)
					{
						if(BD_ADDR_CMP(e.address, addr) == 0)
						{
							e_it.removeElem();
							removedFromScanList = 1;
							break;
						}
					}
					assert(removedFromScanList);

					if(!cached && !scanDevList.size)
					{
						logMsg("finished name requests, scan complete");
						inDetect = 0;
						onScanStatusD(*this, SCAN_COMPLETE, 0);
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
					if(COMMAND_COMPLETE_EVENT(packet, hci_inquiry_cancel))
					{
						logMsg("inquiry canceled");
					}
					else if(COMMAND_COMPLETE_EVENT(packet, hci_remote_name_request_cancel))
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
					auto sock = BtstackBluetoothSocket::findSocket(channel);
					if(!sock)
					{
						logMsg("socket already removed from list");
						return;
					}
					sock->onStatus()(*sock, BluetoothSocket::STATUS_ERROR);
				}

				bcase L2CAP_EVENT_SERVICE_REGISTERED:
				{
					cmdActive = 0;
					uint8 status = packet[2];
					uint psm = READ_BT_16(packet, 3);
					auto onResult = setL2capServiceOnResult;
					setL2capServiceOnResult = {};
					if(status && status != L2CAP_SERVICE_ALREADY_REGISTERED)
					{
						logErr("error %d registering psm %d", status, psm);
						onResult(*this, 0, 0);
						break;
					}
					logMsg("registered l2cap service for psm 0x%X", psm);
					onResult(*this, 1, 0);
					BtstackBluetoothAdapter::processCommands();
				}

				bcase L2CAP_EVENT_INCOMING_CONNECTION:
				{
					uint16_t psm = READ_BT_16(packet, 10);
					uint16_t sourceCid = READ_BT_16(packet, 12);
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					BluetoothPendingSocket pending {0, addr, psm, sourceCid};
					onIncomingL2capConnectionD(*this, pending);
				}

				bdefault:
					logMsg("unhandled HCI event type 0x%X", packet[0]);
				break;
			}
		}

		bcase DAEMON_EVENT_PACKET:
		//logMsg("got DAEMON_EVENT_PACKET");

		bdefault:
			logMsg("unhandled packet type 0x%X", packet_type);
			break;
	}
	//logMsg("end packet");
}

void BtstackBluetoothAdapter::setL2capService(uint psm, bool on, OnStatusDelegate onResult)
{
	if(on)
	{
		logMsg("registering l2cap service for psm 0x%X", psm);
		assert(!setL2capServiceOnResult); // only handle one request at a time
		setL2capServiceOnResult = onResult;
		pendingCmdList.addToEnd(BtstackCmd::l2capRegisterService(psm, 672));
		if(isInactive())
		{
			setActiveState(true,
				[this](BluetoothAdapter &, State newState)
				{
					if(newState != STATE_ON)
					{
						auto onResult = setL2capServiceOnResult;
						setL2capServiceOnResult = {};
						onResult(*this, 0, 0);
						return;
					}
					BtstackBluetoothAdapter::processCommands();
				}
			);
		}
		else
			BtstackBluetoothAdapter::processCommands();
	}
	else
	{
		bt_send_cmd(&l2cap_unregister_service, psm);
		logMsg("unregistered l2cap service for psm 0x%X", psm);
	}
}

BluetoothAdapter::State BtstackBluetoothAdapter::state()
{
	switch(state_)
	{
		case HCI_STATE_OFF:
		case HCI_STATE_SLEEPING:
			return STATE_OFF;
		case HCI_STATE_INITIALIZING:
			return STATE_TURNING_ON;
		case HCI_STATE_WORKING:
			return STATE_ON;
		case HCI_STATE_HALTING:
		case HCI_STATE_FALLING_ASLEEP:
			return STATE_TURNING_OFF;
	}
	logWarn("unknown bluetooth state: %d", state_);
	return STATE_OFF;
}

void BtstackBluetoothAdapter::setActiveState(bool on, OnStateChangeDelegate onStateChange)
{
	if(onStateChangeD)
	{
		bug_exit("state change already in progress");
		return;
	}
	if(on)
	{
		if(isInactive())
		{
			logMsg("powering on Bluetooth");
			onStateChangeD = onStateChange;
			bt_send_cmd(&btstack_set_power_mode, HCI_POWER_ON);
		}
		else if(onStateChange) // already on
		{
			logMsg("Bluetooth is already on");
			onStateChange(*this, STATE_ON);
		}
	}
	else
	{
		// TODO
		bug_exit("TODO");
		onStateChange(*this, STATE_ERROR);
	}
}

bool BtstackBluetoothAdapter::startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName)
{
	if(!inDetect)
	{
		inDetect = 1;
		onScanStatusD = onResult;
		onScanDeviceClassD = onDeviceClass;
		onScanDeviceNameD = onDeviceName;
		pendingCmdList.addToEnd(BtstackCmd::inquiry(scanSecs));
		logMsg("starting inquiry");
		if(isInactive())
		{
			setActiveState(true,
				[this](BluetoothAdapter &, State newState)
				{
					if(newState != STATE_ON)
					{
						onScanStatusD(*this, INIT_FAILED, 0);
						return;
					}
					BtstackBluetoothAdapter::processCommands();
				}
			);
		}
		else
			BtstackBluetoothAdapter::processCommands();
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
	return state_ != HCI_STATE_INITIALIZING && state_ != HCI_STATE_WORKING;
}

CallResult BtstackBluetoothAdapter::openDefault()
{
	if(isOpen)
	{
		return OK;
	}

	logMsg("opening BT adapter");

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

void BtstackBluetoothAdapter::cancelScan()
{
	if(inDetect)
	{
		// check if scan is queued
		bool wasQueued = 0;
		forEachInDLList(&pendingCmdList, e)
		{
			if(e.cmd == BtstackCmd::INQUIRY)
			{
				e_it.removeElem();
				wasQueued = 1;
				logMsg("cancelling scan from queue");
				inDetect = 0;
				onScanStatusD(*this, SCAN_CANCELLED, 0);
				break;
			}
		}

		if(!wasQueued)
		{
			logMsg("cancelling scan in progress");
			bt_send_cmd(&hci_inquiry_cancel);
			//BtstackBluetoothAdapter::processCommands();
		}

		inDetect = 0;
	}
}

void BtstackBluetoothAdapter::close()
{
	if(isOpen)
	{
		logMsg("closing BTstack");
		cancelScan();
		bt_close();
		isOpen = 0;
		state_ = HCI_STATE_OFF;
	}
}

BtstackBluetoothAdapter *BtstackBluetoothAdapter::defaultAdapter()
{
	if(defaultBtstackAdapter.openDefault() == OK)
		return &defaultBtstackAdapter;
	else
		return nullptr;
}

void BtstackBluetoothAdapter::requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName)
{
	onScanDeviceNameD = onDeviceName;
	pendingCmdList.addToEnd(BtstackCmd::remoteNameRequest(pending.addr.b, 0, 0));
	BtstackBluetoothAdapter::processCommands();
}

void BluetoothPendingSocket::requestName(BluetoothAdapter::OnScanDeviceNameDelegate onDeviceName)
{
	assert(ch);
	defaultBtstackAdapter.requestName(*this, onDeviceName);
}

void BluetoothPendingSocket::close()
{
	assert(ch);
	logMsg("declining L2CAP connection %d", localCh);
	bt_send_cmd(&l2cap_decline_connection, localCh, 0);
	ch = 0;
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
	BtstackBluetoothAdapter::processCommands();
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
	if(inL2capSocketOpenHandler)
	{
		// hack to boost command priority when opening 2nd Wiimote channel
		pendingCmdList.add(BtstackCmd::l2capCreateChannel(addr.b, psm));
	}
	else
	{
		pendingCmdList.addToEnd(BtstackCmd::writeAuthenticationEnable(0));
		pendingCmdList.addToEnd(BtstackCmd::l2capCreateChannel(addr.b, psm));
	}
	var_selfs(addr);
	ch = psm;
	BtstackBluetoothAdapter::processCommands();
	return OK;
}

CallResult BtstackBluetoothSocket::open(BluetoothPendingSocket &pending)
{
	assert(pending);
	addr = pending.addr;
	type = pending.type;
	ch = pending.ch;
	localCh = pending.localCh;
	if(!socketList.add(this))
	{
		logMsg("no space left in socket list");
		return NO_FREE_ENTRIES;
	}
	pendingCmdList.addToEnd(BtstackCmd::l2capAcceptConnection(localCh));
	pending = {};
	BtstackBluetoothAdapter::processCommands();
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
	auto sock = findSocket(addr, ch);
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
		if(sock->onStatus()(*sock, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		{

		}
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus()(*sock, STATUS_ERROR);
		if(defaultBtstackAdapter.onScanStatus())
			defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
	}
}

void BtstackBluetoothSocket::handleL2capChannelOpened(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	uint16_t psm = READ_BT_16(packet, 11);
	bd_addr_t addr;
	bt_flip_addr(addr, &packet[3]);
	logMsg("handle L2CAP channel open psm %d from %s", psm, bd_addr_to_str(addr));
	auto sock = findSocket(addr, psm);
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
		inL2capSocketOpenHandler = 1;
		if(sock->onStatus()(*sock, STATUS_OPENED) == REPLY_OPENED_USE_READ_EVENTS)
		{

		}
		inL2capSocketOpenHandler = 0;
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus()(*sock, STATUS_ERROR);
		if(defaultBtstackAdapter.onScanStatus())
			defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);
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
