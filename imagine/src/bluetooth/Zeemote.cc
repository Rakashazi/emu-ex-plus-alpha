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
#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/algorithm.h>
#include <algorithm>
#include "../input/PackedInputAccess.hh"
#include "private.hh"

std::vector<Zeemote*> Zeemote::devList;

const uint8_t Zeemote::btClass[3] = { 0x84, 0x05, 0x00 };

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
	return "";
}

const char *Zeemote::keyName(Input::Key k) const
{
	return zeemoteButtonName(k);
}

uint32_t Zeemote::findFreeDevId()
{
	uint32_t id[5]{};
	for(auto e : devList)
	{
		id[e->player] = 1;
	}
	for(auto &e : id)
	{
		if(e == 0)
			return &e - id;
	}
	logMsg("too many devices");
	return 0;
}

IG::ErrorCode Zeemote::open(BluetoothAdapter &adapter)
{
	logMsg("connecting to Zeemote");
	sock.onData() =
		[this](const char *packet, size_t size)
		{
			return dataHandler(packet, size);
		};
	sock.onStatus() =
		[this](BluetoothSocket &sock, uint32_t status)
		{
			return statusHandler(sock, status);
		};
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	sock.setPin("0000", 4);
	#endif
	if(auto err = sock.openRfcomm(adapter, addr, 1);
		err)
	{
		logErr("error opening socket");
		return err;
	}
	return {};
}

void Zeemote::close()
{
	sock.close();
}

void Zeemote::removeFromSystem()
{
	close();
	IG::eraseFirst(devList, this);
	if(IG::eraseFirst(btInputDevList, this))
	{
		ctx.application().removeSystemInputDevice(*this, true);
	}
}

uint32_t Zeemote::statusHandler(BluetoothSocket &sock, uint32_t status)
{
	if(status == BluetoothSocket::STATUS_OPENED)
	{
		logMsg("Zeemote opened successfully");
		player = findFreeDevId();
		devList.push_back(this);
		btInputDevList.push_back(this);
		devId = player;
		ctx.application().addSystemInputDevice(*this, true);
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("Zeemote connection error");
		ctx.application().dispatchInputDeviceChange(*this, {Input::DeviceAction::CONNECT_ERROR});
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
	uint32_t bytesLeft = size;
	do
	{
		uint32_t processBytes = std::min(bytesLeft, packetSize - inputBufferPos);
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
			auto time = IG::steadyClockTimestamp();
			uint32_t rID = inputBuffer[2];
			logMsg("report id 0x%X, %s", rID, reportIDToStr(rID));
			switch(rID)
			{
				bcase RID_BTN_REPORT:
				{
					const uint8_t *key = &inputBuffer[3];
					logMsg("got button report %X %X %X %X %X %X", key[0], key[1], key[2], key[3], key[4], key[5]);
					processBtnReport(key, time, player);
				}
				bcase RID_8BA_2A_JS_REPORT:
					logMsg("got analog report %d %d", (int8_t)inputBuffer[4], (int8_t)inputBuffer[5]);
					//processStickDataForButtonEmulation((int8_t*)&inputBuffer[4], player);
					iterateTimes(2, i)
					{
						if(axisKey[i].dispatch(inputBuffer[4+i], player, Input::Map::ZEEMOTE, time, *this, ctx.mainWindow()))
							ctx.endIdleByUserActivity();
					}
			}
			inputBufferPos = 0;
		}
		bytesLeft -= processBytes;
	} while(bytesLeft);

	return 1;
}

const char *Zeemote::reportIDToStr(uint32_t id)
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

void Zeemote::processBtnReport(const uint8_t *btnData, Input::Time time, uint32_t player)
{
	using namespace Input;
	uint8_t btnPush[4] {0};
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
			uint32_t code = i + 1;
			//logMsg("%s %s @ Zeemote", e->name, newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			Event event{player, Map::ZEEMOTE, (Key)code, sysKeyMap[i], newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
			ctx.application().dispatchRepeatableKeyInputEvent( event);
		}
	}
	memcpy(prevBtnPush, btnPush, sizeof(prevBtnPush));
}

bool Zeemote::isSupportedClass(const uint8_t devClass[3])
{
	return IG::equal_n(devClass, 3, btClass);
}
