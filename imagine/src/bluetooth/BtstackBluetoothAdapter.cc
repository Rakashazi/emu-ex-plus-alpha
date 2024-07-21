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

#define LOGTAG "BTstack"
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/utility.h>
#include <imagine/util/ranges.hh>
#include <imagine/util/algorithm.h>

namespace IG
{

static int writeAuthEnable = -1;
static bool inL2capSocketOpenHandler = 0;

struct BtstackCmd
{
	enum { NOOP, CREATE_L2CAP, CREATE_RFCOMM, INQUIRY, REMOTE_NAME_REQ, WRITE_AUTH_ENABLE,
		L2CAP_REGISTER_SERVICE, L2CAP_ACCEPT_CONNECTION };
	uint32_t cmd{};
	union
	{
		struct
		{
			BluetoothAddr address;
			uint32_t channel;
		} createChannelData;
		struct
		{
			uint32_t length;
		} inquiryData;
		struct
		{
			BluetoothAddr address;
			uint8_t pageScanRepetitionMode;
			uint16_t clockOffset;
		} remoteNameRequestData;
		struct
		{
			uint32_t on;
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

	constexpr BtstackCmd() {};
	constexpr BtstackCmd(uint32_t cmd):cmd{cmd} {};

	bool exec()
	{
		switch(cmd)
		{
			case CREATE_L2CAP:
			{
				logMsg("l2cap_create_channel");
				bt_send_cmd(&l2cap_create_channel, createChannelData.address.data(), createChannelData.channel);
				break;
			}
			case CREATE_RFCOMM:
			{
				logMsg("rfcomm_create_channel");
				bt_send_cmd(&rfcomm_create_channel, createChannelData.address.data(), createChannelData.channel);
				break;
			}
			case INQUIRY:
			{
				logMsg("hci_inquiry");
				bt_send_cmd(&hci_inquiry, HCI_INQUIRY_LAP, inquiryData.length, 0);
				break;
			}
			case REMOTE_NAME_REQ:
			{
				logMsg("hci_remote_name_request");
				bt_send_cmd(&hci_remote_name_request, remoteNameRequestData.address.data(),
					remoteNameRequestData.pageScanRepetitionMode, 0, remoteNameRequestData.clockOffset);
				break;
			}
			case WRITE_AUTH_ENABLE:
			{
				/*if(writeAuthEnable == (int)writeAuthEnableData.on)
				{
					logMsg("skipping hci_write_authentication_enable: %d", writeAuthEnable);
					return 0;
				}*/
				logMsg("hci_write_authentication_enable");
				bt_send_cmd(&hci_write_authentication_enable, writeAuthEnableData.on);
				writeAuthEnable = writeAuthEnableData.on;
				break;
			}
			case L2CAP_REGISTER_SERVICE:
			{
				logMsg("l2cap_register_service");
				bt_send_cmd(&l2cap_register_service, l2capRegisterServiceData.psm, l2capRegisterServiceData.mtu);
				break;
			}
			case L2CAP_ACCEPT_CONNECTION:
			{
				logMsg("l2cap_accept_connection");
				bt_send_cmd(&l2cap_accept_connection, l2capAcceptConnectionData.localCh);
				break;
			}
			case NOOP:
				break;
		}
		return 1;
	}

	static BtstackCmd l2capCreateChannel(BluetoothAddr address, uint32_t channel)
	{
		BtstackCmd cmd{CREATE_L2CAP};
		cmd.createChannelData.address = address;
		cmd.createChannelData.channel = channel;
		return cmd;
	}

	static BtstackCmd rfcommCreateChannel(BluetoothAddr address, uint32_t channel)
	{
		BtstackCmd cmd{CREATE_RFCOMM};
		cmd.createChannelData.address = address;
		cmd.createChannelData.channel = channel;
		return cmd;
	}

	static BtstackCmd inquiry(uint32_t length)
	{
		BtstackCmd cmd{INQUIRY};
		cmd.inquiryData.length = length;
		return cmd;
	}

	static BtstackCmd remoteNameRequest(BluetoothAddr address, uint8_t pageScanRepetitionMode, uint16_t clockOffset)
	{
		BtstackCmd cmd{REMOTE_NAME_REQ};
		cmd.remoteNameRequestData.address = address;
		cmd.remoteNameRequestData.pageScanRepetitionMode = pageScanRepetitionMode;
		cmd.remoteNameRequestData.clockOffset = clockOffset;
		return cmd;
	}

	static BtstackCmd writeAuthenticationEnable(uint32_t on)
	{
		BtstackCmd cmd{WRITE_AUTH_ENABLE};
		cmd.writeAuthEnableData.on = on;
		return cmd;
	}

	static BtstackCmd l2capRegisterService(uint16_t psm, uint16_t mtu)
	{
		BtstackCmd cmd{L2CAP_REGISTER_SERVICE};
		cmd.l2capRegisterServiceData.psm = psm;
		cmd.l2capRegisterServiceData.mtu = mtu;
		return cmd;
	}

	static BtstackCmd l2capAcceptConnection(uint16_t localCh)
	{
		BtstackCmd cmd{L2CAP_ACCEPT_CONNECTION};
		cmd.l2capAcceptConnectionData.localCh = localCh;
		return cmd;
	}
};

static StaticArrayList<BtstackCmd, 8> pendingCmdList;
static BtstackCmd activeCmd{BtstackCmd::NOOP};

bool BtstackBluetoothAdapter::cmdActive = 0;

void BtstackBluetoothAdapter::processCommands()
{
	while(pendingCmdList.size() && !cmdActive)
	{
		BtstackCmd cmd = pendingCmdList.front();
		pendingCmdList.erase(pendingCmdList.begin());
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
		pendingCmdList.emplace_back(BtstackCmd::remoteNameRequest(address, pageScanRepetitionMode, clockOffset | 0x8000));
	}
};

inline void sprintBTAddr(char *addrStr, bd_addr_t &addr)
{
	strcpy(addrStr, "");
	for(auto i : iotaCount(6))
	{
		if(i != 0)
			strcat(addrStr, ":");
		char byteHex[5];
		snprintf(byteHex, sizeof(byteHex), "%02X", addr[i]);
		strcat(addrStr, byteHex);
	}
}

static StaticArrayList<BTDevice, 10> scanDevList;
static StaticArrayList<BTDevice, 2> incomingDevList;
static StaticArrayList<BluetoothSocket*, 16> socketList;
static BluetoothAdapter *btaPtr{};

static void btHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	btaPtr->packetHandler(packet_type, channel, packet, size);
}

void BtstackBluetoothAdapter::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	//logMsg("got packet type: %s", btstackPacketTypeToString(packet_type));
	auto &bta = static_cast<BluetoothAdapter&>(*this);
	switch (packet_type)
	{
		case L2CAP_DATA_PACKET:
		case RFCOMM_DATA_PACKET:
		{
			/*logMsg("%s ch 0x%02X, size %d",
					packet_type == L2CAP_DATA_PACKET ? "L2CAP_DATA_PACKET" : "RFCOMM_DATA_PACKET",
					(int)channel, size);*/
			auto sock = BtstackBluetoothSocket::findSocket(channel);
			if(!sock)
			{
				logErr("can't find socket");
				return;
			}
			sock->onData((char*)packet, size);
			//debugPrintL2CAPPacket(channel, packet, size);
			break;
		}

		case HCI_EVENT_PACKET:
		{
			switch (packet[0])
			{
				case BTSTACK_EVENT_STATE:
				{
					state_ = (HCI_STATE)packet[2];
					logMsg("got BTSTACK_EVENT_STATE: %d", state_);
					if(state_ == HCI_STATE_WORKING)
					{
						if(bta.onStateChange)
						{
							bta.onStateChange(bta, BluetoothState::On);
							bta.onStateChange = {};
						}
						//printAddrs();
						//BtstackBluetoothAdapter::processCommands();
					}
					else if(state_ == HCI_STATE_OFF)
					{
						if(inDetect)
						{
							bta.onScanStatus(bta, BluetoothScanState::Failed, 0);
							inDetect = 0;
							cmdActive = 0;
						}
					}
					break;
				}

				case BTSTACK_EVENT_POWERON_FAILED:
				{
					if(inDetect)
					{
						//onStatus.invoke(BluetoothScanState::Failed);
						inDetect = 0;
						cmdActive = 0;
					}
					state_ = HCI_STATE_OFF;
					//logMsg("Bluetooth not accessible, Make sure you have turned off Bluetooth in the System Settings.");
					//onScanStatus(INIT_FAILED, 0);
					if(bta.onStateChange)
					{
						bta.onStateChange(bta, BluetoothState::Error);
						bta.onStateChange = {};
					}
					break;
				}

				case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
				{
					logMsg("got BTSTACK_EVENT_NR_CONNECTIONS_CHANGED"); break;
				}

				case BTSTACK_EVENT_DISCOVERABLE_ENABLED:
				{
					logMsg("got BTSTACK_EVENT_DISCOVERABLE_ENABLED"); break;
				}

				case HCI_EVENT_COMMAND_STATUS:
				{
					//logMsg("got HCI_EVENT_COMMAND_STATUS");
					break;
				}

				case HCI_EVENT_CONNECTION_COMPLETE:
				{
					uint16_t handle = READ_BT_16(packet, 3);
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[5]);
					uint32_t status = packet[2];
					logMsg("got HCI_EVENT_CONNECTION_COMPLETE: addr: %s, handle: %d, status: %d", bd_addr_to_str(addr), handle, status);
					if(cmdActive && activeCmd.cmd == BtstackCmd::CREATE_L2CAP && BD_ADDR_CMP(activeCmd.createChannelData.address.data(), addr) == 0)
					{
						// update handle
						auto sock = BtstackBluetoothSocket::findSocket(addr, activeCmd.createChannelData.channel);
						if(!sock)
						{
							logErr("can't find socket");
							break;
						}
						sock->handle = handle;
					}
					break;
				}

				case HCI_EVENT_DISCONNECTION_COMPLETE:
				{
					uint16_t handle = READ_BT_16(packet, 3);
					logMsg("got HCI_EVENT_DISCONNECTION_COMPLETE: handle: %d", handle);
					if(cmdActive && activeCmd.cmd == BtstackCmd::CREATE_L2CAP)
					{
						bd_addr_t &btAddr = *((bd_addr_t*)activeCmd.createChannelData.address.data());
						auto sock = BtstackBluetoothSocket::findSocket(btAddr, activeCmd.createChannelData.channel);
						if(!sock)
						{
							logErr("can't find socket");
							break;
						}
						if(sock->handle != handle)
							break;
						logMsg("disconnection while l2cap open in progress");
						cmdActive = 0;
						sock->onStatus(*sock, BluetoothSocketState::ConnectError);
						/*if(defaultBtstackAdapter.onScanStatus())
							defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);*/
					}
					break;
				}

				case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
				{
					//logMsg("got HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS");
					break;
				}

				case L2CAP_EVENT_CREDITS:
				{
					//logMsg("got L2CAP_EVENT_CREDITS");
					break;
				}

				case HCI_EVENT_QOS_SETUP_COMPLETE:
				{
					//logMsg("got HCI_EVENT_QOS_SETUP_COMPLETE");
					break;
				}

				case RFCOMM_EVENT_CREDITS:
				{
					//logMsg("got RFCOMM_EVENT_CREDITS");
					break;
				}

				case HCI_EVENT_PIN_CODE_REQUEST:
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
					uint32_t size;
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
					break;
				}

				case HCI_EVENT_INQUIRY_RESULT:
				case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
				{
					uint32_t responses = scanResponses = packet[2];
					logMsg("got HCI_EVENT_INQUIRY_RESULT, %d responses", responses);
					for(auto i : iotaCount(responses))
					{
						bd_addr_t addr;
						bt_flip_addr(addr, &packet[3+i*6]);

						auto devClassPtr = &packet[3 + responses*(6+1+1+1) + i*3];
						std::array<uint8_t, 3> devClass{devClassPtr[0], devClassPtr[1], devClassPtr[2]};
						if(!bta.onScanDeviceClass(bta, devClass))
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
						if(scanDevList.isFull())
						{
							logMsg("max devices reached");
							break;
						}
						scanDevList.push_back(dev);
					}
					break;
				}

				case HCI_EVENT_INQUIRY_COMPLETE:
				{
					cmdActive = 0;
					logMsg("got HCI_EVENT_INQUIRY_COMPLETE");
					if(scanDevList.size())
					{
						// scan will complete after name requests
						logMsg("starting name requests");
						for(auto &e : scanDevList)
						{
							e.requestName();
						}
						bta.onScanStatus(bta, BluetoothScanState::Processing, scanDevList.size());
					}
					else
					{
						inDetect = 0;
						if(!scanResponses)
						{
							bta.onScanStatus(bta, BluetoothScanState::NoDevs, 0);
						}
						else
						{
							logMsg("no name requests needed, scan complete");
							bta.onScanStatus(bta, BluetoothScanState::Complete, 0);
						}
					}
					BtstackBluetoothAdapter::processCommands();
					scanResponses = 0;
					break;
				}

				case BTSTACK_EVENT_REMOTE_NAME_CACHED:
					if(!bta.useScanCache)
					{
						logMsg("ignoring cached name");
						break;
					}
					[[fallthrough]];
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
						bta.onScanDeviceName(bta, name, addr);
					}
					else
					{
						bta.onScanDeviceName(bta, nullptr, addr);
						logMsg("Failed to get name: page timeout");
					}

					// TODO: decouple from Bluetooth inquiry operations
					if(!inDetect)
					{
						cmdActive = 0;
						scanDevList.clear();
						if(bta.onScanStatus)
							bta.onScanStatus(bta, BluetoothScanState::Cancelled, 0);
						BtstackBluetoothAdapter::processCommands();
						break;
					}

					bool removedFromScanList = IG::erase_if(scanDevList, [&](BTDevice &dev){ return BD_ADDR_CMP(dev.address, addr) == 0; });
					assert(removedFromScanList);

					if(!cached && !scanDevList.size())
					{
						logMsg("finished name requests, scan complete");
						inDetect = 0;
						bta.onScanStatus(bta, BluetoothScanState::Complete, 0);
					}
					BtstackBluetoothAdapter::processCommands();
					break;
				}

				case HCI_EVENT_LINK_KEY_NOTIFICATION:
				{
					logMsg("got HCI_EVENT_LINK_KEY_NOTIFICATION"); break;
				}

				case HCI_EVENT_LINK_KEY_REQUEST:
				{
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					logMsg("got HCI_EVENT_LINK_KEY_REQUEST from %s", bd_addr_to_str(addr));
					bt_send_cmd(&hci_link_key_request_negative_reply, &addr);
					break;
				}

				case L2CAP_EVENT_TIMEOUT_CHECK:
				{
					//logMsg("got L2CAP_EVENT_TIMEOUT_CHECK");
					break;
				}

				case HCI_EVENT_ENCRYPTION_CHANGE:
				{
					logMsg("got HCI_EVENT_ENCRYPTION_CHANGE"); break;
				}

				case HCI_EVENT_MAX_SLOTS_CHANGED:
				{
					logMsg("got HCI_EVENT_MAX_SLOTS_CHANGED"); break;
				}

				case HCI_EVENT_COMMAND_COMPLETE:
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
					break;
				}

				case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
				{
					logMsg("got RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE ch 0x%02X", (int)channel);
					cmdActive = 0;
					BtstackBluetoothSocket::handleRfcommChannelOpened(packet_type, channel, packet, size);
					BtstackBluetoothAdapter::processCommands();
					break;
				}

				case L2CAP_EVENT_CHANNEL_OPENED:
				{
					logMsg("got L2CAP_EVENT_CHANNEL_OPENED ch 0x%02X", (int)channel);
					cmdActive = 0;
					BtstackBluetoothSocket::handleL2capChannelOpened(packet_type, channel, packet, size);
					BtstackBluetoothAdapter::processCommands();
					break;
				}

				case RFCOMM_EVENT_CHANNEL_CLOSED:
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
					sock->onStatus(*sock, BluetoothSocketState::ReadError);
					break;
				}

				case L2CAP_EVENT_SERVICE_REGISTERED:
				{
					cmdActive = 0;
					uint8_t status = packet[2];
					uint32_t psm = READ_BT_16(packet, 3);
					auto onResult = std::exchange(setL2capServiceOnResult, {});
					if(status && status != L2CAP_SERVICE_ALREADY_REGISTERED)
					{
						logErr("error %d registering psm %d", status, psm);
						onResult(bta, BluetoothScanState::InitFailed, 0);
						break;
					}
					logMsg("registered l2cap service for psm 0x%X", psm);
					onResult(bta, BluetoothScanState::Complete, 0);
					BtstackBluetoothAdapter::processCommands();
					break;
				}

				case L2CAP_EVENT_INCOMING_CONNECTION:
				{
					uint16_t psm = READ_BT_16(packet, 10);
					uint16_t sourceCid = READ_BT_16(packet, 12);
					bd_addr_t addr;
					bt_flip_addr(addr, &packet[2]);
					BluetoothPendingSocket pending {0, addr, psm, sourceCid};
					bta.onIncomingL2capConnection(bta, pending);
					break;
				}

				default:
					logMsg("unhandled HCI event type 0x%X", packet[0]);
			}
			break;
		}

		case DAEMON_EVENT_PACKET:
		//logMsg("got DAEMON_EVENT_PACKET");
			break;

		default:
			logMsg("unhandled packet type 0x%X", packet_type);
	}
	//logMsg("end packet");
}

void BluetoothAdapter::setL2capService(uint32_t psm, bool on, OnStatusDelegate onResult)
{
	if(on)
	{
		logMsg("registering l2cap service for psm 0x%X", psm);
		assert(!setL2capServiceOnResult); // only handle one request at a time
		setL2capServiceOnResult = onResult;
		pendingCmdList.emplace_back(BtstackCmd::l2capRegisterService(psm, 672));
		if(isInactive())
		{
			setActiveState(true,
				[this](BluetoothAdapter &, State newState)
				{
					if(newState != BluetoothState::On)
					{
						auto onResult = std::exchange(setL2capServiceOnResult, {});
						onResult(*this, BluetoothScanState::InitFailed, 0);
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

BluetoothAdapter::State BluetoothAdapter::state()
{
	switch(state_)
	{
		case HCI_STATE_OFF:
		case HCI_STATE_SLEEPING:
			return BluetoothState::Off;
		case HCI_STATE_INITIALIZING:
			return BluetoothState::TurningOn;
		case HCI_STATE_WORKING:
			return BluetoothState::On;
		case HCI_STATE_HALTING:
		case HCI_STATE_FALLING_ASLEEP:
			return BluetoothState::TurningOff;
	}
	logWarn("unknown bluetooth state: %d", state_);
	return BluetoothState::Off;
}

void BluetoothAdapter::setActiveState(bool on, OnStateChangeDelegate onStateChange)
{
	if(onStateChange)
	{
		logErr("state change already in progress");
		return;
	}
	if(on)
	{
		if(isInactive())
		{
			logMsg("powering on Bluetooth");
			this->onStateChange = onStateChange;
			bt_send_cmd(&btstack_set_power_mode, HCI_POWER_ON);
		}
		else if(onStateChange) // already on
		{
			logMsg("Bluetooth is already on");
			onStateChange(*this, BluetoothState::On);
		}
	}
	else
	{
		// TODO
		bug_unreachable("TODO");
		onStateChange(*this, BluetoothState::Error);
	}
}

bool BluetoothAdapter::startScan(OnStatusDelegate onResult, OnScanDeviceClassDelegate onDeviceClass, OnScanDeviceNameDelegate onDeviceName)
{
	if(!inDetect)
	{
		inDetect = 1;
		onScanStatus = onResult;
		onScanDeviceClass = onDeviceClass;
		onScanDeviceName = onDeviceName;
		pendingCmdList.emplace_back(BtstackCmd::inquiry(scanSecs));
		logMsg("starting inquiry");
		if(isInactive())
		{
			setActiveState(true,
				[this](BluetoothAdapter &, State newState)
				{
					if(newState != BluetoothState::On)
					{
						onScanStatus(*this, BluetoothScanState::InitFailed, 0);
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

bool BluetoothAdapter::isInScan() const { return inDetect; }

bool BtstackBluetoothAdapter::isInactive()
{
	return state_ != HCI_STATE_INITIALIZING && state_ != HCI_STATE_WORKING;
}

bool BluetoothAdapter::openDefault()
{
	if(isOpen())
	{
		return true;
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
		return false;
	}
	btaPtr = this;
	bt_register_packet_handler(btHandler);
	logMsg("BTStack init");
	return true;
}

bool BluetoothAdapter::isOpen() const
{
	return btaPtr;
}

void BluetoothAdapter::cancelScan()
{
	if(inDetect)
	{
		// check if scan is queued
		bool wasQueued = IG::erase_if(pendingCmdList, [&](BtstackCmd &cmd){ return cmd.cmd == BtstackCmd::INQUIRY; });
		if(wasQueued)
		{
			logMsg("cancelling scan from queue");
			inDetect = 0;
			onScanStatus(*this, BluetoothScanState::Cancelled, 0);
		}
		else
		{
			logMsg("cancelling scan in progress");
			bt_send_cmd(&hci_inquiry_cancel);
			//BtstackBluetoothAdapter::processCommands();
		}

		inDetect = 0;
	}
}

void BluetoothAdapter::close()
{
	if(isOpen())
	{
		logMsg("closing BTstack");
		cancelScan();
		bt_close();
		state_ = HCI_STATE_OFF;
		btaPtr = {};
	}
}

void BluetoothAdapter::requestName(BluetoothPendingSocket &pending, OnScanDeviceNameDelegate onDeviceName)
{
	onScanDeviceName = onDeviceName;
	pendingCmdList.emplace_back(BtstackCmd::remoteNameRequest(pending.addr, 0, 0));
	BtstackBluetoothAdapter::processCommands();
}

void BluetoothPendingSocket::requestName(BluetoothAdapter& bta, BTOnScanDeviceNameDelegate onDeviceName)
{
	assert(ch);
	bta.requestName(*this, onDeviceName);
}

void BluetoothPendingSocket::close()
{
	assert(ch);
	logMsg("declining L2CAP connection %d", localCh);
	bt_send_cmd(&l2cap_decline_connection, localCh, 0);
	ch = 0;
}

std::system_error BluetoothSocket::openRfcomm(BluetoothAdapter &, BluetoothAddr addr, uint32_t channel)
{
	type = 1;
	if(socketList.isFull())
	{
		logMsg("no space left in socket list");
		return {ENOSPC, std::generic_category()};
	}
	socketList.emplace_back(this);
	logMsg("creating RFCOMM channel %d socket", channel);
	pendingCmdList.emplace_back(BtstackCmd::writeAuthenticationEnable(1));
	pendingCmdList.emplace_back(BtstackCmd::rfcommCreateChannel(addr, channel));
	this->addr = addr;
	ch = channel;
	BtstackBluetoothAdapter::processCommands();
	return std::error_code{};
}

std::system_error BluetoothSocket::openL2cap(BluetoothAdapter &, BluetoothAddr addr, uint32_t psm)
{
	type = 0;
	if(socketList.isFull())
	{
		logMsg("no space left in socket list");
		return {ENOSPC, std::generic_category()};
	}
	socketList.emplace_back(this);
	if(inL2capSocketOpenHandler)
	{
		// hack to boost command priority when opening 2nd Wiimote channel
		pendingCmdList.emplace_back(BtstackCmd::l2capCreateChannel(addr, psm));
	}
	else
	{
		pendingCmdList.emplace_back(BtstackCmd::writeAuthenticationEnable(0));
		pendingCmdList.emplace_back(BtstackCmd::l2capCreateChannel(addr, psm));
	}
	this->addr = addr;
	ch = psm;
	BtstackBluetoothAdapter::processCommands();
	return std::error_code{};
}

std::system_error BluetoothSocket::open(BluetoothAdapter &, BluetoothPendingSocket &pending)
{
	assert(pending);
	addr = pending.addr;
	type = pending.type;
	ch = pending.ch;
	localCh = pending.localCh;
	if(socketList.isFull())
	{
		logMsg("no space left in socket list");
		return {ENOSPC, std::generic_category()};
	}
	socketList.emplace_back(this);
	pendingCmdList.emplace_back(BtstackCmd::l2capAcceptConnection(localCh));
	pending = {};
	BtstackBluetoothAdapter::processCommands();
	return std::error_code{};
}

static bool btAddrIsEqual(BluetoothAddr addr1, const bd_addr_t addr2)
{
	return memcmp(addr1.data(), addr2, 6) == 0;
}

BluetoothSocket* BtstackBluetoothSocket::findSocket(uint16_t localCh)
{
	for(auto e : socketList)
	{
		if(e->localCh == localCh)
		{
			return e;
		}
	}
	return nullptr;
}

BluetoothSocket* BtstackBluetoothSocket::findSocket(const bd_addr_t addr, uint16_t ch)
{
	for(auto e : socketList)
	{
		if(e->ch == ch && btAddrIsEqual(e->addr, addr))
		{
			return e;
		}
	}
	return nullptr;
}

BluetoothSocket* BtstackBluetoothSocket::findSocket(const bd_addr_t addr)
{
	for(auto e : socketList)
	{
		if(btAddrIsEqual(e->addr, addr))
		{
			return e;
		}
	}
	return nullptr;
}

const void *BtstackBluetoothSocket::pin(uint32_t &size)
{
	size = pinSize;
	return pinCode;
}

void BtstackBluetoothSocket::setPin(const void *pin, uint32_t size)
{
	pinCode = pin;
	pinSize = size;
}

void BtstackBluetoothSocket::handleRfcommChannelOpened([[maybe_unused]] uint8_t packet_type, [[maybe_unused]] uint16_t channel, uint8_t *packet, [[maybe_unused]] uint16_t size)
{
	int ch = packet[11];
	bd_addr_t addr;
	bt_flip_addr(addr, &packet[3]);
	logMsg("handle RFCOMM channel open ch %d from %s", ch, bd_addr_to_str(addr));
	auto sock = findSocket(addr, ch);
	if(!sock)
	{
		logErr("can't find socket");
		return;
	}
	if(packet[2] == 0)
	{
		uint16_t rfcommCh = READ_BT_16(packet, 12);
		uint16_t handle = READ_BT_16(packet, 9);
		logMsg("rfcomm ch %d, handle %d", rfcommCh, handle);
		sock->localCh = rfcommCh;
		sock->handle = handle;
		if(sock->onStatus(*sock, BluetoothSocketState::Opened) == 1)
		{

		}
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus(*sock, BluetoothSocketState::ConnectError);
		/*if(defaultBtstackAdapter.onScanStatus())
			defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);*/
	}
}

void BtstackBluetoothSocket::handleL2capChannelOpened([[maybe_unused]] uint8_t packet_type, [[maybe_unused]] uint16_t channel, uint8_t *packet, [[maybe_unused]] uint16_t size)
{
	uint16_t psm = READ_BT_16(packet, 11);
	bd_addr_t addr;
	bt_flip_addr(addr, &packet[3]);
	logMsg("handle L2CAP channel open psm %d from %s", psm, bd_addr_to_str(addr));
	auto sock = findSocket(addr, psm);
	if(!sock)
	{
		logErr("can't find socket");
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
		if(sock->onStatus(*sock, BluetoothSocketState::Opened) == 1)
		{

		}
		inL2capSocketOpenHandler = 0;
	}
	else
	{
		logMsg("failed. status code %u\n", packet[2]);
		sock->onStatus(*sock, BluetoothSocketState::ConnectError);
		/*if(defaultBtstackAdapter.onScanStatus())
			defaultBtstackAdapter.onScanStatus()(defaultBtstackAdapter, BluetoothAdapter::SOCKET_OPEN_FAILED, 0);*/
	}
}

BtstackBluetoothSocket::~BtstackBluetoothSocket()
{
	close();
}

void BtstackBluetoothSocket::close()
{
	if(localCh)
	{
		logMsg("closing BT handle %d", handle);
		bt_send_cmd(&hci_disconnect, handle, 0x13);
		/*if(type)
			bt_send_cmd(&rfcomm_disconnect, localCh, (uint8_t)0);
		else
			bt_send_cmd(&l2cap_disconnect, localCh, (uint8_t)0);*/
		handle = 0;
		localCh = 0;
	}
	IG::eraseFirst(socketList, this);
}

ssize_t BluetoothSocket::write(const void *data, size_t size)
{
	assert(localCh);
	if(type)
		bt_send_rfcomm(localCh, (uint8_t*)data, size);
	else
		bt_send_l2cap(localCh, (uint8_t*)data, size);
	return size;
}

}
