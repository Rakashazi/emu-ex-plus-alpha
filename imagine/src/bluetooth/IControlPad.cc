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

#define LOGTAG "ICP"
#include <imagine/bluetooth/IControlPad.hh>
#include <imagine/base/Application.hh>
#include <imagine/input/bluetoothInputDefs.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/bit.hh>
#include <imagine/util/ranges.hh>
#include "../input/PackedInputAccess.hh"
#include <algorithm>

namespace IG
{

using namespace IG::Input;

constexpr PackedInputAccess iCPDataAccess[]
{
	{ 0, bit(2), iControlPadKey::LEFT },
	{ 0, bit(1), iControlPadKey::RIGHT },
	{ 0, bit(3), iControlPadKey::DOWN },
	{ 0, bit(0), iControlPadKey::UP },
	{ 0, bit(4), iControlPadKey::L },

	{ 1, bit(3), iControlPadKey::A },
	{ 1, bit(4), iControlPadKey::X },
	{ 1, bit(5), iControlPadKey::B },
	{ 1, bit(6), iControlPadKey::R },
	{ 1, bit(0), iControlPadKey::SELECT },
	{ 1, bit(2), iControlPadKey::Y },
	{ 1, bit(1), iControlPadKey::START },
};

constexpr inline uint8_t CMD_SPP_GP_REPORTS = 0xAD;
constexpr inline uint8_t turnOnReports[2] = { CMD_SPP_GP_REPORTS, 1 };
constexpr inline uint8_t turnOffReports[2] = { CMD_SPP_GP_REPORTS, 0 };

constexpr inline uint8_t CMD_SET_LED = 0xFF;
constexpr inline uint8_t turnOnLED[2] = { CMD_SET_LED, 1 };

constexpr inline uint8_t CMD_FORCE_LED_CTRL = 0x6D;
constexpr inline uint8_t turnOnLEDControl[2] = { CMD_FORCE_LED_CTRL, 1 };

constexpr inline uint8_t CMD_SET_LED_MODE = 0xE4;
constexpr inline uint8_t LED_PULSE_DOUBLE = 0;
constexpr inline uint8_t setLEDPulseDouble[2] = { CMD_SET_LED_MODE, LED_PULSE_DOUBLE };
constexpr inline uint8_t LED_PULSE_INVERSE = 2;
constexpr inline uint8_t setLEDPulseInverse[2] = { CMD_SET_LED_MODE, LED_PULSE_INVERSE };
/*constexpr inline uint8_t LED_NO_PULSE = 3;
constexpr inline uint8_t setLEDNoPulse[2] = { CMD_SET_LED_MODE, LED_NO_PULSE };*/
constexpr inline uint8_t LED_PULSE_DQUICK = 5;
constexpr inline uint8_t setLEDPulseDQuick[2] = { CMD_SET_LED_MODE, LED_PULSE_DQUICK };


constexpr inline uint8_t CMD_POWER_OFF = 0x94;
constexpr inline uint8_t PWR_OFF_CHK_BYTE1 = 0x27;
constexpr inline uint8_t PWR_OFF_CHK_BYTE2 = 0x6A;
constexpr inline uint8_t PWR_OFF_CHK_BYTE3 = 0xFE;
//constexpr inline char shutdown[] = { CMD_POWER_OFF, PWR_OFF_CHK_BYTE1, PWR_OFF_CHK_BYTE2, PWR_OFF_CHK_BYTE3 };

constexpr inline uint8_t RESP_OKAY = 0x80;

constexpr const char* icpButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case iControlPadKey::A: return "A";
		case iControlPadKey::B: return "B";
		case iControlPadKey::X: return "X";
		case iControlPadKey::Y: return "Y";
		case iControlPadKey::L: return "L";
		case iControlPadKey::R: return "R";
		case iControlPadKey::START: return "Start";
		case iControlPadKey::SELECT: return "Select";
		case iControlPadKey::LNUB_LEFT: return "L:Left";
		case iControlPadKey::LNUB_RIGHT: return "L:Right";
		case iControlPadKey::LNUB_UP: return "L:Up";
		case iControlPadKey::LNUB_DOWN: return "L:Down";
		case iControlPadKey::RNUB_LEFT: return "R:Left";
		case iControlPadKey::RNUB_RIGHT: return "R:Right";
		case iControlPadKey::RNUB_UP: return "R:Up";
		case iControlPadKey::RNUB_DOWN: return "R:Down";
		case iControlPadKey::UP: return "Up";
		case iControlPadKey::RIGHT: return "Right";
		case iControlPadKey::DOWN: return "Down";
		case iControlPadKey::LEFT: return "Left";
	}
	return "";
}

IControlPad::IControlPad(ApplicationContext ctx, BluetoothAddr addr):
	BluetoothInputDevice{ctx, Input::Map::ICONTROLPAD, {.gamepad = true}, "iControlPad"},
	sock{ctx},
	addr{addr}
{}

const char *IControlPad::keyName(Key k) const
{
	return icpButtonName(k);
}

bool IControlPad::open(BluetoothAdapter &adapter, Input::Device &dev)
{
	logMsg("connecting to iCP");
	sock.onData =
		[&dev](const char *packet, size_t size)
		{
			return getAs<IControlPad>(dev).dataHandler(dev, packet, size);
		};
	sock.onStatus =
		[&dev](BluetoothSocket &sock, BluetoothSocketState status)
		{
			return getAs<IControlPad>(dev).statusHandler(dev, sock, status);
		};
	if(auto err = sock.openRfcomm(adapter, addr, 1);
		err.code())
	{
		logErr("error opening socket");
		return false;
	}
	return true;
}

void IControlPad::close()
{
	sock.close();
}

uint32_t IControlPad::statusHandler(Input::Device &dev, BluetoothSocket &sock, BluetoothSocketState status)
{
	if(status == BluetoothSocketState::Opened)
	{
		logMsg("iCP opened successfully");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
		sock.write(setLEDPulseInverse, sizeof setLEDPulseInverse);
		function = FUNC_SET_LED_MODE;
		return 1;
	}
	else if(status == BluetoothSocketState::ConnectError)
	{
		logErr("iCP connection error");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
	}
	else if(status == BluetoothSocketState::ReadError)
	{
		logErr("iCP read error, disconnecting");
		ctx.application().bluetoothInputDeviceStatus(ctx, dev, status);
	}
	return 0;
}

bool IControlPad::dataHandler(Input::Device &dev, const char *packetPtr, size_t size)
{
	auto packet = (const uint8_t*)packetPtr;
	uint32_t bytesLeft = size;
	//logMsg("%d bytes ready", bytesToRead);
	do
	{
		if(function != FUNC_NONE)
		{
			if(packet[size-bytesLeft] != RESP_OKAY)
			{
				logErr("error: iCP didn't respond with OK");
				ctx.application().bluetoothInputDeviceStatus(ctx, dev, BluetoothSocketState::ReadError);
				return 0;
			}
			logMsg("got OK reply");
			if(function == FUNC_SET_LED_MODE)
			{
				logMsg("turning on GP_REPORTS");
				sock.write(turnOnReports, sizeof turnOnReports);
				function = FUNC_GP_REPORTS;
				return 1;
			}
			function = FUNC_NONE;
			bytesLeft--;
		}
		else // handle GP_REPORT
		{
			uint32_t processBytes = std::min(bytesLeft, uint32_t(sizeof inputBuffer - inputBufferPos));
			memcpy(&inputBuffer[inputBufferPos], &packet[size-bytesLeft], processBytes);
			//logDMsg("read %d bytes from iCP", len);
			inputBufferPos += processBytes;
			assert(inputBufferPos <= 6);

			// check if inputBuffer is complete
			if(inputBufferPos == 6)
			{
				auto time = SteadyClock::now();
				for(auto i : iotaCount(4))
				{
					if(axis[i].dispatchInputEvent(inputBuffer[i], Input::Map::ICONTROLPAD, time, dev, ctx.mainWindow()))
						ctx.endIdleByUserActivity();
				}
				processBtnReport(dev, &inputBuffer[4], time);
				inputBufferPos = 0;
			}
			bytesLeft -= processBytes;
		}
	} while(bytesLeft);

	//logDMsg("done reading iCP");
	return 1;
}

void IControlPad::processBtnReport(Input::Device &dev, const char *btnData, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	for(auto e : iCPDataAccess)
	{
		bool oldState = prevBtnData[e.byteOffset] & e.mask,
			newState = btnData[e.byteOffset] & e.mask;
		if(oldState != newState)
		{
			//logMsg("%s %s @ iCP", e->name, newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::ICONTROLPAD, e.key, newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &dev};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

bool IControlPad::isSupportedClass(std::array<uint8_t, 3> devClass)
{
	return devClass == btClass;
}

}
