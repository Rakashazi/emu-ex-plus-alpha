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

#define thisModuleName "wiimote"
#include "Wiimote.hh"
#include <base/Base.hh>
#include <util/bits.h>
#include <util/cLang.h>
#include <util/collection/DLList.hh>

const uchar Wiimote::btClass[3] = { 0x04, 0x25, 0x00 };
const uchar Wiimote::btClassDevOnly[3] = { 0x04, 0x05, 0x00 };
const uchar Wiimote::btClassRemotePlus[3] = { 0x08, 0x05, 0x00 };
static const char ccDataBytes = 6;
static const char nunchuckDataBytes = 6;
static const char proDataBytes = 10;

extern StaticDLList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticDLList<Wiimote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> Wiimote::devList;

uint Wiimote::findFreeDevId()
{
	uint id[5] = { 0 };
	forEachInDLList(&Wiimote::devList, e)
	{
		id[e->player] = 1;
	}
	forEachInArray(id, e)
	{
		if(*e == 0)
			return e_i;
	}
	logMsg("too many devices");
	return 0;
}

CallResult Wiimote::open(BluetoothAdapter &adapter)
{
	logMsg("opening Wiimote");
	ctlSock.onData() = intSock.onData() =
		[this](const uchar *packet, size_t size)
		{
			return dataHandler(packet, size);
		};
	ctlSock.onStatus() = intSock.onStatus() =
		[this](BluetoothSocket &sock, uint status)
		{
			return statusHandler(sock, status);
		};
	if(ctlSock.openL2cap(addr, 17) != OK)
	{
		logErr("error opening control socket");
		return IO_ERROR;
	}
	return OK;
}

uint Wiimote::statusHandler(BluetoothSocket &sock, uint status)
{
	if(status == BluetoothSocket::STATUS_OPENED && &sock == (BluetoothSocket*)&ctlSock)
	{
		logMsg("opened Wiimote control socket, opening interrupt socket");
		intSock.openL2cap(addr, 19);
		return 0; // don't add ctlSock to event loop
	}
	else if(status == BluetoothSocket::STATUS_OPENED && &sock == (BluetoothSocket*)&intSock)
	{
		logMsg("Wiimote opened successfully");
		player = findFreeDevId();
		if(!devList.add(this) || !btInputDevList.add(this) || Input::devList.isFull())
		{
			logErr("No space left in BT input device list");
			removeFromSystem();
			delete this;
			return 1;
		}
		setLEDs(player);
		requestStatus();
		Input::addDevice((Input::Device){player, Input::Event::MAP_WIIMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Wiimote"});
		device = Input::devList.last();
		return BluetoothSocket::REPLY_OPENED_USE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_ERROR)
	{
		logErr("Wiimote read error, disconnecting");
		removeFromSystem();
		delete this;
	}
	return 1;
}

void Wiimote::close()
{
	intSock.close();
	ctlSock.close();
	extension = EXT_NONE;
	function = FUNC_NONE;
}

void Wiimote::removeFromSystem()
{
	close();
	devList.remove(this);
	if(btInputDevList.remove(this))
	{
		if(subDevice)
		{
			Input::removeDevice((Input::Device){player, subDevice->map(), Input::Device::TYPE_BIT_GAMEPAD, subDevice->name()});
		}
		auto map = device->map();
		Input::removeDevice((Input::Device){player, map, Input::Device::TYPE_BIT_GAMEPAD, device->name()});
		Input::onInputDevChange((Input::DeviceChange){ player, map, Input::DeviceChange::REMOVED });
	}
}

void Wiimote::writeReg(uchar offset, uchar val)
{
	uchar toWrite[23] = { 0xa2, 0x16, 0x04, 0xA4, 0x00, offset, 0x01, val }; // extra 15 bytes padding
	intSock.write(toWrite, sizeof(toWrite));
}

void Wiimote::readReg(uint offset, uchar size)
{
	uchar toRead[8] = { 0xa2, 0x17, 0x04,
			uchar((offset & 0xFF0000) >> 16), uchar((offset & 0xFF00) >> 8), uchar(offset & 0xFF), 0x00, size };
	logMsg("read reg %X %X %X", toRead[3], toRead[4], toRead[5]);
	intSock.write(toRead, sizeof(toRead));
}

void Wiimote::setLEDs(uint player)
{
	logMsg("setting LEDs for player %d", player);
	const uchar setLEDs[] = { 0xa2, 0x11, playerLEDs(player) };
	intSock.write(setLEDs, sizeof(setLEDs));
}

void Wiimote::requestStatus()
{
	logMsg("requesting status");
	const uchar reqStatus[] = { 0xa2, 0x15, 0x00 };
	intSock.write(reqStatus, sizeof(reqStatus));
}

void Wiimote::sendDataMode(uchar mode)
{
	logMsg("setting mode 0x%X", mode);
	uchar setMode[] = { 0xa2, 0x12, 0x00, mode };
	intSock.write(setMode, sizeof(setMode));
}

void Wiimote::initExtension()
{
	writeReg(0xF0, 0x55);
	function = FUNC_INIT_EXT;
}

void Wiimote::initExtensionPart2()
{
	writeReg(0xFB, 0x00);
	function = FUNC_INIT_EXT_DONE;
}

void Wiimote::sendDataModeByExtension()
{
	switch(extension)
	{
		bcase EXT_CC:
		case EXT_NUNCHUK:
			sendDataMode(0x32);
		bcase EXT_WIIU_PRO:
			sendDataMode(0x34);
		bdefault:
			sendDataMode(0x30);
	}
}

bool Wiimote::dataHandler(const uchar *packet, size_t size)
{
	if(unlikely(packet[0] != 0xa1))
	{
		logWarn("Unknown report in Wiimote packet");
		return 1;
	}
	switch(packet[1])
	{
		bcase 0x30:
		{
			//logMsg("got core report");
			assert(device);
			processCoreButtons(packet, player);
		}

		bcase 0x32:
		{
			//logMsg("got core+extension report");
			assert(device);
			processCoreButtons(packet, player);
			switch(extension)
			{
				bcase EXT_CC:
					processClassicButtons(packet, player);
				bcase EXT_NUNCHUK:
					processNunchukButtons(packet, player);
			}
		}

		bcase 0x34:
		{
			//logMsg("got core+extension19 report");
			assert(device);
			processProButtons(packet, player);
		}

		bcase 0x20:
		{
			logMsg("got status report, bits 0x%X", packet[4]);
			if(extension && !(packet[4] & BIT(1)))
			{
				logMsg("extension disconnected");
				extension = EXT_NONE;
				sendDataMode(0x30);
				if(subDevice)
				{
					auto name = subDevice->name();
					auto map = subDevice->map();
					subDevice = nullptr;
					Input::removeDevice((Input::Device){player, map, Input::Device::TYPE_BIT_GAMEPAD, name});
					Input::onInputDevChange((Input::DeviceChange){ player, map, Input::DeviceChange::REMOVED });
				}
			}
			else if(!extension && (packet[4] & BIT(1)))
			{
				logMsg("extension connected");
				initExtension();
			}
			else
			{
				logMsg("no extension change");
				// set report mode
				sendDataModeByExtension();
				if(!identifiedType)
				{
					Input::onInputDevChange((Input::DeviceChange){ player, device->map(), Input::DeviceChange::ADDED });
					identifiedType = 1;
				}
			}
		}

		bcase 0x21:
		{
			logMsg("got read report from addr %X %X", packet[5], packet[6]);
			switch(function)
			{
				bcase FUNC_GET_EXT_TYPE:
				{
					// CCs can have 0 or 1 in first byte, check only last 5 bytes
					const uchar ccType[5] = { /*0x00,*/ 0x00, 0xA4, 0x20, 0x01, 0x01 };
					const uchar wiiUProType[5] = { /*0x00,*/ 0x00, 0xA4, 0x20, 0x01, 0x20 };
					const uchar nunchukType[6] = { 0x00, 0x00, 0xA4, 0x20, 0x00, 0x00 };
					logMsg("ext type: %X %X %X %X %X %X",
						packet[7], packet[8], packet[9], packet[10], packet[11], packet[12]);
					if(memcmp(&packet[8], ccType, sizeof(ccType)) == 0)
					{
						logMsg("extension is CC");
						if(Input::devList.isFull())
						{
							logMsg("ignoring CC because input list is full");
							extension = EXT_UNKNOWN;
							sendDataModeByExtension();
							return 1;
						}
						extension = EXT_CC;
						sendDataModeByExtension();
						memset(prevExtData, 0xFF, sizeof(prevExtData));
						mem_zero(stickBtn);
						Input::addDevice((Input::Device){player, Input::Event::MAP_WII_CC, Input::Device::TYPE_BIT_GAMEPAD, "Wii Classic Controller"});
						assert(!subDevice);
						subDevice = Input::devList.last();
						if(identifiedType)
							Input::onInputDevChange((Input::DeviceChange){ player, Input::Event::MAP_WII_CC, Input::DeviceChange::ADDED });
					}
					else if(memcmp(&packet[7], nunchukType, 6) == 0)
					{
						logMsg("extension is Nunchuk");
						extension = EXT_NUNCHUK;
						sendDataModeByExtension();
						memset(prevExtData, 0xFF, sizeof(prevExtData));
						mem_zero(stickBtn);
					}
					else if(memcmp(&packet[8], wiiUProType, sizeof(ccType)) == 0)
					{
						logMsg("extension is Wii U Pro");
						extension = EXT_WIIU_PRO;
						sendDataModeByExtension();
						memset(prevExtData, 0xFF, sizeof(prevExtData));
						mem_zero(stickBtn);
						device->setMap(Input::Event::MAP_WII_CC);
						device->name_ = "Wii U Pro Controller";
					}
					else
					{
						logMsg("unknown extension");
						extension = EXT_UNKNOWN;
						sendDataModeByExtension();
					}
					function = FUNC_NONE;

					if(!identifiedType)
					{
						Input::onInputDevChange((Input::DeviceChange){ player, device->map(), Input::DeviceChange::ADDED });
						identifiedType = 1;
					}
				}
			}
		}

		bcase 0x22:
		{
			logMsg("ack output report, %X %X", packet[4], packet[5]);
			switch(function)
			{
				bcase FUNC_INIT_EXT: initExtensionPart2();
				bcase FUNC_INIT_EXT_DONE:
					readReg(0xa400fa, 6);
					function = FUNC_GET_EXT_TYPE;
					logMsg("done extension init, getting type");
			}
		}

		bdefault:
		{
			logMsg("unhandled packet type %d from wiimote", packet[1]);
		}
	}
	return 1;
}

uchar Wiimote::playerLEDs(int player)
{
	switch(player)
	{
		default:
		case 0: return BIT(4);
		case 1: return BIT(5);
		case 2: return BIT(6);
		case 3: return BIT(7);
		case 4: return BIT(4) | BIT(7);
	}
}

void Wiimote::decodeCCSticks(const uchar *ccSticks, int &lX, int &lY, int &rX, int &rY)
{
	lX = ccSticks[0] & 0x3F;
	lY = ccSticks[1] & 0x3F;
	rX = (ccSticks[0] & 0xC0) >> 3 | (ccSticks[1] & 0xC0) >> 5 | (ccSticks[2] & 0x80) >> 7;
	rY = ccSticks[2] & 0x1F;
}

void Wiimote::decodeProSticks(const uchar *ccSticks, int &lX, int &lY, int &rX, int &rY)
{
	lX = ccSticks[0] | (ccSticks[1] << 8);
	lY = ccSticks[4] | (ccSticks[5] << 8);
	rX = ccSticks[2] | (ccSticks[3] << 8);
	rY = ccSticks[6] | (ccSticks[7] << 8);
}

void Wiimote::processStickDataForButtonEmulation(int player, const uchar *data)
{
	using namespace Input;
	int pos[4];
	decodeCCSticks(data, pos[0], pos[1], pos[2], pos[3]);
	//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
	forEachInArray(stickBtn, e)
	{
		bool newState;
		switch(e_i)
		{
			case 0: newState = pos[0] < 31-8; break;
			case 1: newState = pos[0] > 31+8; break;
			case 2: newState = pos[1] < 31-8; break;
			case 3: newState = pos[1] > 31+8; break;

			case 4: newState = pos[2] < 16-4; break;
			case 5: newState = pos[2] > 16+4; break;
			case 6: newState = pos[3] < 16-4; break;
			case 7: newState = pos[3] > 16+4; break;
			default: bug_branch("%d", (int)e_i); return;
		}
		if(*e != newState)
		{
			static const uint btnEvent[8] =
			{
				Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP,
				Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP
			};
			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
			onInputEvent(Event(player, Event::MAP_WII_CC, btnEvent[e_i], newState ? PUSHED : RELEASED, 0, subDevice));
		}
		*e = newState;
	}
}

void Wiimote::processCoreButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *btnData = &packet[2];
	forEachInArray(wiimoteDataAccess, e)
	{
		int newState = e->updateState(prevBtnData, btnData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, e->keyEvent), newState ? "pushed" : "released", player);
			onInputEvent(Event(player, Event::MAP_WIIMOTE, e->keyEvent, newState ? PUSHED : RELEASED, 0, device));
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

void Wiimote::processClassicButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *ccData = &packet[4];
	processStickDataForButtonEmulation(player, ccData);
	forEachInArray(wiimoteCCDataAccess, e)
	{
		int newState = e->updateState(prevExtData, ccData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wiimote cc", buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			onInputEvent(Event(player, Event::MAP_WII_CC, e->keyEvent, !newState ? PUSHED : RELEASED, 0, subDevice));
		}
	}
	memcpy(prevExtData, ccData, ccDataBytes);
}

void Wiimote::processProStickDataForButtonEmulation(int player, const uchar *data)
{
	using namespace Input;
	int pos[4];
	decodeProSticks(data, pos[0], pos[1], pos[2], pos[3]);
	//logMsg("Pro sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
	forEachInArray(stickBtn, e)
	{
		bool newState;
		switch(e_i)
		{
			case 0: newState = pos[0] < 2048-256; break;
			case 1: newState = pos[0] > 2048+256; break;
			case 2: newState = pos[1] < 2048-256; break;
			case 3: newState = pos[1] > 2048+256; break;
			case 4: newState = pos[2] < 2048-256; break;
			case 5: newState = pos[2] > 2048+256; break;
			case 6: newState = pos[3] < 2048-256; break;
			case 7: newState = pos[3] > 2048+256; break;
			default: bug_branch("%d", (int)e_i); return;
		}
		if(*e != newState)
		{
			static const uint btnEvent[8] =
			{
				Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP,
				Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP
			};
			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
			onInputEvent(Event(player, Event::MAP_WII_CC, btnEvent[e_i], newState ? PUSHED : RELEASED, 0, device));
		}
		*e = newState;
	}
}

void Wiimote::processProButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *proData = &packet[4];
	processProStickDataForButtonEmulation(player, proData);
	forEachInArray(wiimoteProDataAccess, e)
	{
		int newState = e->updateState(prevExtData, proData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wii u pro", buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			onInputEvent(Event(player, Event::MAP_WII_CC, e->keyEvent, !newState ? PUSHED : RELEASED, 0, device));
		}
	}
	memcpy(prevExtData, proData, proDataBytes);
}

void Wiimote::processNunchukStickDataForButtonEmulation(int player, const uchar *data)
{
	using namespace Input;
	int pos[2] = { data[0], data[1] };
	//logMsg("Nunchuk stick %dx%d", pos[0], pos[1]);
	forEachInArray(stickBtn, e)
	{
		bool newState;
		switch(e_i)
		{
			case 0: newState = pos[0] < 127-64; break;
			case 1: newState = pos[0] > 127+64; break;
			case 2: newState = pos[1] < 127-64; break;
			case 3: newState = pos[1] > 127+64; break;
			default: bug_branch("%d", (int)e_i); return;
		}
		if(*e != newState)
		{
			static const uint btnEvent[8] =
			{
				Input::Wiimote::NUN_STICK_LEFT, Input::Wiimote::NUN_STICK_RIGHT, Input::Wiimote::NUN_STICK_DOWN, Input::Wiimote::NUN_STICK_UP
			};
			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
			onInputEvent(Event(player, Event::MAP_WIIMOTE, btnEvent[e_i], newState ? PUSHED : RELEASED, 0, device));
		}
		*e = newState;

		if(e_i == 3)
			break; // only use half the array
	}
}

void Wiimote::processNunchukButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *nunData = &packet[4];
	processNunchukStickDataForButtonEmulation(player, nunData);
	forEachInArray(wiimoteNunchukDataAccess, e)
	{
		int newState = e->updateState(prevExtData, nunData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote nunchuk",buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			onInputEvent(Input::Event(player, Event::MAP_WIIMOTE, e->keyEvent, !newState ? PUSHED : RELEASED, 0, device));
		}
	}
	memcpy(prevExtData, nunData, nunchuckDataBytes);
}
