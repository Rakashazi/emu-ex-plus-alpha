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

#define thisModuleName "zeemote"
#include "Zeemote.hh"
#include <base/Base.hh>
#include <util/bits.h>
#include <util/collection/DLList.hh>
#include <algorithm>

extern StaticDLList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticDLList<Zeemote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> Zeemote::devList;

const uchar Zeemote::btClass[3] = { 0x84, 0x05, 0x00 };

uint Zeemote::findFreeDevId()
{
	uint id[5] = { 0 };
	forEachInDLList(&devList, e)
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

CallResult Zeemote::open(BluetoothAdapter &adapter)
{
	logMsg("connecting to Zeemote");
	sock.onData() =
		[this](const uchar *packet, size_t size)
		{
			return dataHandler(packet, size);
		};
	sock.onStatus() =
		[this](BluetoothSocket &sock, uint status)
		{
			return statusHandler(sock, status);
		};
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	sock.setPin("0000", 4);
	#endif
	if(sock.openRfcomm(addr, 1) != OK)
	{
		logErr("error opening socket");
		return IO_ERROR;
	}

	return OK;
}

void Zeemote::close()
{
	sock.close();
}

void Zeemote::removeFromSystem()
{
	close();
	devList.remove(this);
	if(btInputDevList.remove(this))
	{
		Input::removeDevice((Input::Device){player, Input::Event::MAP_ZEEMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Zeemote"});
		Input::onInputDevChange((Input::DeviceChange){ player, Input::Event::MAP_ZEEMOTE, Input::DeviceChange::REMOVED });
	}
}

uint Zeemote::statusHandler(BluetoothSocket &sock, uint status)
{
	if(status == BluetoothSocket::STATUS_OPENED)
	{
		logMsg("Zeemote opened successfully");
		player = findFreeDevId();
		if(!devList.add(this) || !btInputDevList.add(this))
		{
			logErr("No space left in BT input device list");
			removeFromSystem();
			delete this;
			return 0;
		}
		Input::addDevice((Input::Device){player, Input::Event::MAP_ZEEMOTE, Input::Device::TYPE_BIT_GAMEPAD, "Zeemote"});
		device = Input::devList.last();
		Input::onInputDevChange((Input::DeviceChange){ player, Input::Event::MAP_ZEEMOTE, Input::DeviceChange::ADDED });
		return BluetoothSocket::REPLY_OPENED_USE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_ERROR)
	{
		logErr("Zeemote read error, disconnecting");
		removeFromSystem();
		delete this;
	}
	return 0;
}

bool Zeemote::dataHandler(const uchar *packet, size_t size)
{
	//logMsg("%d bytes ready", size);
	uint bytesLeft = size;
	do
	{
		uint processBytes = std::min(bytesLeft, packetSize - inputBufferPos);
		memcpy(&inputBuffer[inputBufferPos], &packet[size-bytesLeft], processBytes);
		if(inputBufferPos == 0) // get data size
		{
			packetSize = inputBuffer[0] + 1;
			//logMsg("got packet size %d", packetSize);
		}
		//logDMsg("read %d bytes from Zeemote", len);

		if(packetSize > sizeof(inputBuffer))
		{
			logErr("can't handle packet, closing Zeemote");
			removeFromSystem();
			delete this;
			return 0;
		}

		inputBufferPos += processBytes;
		assert(inputBufferPos <= sizeof(inputBuffer));

		// check if inputBuffer is complete
		if(inputBufferPos == packetSize)
		{
			uint rID = inputBuffer[2];
			logMsg("report id 0x%X, %s", rID, reportIDToStr(rID));
			switch(rID)
			{
				bcase RID_BTN_REPORT:
				{
					const uchar *key = &inputBuffer[3];
					logMsg("got button report %X %X %X %X %X %X", key[0], key[1], key[2], key[3], key[4], key[5]);
					processBtnReport(key, player);
				}
				bcase RID_8BA_2A_JS_REPORT:
					logMsg("got analog report %d %d", (schar)inputBuffer[4], (schar)inputBuffer[5]);
					processStickDataForButtonEmulation((schar*)&inputBuffer[4], player);
			}
			inputBufferPos = 0;
		}
		bytesLeft -= processBytes;
	} while(bytesLeft);

	return 1;
}

const char *Zeemote::reportIDToStr(uint id)
{
	switch(id)
	{
		case RID_VERSION: return "Version Report";
		case RID_BTN_METADATA: return "Button Metadata";
		case RID_CONFIG_DATA: return "Configuration Data";
		case RID_BTN_REPORT: return "Button Report";
		case RID_8BA_2A_JS_REPORT: return "8-bit Analog 2-Axis Joystick Report";
		case RID_BATTERY_REPORT: return "Battery Report";
	}
	return "Unknown";
}

void Zeemote::processBtnReport(const uchar *btnData, uint player)
{
	using namespace Input;
	uchar btnPush[4] = { 0 };
	iterateTimes(4, i)
	{
		if(btnData[i] >= 4)
			break;
		btnPush[btnData[i]] = 1;
	}
	iterateTimes(4, i)
	{
		if(prevBtnPush[i] != btnPush[i])
		{
			bool newState = btnPush[i];
			uint code = i + 1;
			//logMsg("%s %s @ Zeemote", e->name, newState ? "pushed" : "released");
			onInputEvent(Event(player, Event::MAP_ZEEMOTE, code, newState ? PUSHED : RELEASED, 0, device));
		}
	}
	memcpy(prevBtnPush, btnPush, sizeof(prevBtnPush));
}


void Zeemote::processStickDataForButtonEmulation(const schar *pos, int player)
{
	using namespace Input;
	//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
	forEachInArray(stickBtn, e)
	{
		bool newState;
		switch(e_i)
		{
			case 0: newState = pos[0] < -63; break;
			case 1: newState = pos[0] > 63; break;
			case 2: newState = pos[1] > 63; break;
			case 3: newState = pos[1] < -63; break;
			default: bug_branch("%d", (int)e_i); return;
		}
		if(*e != newState)
		{
			static const uint btnEvent[] =
			{
				Input::Zeemote::LEFT, Input::Zeemote::RIGHT, Input::Zeemote::DOWN, Input::Zeemote::UP,
			};
			onInputEvent(Event(player, Event::MAP_ZEEMOTE, btnEvent[e_i], newState ? PUSHED : RELEASED, 0, device));
		}
		*e = newState;
	}
}
