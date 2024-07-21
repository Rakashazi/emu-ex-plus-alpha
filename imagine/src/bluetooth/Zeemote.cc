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
#include <imagine/input/bluetoothInputDefs.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/ranges.hh>
#include <algorithm>
#include "../input/PackedInputAccess.hh"

namespace IG
{

constexpr Input::Key sysKeyMap[4]
{
	Input::ZeemoteKey::A,
	Input::ZeemoteKey::B,
	Input::ZeemoteKey::C,
	Input::ZeemoteKey::POWER
};

constexpr const char *zeemoteButtonName(Input::Key k)
{
	switch(k)
	{
		case 0: return "None";
		case Input::ZeemoteKey::A: return "A";
		case Input::ZeemoteKey::B: return "B";
		case Input::ZeemoteKey::C: return "C";
		case Input::ZeemoteKey::POWER: return "Power";
		case Input::ZeemoteKey::UP: return "Up";
		case Input::ZeemoteKey::RIGHT: return "Right";
		case Input::ZeemoteKey::DOWN: return "Down";
		case Input::ZeemoteKey::LEFT: return "Left";
	}
	return "";
}

Zeemote::Zeemote(ApplicationContext ctx, BluetoothAddr addr):
	BluetoothInputDevice{ctx, Input::Map::ZEEMOTE, {.gamepad = true}, "Zeemote"},
	sock{ctx},
	addr{addr} {}

const char *Zeemote::keyName(Input::Key k) const
{
	return zeemoteButtonName(k);
}

bool Zeemote::open(BluetoothAdapter& adapter, Input::Device& dev)
{
	logMsg("connecting to Zeemote");
	sock.onData =
		[&dev](const char *packet, size_t size)
		{
			return getAs<Zeemote>(dev).dataHandler(dev, packet, size);
		};
	sock.onStatus =
		[&dev](BluetoothSocket &sock, BluetoothSocketState status)
		{
			return getAs<Zeemote>(dev).statusHandler(dev, sock, status);
		};
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	sock.setPin("0000", 4);
	#endif
	if(auto err = sock.openRfcomm(adapter, addr, 1);
		err.code())
	{
		logErr("error opening socket");
		return false;
	}
	return true;
}

void Zeemote::close()
{
	sock.close();
}

uint32_t Zeemote::statusHandler(Input::Device& dev, BluetoothSocket&, BluetoothSocketState status)
{
	if(status == BluetoothSocketState::Opened)
	{
		logMsg("Zeemote opened successfully");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
		return 1;
	}
	else if(status == BluetoothSocketState::ConnectError)
	{
		logErr("Zeemote connection error");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
	}
	else if(status == BluetoothSocketState::ReadError)
	{
		logErr("Zeemote read error, disconnecting");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
	}
	return 0;
}

bool Zeemote::dataHandler(Input::Device &dev, const char *packet, size_t size)
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
			ctx.application().bluetoothInputDeviceStatus(ctx, dev, BluetoothSocketState::ReadError);
			return 0;
		}

		inputBufferPos += processBytes;
		assert(inputBufferPos <= sizeof(inputBuffer));

		// check if inputBuffer is complete
		if(inputBufferPos == packetSize)
		{
			auto time = SteadyClock::now();
			uint32_t rID = inputBuffer[2];
			logMsg("report id 0x%X, %s", rID, reportIDToStr(rID));
			switch(rID)
			{
				case RID_BTN_REPORT:
				{
					const uint8_t *key = &inputBuffer[3];
					//logMsg("got button report %X %X %X %X %X %X", key[0], key[1], key[2], key[3], key[4], key[5]);
					processBtnReport(dev, key, time);
					break;
				}
				case RID_8BA_2A_JS_REPORT:
					//logMsg("got analog report %d %d", (int8_t)inputBuffer[4], (int8_t)inputBuffer[5]);
				for(auto i : iotaCount(2))
					{
						if(axis[i].dispatchInputEvent((int8_t)inputBuffer[4+i], Input::Map::ZEEMOTE, time, dev, ctx.mainWindow()))
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

void Zeemote::processBtnReport(Input::Device &dev, const uint8_t *btnData, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	uint8_t btnPush[4] {0};
	for(auto i : iotaCount(4))
	{
		if(btnData[i] >= 4)
			break;
		btnPush[btnData[i]] = 1;
	}
	for(auto i : iotaCount(4))
	{
		if(prevBtnPush[i] != btnPush[i])
		{
			bool newState = btnPush[i];
			//logMsg("%s %s @ Zeemote", e->name, newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::ZEEMOTE, sysKeyMap[i], newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &dev};
			ctx.application().dispatchRepeatableKeyInputEvent( event);
		}
	}
	memcpy(prevBtnPush, btnPush, sizeof(prevBtnPush));
}

bool Zeemote::isSupportedClass(std::array<uint8_t, 3> devClass)
{
	return devClass == btClass;
}

}
