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

#define LOGTAG "Zeemote"
#include <imagine/bluetooth/Zeemote.hh>
#include <imagine/base/Base.hh>
#include <imagine/util/bits.h>
#include <algorithm>
#include "../input/private.hh"

extern StaticArrayList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticArrayList<Zeemote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> Zeemote::devList;

const uchar Zeemote::btClass[3] = { 0x84, 0x05, 0x00 };

static const Input::Key sysKeyMap[4]
{
	Input::Keycode::GAME_A,
	Input::Keycode::GAME_B,
	Input::Keycode::GAME_C,
	Input::Keycode::MENU
};

static const char *zeemoteButtonName(Input::Key k)
{
	switch(k)
	{
		case 0: return "None";
		case Input::Zeemote::A: return "A";
		case Input::Zeemote::B: return "B";
		case Input::Zeemote::C: return "C";
		case Input::Zeemote::POWER: return "Power";
		case Input::Zeemote::UP: return "Up";
		case Input::Zeemote::RIGHT: return "Right";
		case Input::Zeemote::DOWN: return "Down";
		case Input::Zeemote::LEFT: return "Left";
	}
	return "Unknown";
}

const char *Zeemote::keyName(Input::Key k) const
{
	return zeemoteButtonName(k);
}

uint Zeemote::findFreeDevId()
{
	uint id[5] = { 0 };
	for(auto e : devList)
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
		[this](const char *packet, size_t size)
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
		Input::removeDevice(*this);
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::REMOVED });
	}
}

uint Zeemote::statusHandler(BluetoothSocket &sock, uint status)
{
	if(status == BluetoothSocket::STATUS_OPENED)
	{
		logMsg("Zeemote opened successfully");
		player = findFreeDevId();
		if(devList.isFull() || btInputDevList.isFull() || Input::devList.isFull())
		{
			logErr("No space left in BT input device list");
			removeFromSystem();
			delete this;
			return 0;
		}
		devList.push_back(this);
		btInputDevList.push_back(this);
		devId = player;
		Input::addDevice(*this);
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::ADDED });
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("Zeemote connection error");
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::CONNECT_ERROR });
		close();
		delete this;
	}
	else if(status == BluetoothSocket::STATUS_READ_ERROR)
	{
		logErr("Zeemote read error, disconnecting");
		removeFromSystem();
		delete this;
	}
	return 0;
}

bool Zeemote::dataHandler(const char *packet, size_t size)
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
					//processStickDataForButtonEmulation((schar*)&inputBuffer[4], player);
					iterateTimes(2, i)
					{
						if(axisKey[i].dispatch(inputBuffer[4+i], player, Input::Event::MAP_ZEEMOTE, *this, Base::mainWindow()))
							Base::endIdleByUserActivity();
					}
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
	uchar btnPush[4] {0};
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
			Base::endIdleByUserActivity();
			Event event{player, Event::MAP_ZEEMOTE, (Key)code, sysKeyMap[i], newState ? PUSHED : RELEASED, 0, 0, this};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevBtnPush, btnPush, sizeof(prevBtnPush));
}


//void Zeemote::processStickDataForButtonEmulation(const schar *pos, int player)
//{
//	using namespace Input;
//	//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
//	forEachInArray(stickBtn, e)
//	{
//		bool newState;
//		switch(e_i)
//		{
//			case 0: newState = pos[0] < -63; break;
//			case 1: newState = pos[0] > 63; break;
//			case 2: newState = pos[1] > 63; break;
//			case 3: newState = pos[1] < -63; break;
//			default: bug_branch("%d", (int)e_i); return;
//		}
//		if(*e != newState)
//		{
//			static const uint btnEvent[] =
//			{
//				Input::Zeemote::LEFT, Input::Zeemote::RIGHT, Input::Zeemote::DOWN, Input::Zeemote::UP,
//			};
//			Base::endIdleByUserActivity();
//			Event event{(uint)player, Event::MAP_ZEEMOTE, (Key)btnEvent[e_i], newState ? PUSHED : RELEASED, 0, 0, this};
//			startKeyRepeatTimer(event);
//			dispatchInputEvent(event);
//		}
//		*e = newState;
//	}
//}
