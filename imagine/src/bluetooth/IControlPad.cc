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
#include <imagine/base/Error.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/bit.hh>
#include <imagine/util/ranges.hh>
#include "../input/PackedInputAccess.hh"
#include <algorithm>

namespace IG
{

using namespace IG::Input;

static const PackedInputAccess iCPDataAccess[] =
{
	{ 0, bit(2), iControlPad::LEFT, Keycode::LEFT },
	{ 0, bit(1), iControlPad::RIGHT, Keycode::RIGHT },
	{ 0, bit(3), iControlPad::DOWN, Keycode::DOWN },
	{ 0, bit(0), iControlPad::UP, Keycode::UP },
	{ 0, bit(4), iControlPad::L, Keycode::GAME_L1 },

	{ 1, bit(3), iControlPad::A, Keycode::GAME_X },
	{ 1, bit(4), iControlPad::X, Keycode::GAME_A },
	{ 1, bit(5), iControlPad::B, Keycode::GAME_B },
	{ 1, bit(6), iControlPad::R, Keycode::GAME_R1 },
	{ 1, bit(0), iControlPad::SELECT, Keycode::GAME_SELECT },
	{ 1, bit(2), iControlPad::Y, Keycode::GAME_Y },
	{ 1, bit(1), iControlPad::START, Keycode::GAME_START },
};

static const uint8_t CMD_SPP_GP_REPORTS = 0xAD;
static const uint8_t turnOnReports[2] = { CMD_SPP_GP_REPORTS, 1 };
static const uint8_t turnOffReports[2] = { CMD_SPP_GP_REPORTS, 0 };

static const uint8_t CMD_SET_LED = 0xFF;
static const uint8_t turnOnLED[2] = { CMD_SET_LED, 1 };

static const uint8_t CMD_FORCE_LED_CTRL = 0x6D;
static const uint8_t turnOnLEDControl[2] = { CMD_FORCE_LED_CTRL, 1 };

static const uint8_t CMD_SET_LED_MODE = 0xE4;
static const uint8_t LED_PULSE_DOUBLE = 0;
static const uint8_t setLEDPulseDouble[2] = { CMD_SET_LED_MODE, LED_PULSE_DOUBLE };
static const uint8_t LED_PULSE_INVERSE = 2;
static const uint8_t setLEDPulseInverse[2] = { CMD_SET_LED_MODE, LED_PULSE_INVERSE };
/*const uint8_t LED_NO_PULSE = 3;
const uint8_t setLEDNoPulse[2] = { CMD_SET_LED_MODE, LED_NO_PULSE };*/
static const uint8_t LED_PULSE_DQUICK = 5;
static const uint8_t setLEDPulseDQuick[2] = { CMD_SET_LED_MODE, LED_PULSE_DQUICK };


static const uint8_t CMD_POWER_OFF = 0x94;
static const uint8_t PWR_OFF_CHK_BYTE1 = 0x27;
static const uint8_t PWR_OFF_CHK_BYTE2 = 0x6A;
static const uint8_t PWR_OFF_CHK_BYTE3 = 0xFE;
//static const char shutdown[] = { CMD_POWER_OFF, PWR_OFF_CHK_BYTE1, PWR_OFF_CHK_BYTE2, PWR_OFF_CHK_BYTE3 };

static const uint8_t RESP_OKAY = 0x80;

static const char *icpButtonName(Key b)
{
	switch(b)
	{
		case 0: return "None";
		case iControlPad::A: return "A";
		case iControlPad::B: return "B";
		case iControlPad::X: return "X";
		case iControlPad::Y: return "Y";
		case iControlPad::L: return "L";
		case iControlPad::R: return "R";
		case iControlPad::START: return "Start";
		case iControlPad::SELECT: return "Select";
		case iControlPad::LNUB_LEFT: return "L:Left";
		case iControlPad::LNUB_RIGHT: return "L:Right";
		case iControlPad::LNUB_UP: return "L:Up";
		case iControlPad::LNUB_DOWN: return "L:Down";
		case iControlPad::RNUB_LEFT: return "R:Left";
		case iControlPad::RNUB_RIGHT: return "R:Right";
		case iControlPad::RNUB_UP: return "R:Up";
		case iControlPad::RNUB_DOWN: return "R:Down";
		case iControlPad::UP: return "Up";
		case iControlPad::RIGHT: return "Right";
		case iControlPad::DOWN: return "Down";
		case iControlPad::LEFT: return "Left";
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

IG::ErrorCode IControlPad::open(BluetoothAdapter &adapter)
{
	logMsg("connecting to iCP");
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
	if(auto err = sock.openRfcomm(adapter, addr, 1);
		err)
	{
		logErr("error opening socket");
		return err;
	}
	return {};
}

void IControlPad::close()
{
	sock.close();
}

uint32_t IControlPad::statusHandler(BluetoothSocket &sock, uint32_t status)
{
	if(status == BluetoothSocket::STATUS_OPENED)
	{
		logMsg("iCP opened successfully");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
		sock.write(setLEDPulseInverse, sizeof setLEDPulseInverse);
		function = FUNC_SET_LED_MODE;
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("iCP connection error");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
	}
	else if(status == BluetoothSocket::STATUS_READ_ERROR)
	{
		logErr("iCP read error, disconnecting");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
	}
	return 0;
}

bool IControlPad::dataHandler(const char *packetPtr, size_t size)
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
				ctx.application().bluetoothInputDeviceStatus(ctx, *this, BluetoothSocket::STATUS_READ_ERROR);
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
					if(axis[i].update(inputBuffer[i], Input::Map::ICONTROLPAD, time, *this, ctx.mainWindow()))
						ctx.endIdleByUserActivity();
				}
				processBtnReport(&inputBuffer[4], time);
				inputBufferPos = 0;
			}
			bytesLeft -= processBytes;
		}
	} while(bytesLeft);

	//logDMsg("done reading iCP");
	return 1;
}

void IControlPad::processBtnReport(const char *btnData, SteadyClockTimePoint time)
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
			KeyEvent event{Map::ICONTROLPAD, e.keyEvent, e.sysKey, newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

bool IControlPad::isSupportedClass(std::array<uint8_t, 3> devClass)
{
	return devClass == btClass;
}

std::span<Input::Axis> IControlPad::motionAxes()
{
	return axis;
}

std::pair<Input::Key, Input::Key> IControlPad::joystickKeys(Input::AxisId axisId)
{
	switch(axisId)
	{
		case Input::AxisId::X: return {Input::iControlPad::LNUB_LEFT, Input::iControlPad::LNUB_RIGHT};
		case Input::AxisId::Y: return {Input::iControlPad::LNUB_DOWN, Input::iControlPad::LNUB_UP};
		case Input::AxisId::Z: return {Input::iControlPad::RNUB_LEFT, Input::iControlPad::RNUB_RIGHT};
		case Input::AxisId::RZ: return {Input::iControlPad::RNUB_DOWN, Input::iControlPad::RNUB_UP};
		default: return {};
	}
}

}
