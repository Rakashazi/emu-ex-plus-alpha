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
#include <imagine/base/Base.hh>
#include <imagine/util/bits.h>
#include "../input/private.hh"
#include <algorithm>

using namespace IG;
using namespace Input;

extern StaticArrayList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticArrayList<IControlPad*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> IControlPad::devList;

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

static const uchar CMD_SPP_GP_REPORTS = 0xAD;
static const uchar turnOnReports[2] = { CMD_SPP_GP_REPORTS, 1 };
static const uchar turnOffReports[2] = { CMD_SPP_GP_REPORTS, 0 };

static const uchar CMD_SET_LED = 0xFF;
static const uchar turnOnLED[2] = { CMD_SET_LED, 1 };

static const uchar CMD_FORCE_LED_CTRL = 0x6D;
static const uchar turnOnLEDControl[2] = { CMD_FORCE_LED_CTRL, 1 };

static const uchar CMD_SET_LED_MODE = 0xE4;
static const uchar LED_PULSE_DOUBLE = 0;
static const uchar setLEDPulseDouble[2] = { CMD_SET_LED_MODE, LED_PULSE_DOUBLE };
static const uchar LED_PULSE_INVERSE = 2;
static const uchar setLEDPulseInverse[2] = { CMD_SET_LED_MODE, LED_PULSE_INVERSE };
/*const uchar LED_NO_PULSE = 3;
const uchar setLEDNoPulse[2] = { CMD_SET_LED_MODE, LED_NO_PULSE };*/
static const uchar LED_PULSE_DQUICK = 5;
static const uchar setLEDPulseDQuick[2] = { CMD_SET_LED_MODE, LED_PULSE_DQUICK };


static const uchar CMD_POWER_OFF = 0x94;
static const uchar PWR_OFF_CHK_BYTE1 = 0x27;
static const uchar PWR_OFF_CHK_BYTE2 = 0x6A;
static const uchar PWR_OFF_CHK_BYTE3 = 0xFE;
//static const char shutdown[] = { CMD_POWER_OFF, PWR_OFF_CHK_BYTE1, PWR_OFF_CHK_BYTE2, PWR_OFF_CHK_BYTE3 };

static const uchar RESP_OKAY = 0x80;

const uchar IControlPad::btClass[3] = { 0x00, 0x1F, 0x00 };

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
	return "Unknown";
}

const char *IControlPad::keyName(Key k) const
{
	return icpButtonName(k);
}

uint IControlPad::findFreeDevId()
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

CallResult IControlPad::open(BluetoothAdapter &adapter)
{
	logMsg("connecting to iCP");
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
	if(sock.openRfcomm(addr, 1) != OK)
	{
		logErr("error opening socket");
		return IO_ERROR;
	}

	return OK;
}

void IControlPad::close()
{
	sock.close();
}

void IControlPad::removeFromSystem()
{
	close();
	devList.remove(this);
	if(btInputDevList.remove(this))
	{
		removeDevice(*this);
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::REMOVED });
	}
}

uint IControlPad::statusHandler(BluetoothSocket &sock, uint status)
{
	if(status == BluetoothSocket::STATUS_OPENED)
	{
		logMsg("iCP opened successfully");
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
		sock.write(setLEDPulseInverse, sizeof setLEDPulseInverse);
		function = FUNC_SET_LED_MODE;
		devId = player;
		setJoystickAxisAsDpadBits(joystickAxisAsDpadBitsDefault());
		Input::addDevice(*this);
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::ADDED });
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("iCP connection error");
		Input::onDeviceChange.callCopySafe(*this, { Input::Device::Change::CONNECT_ERROR });
		close();
		delete this;
	}
	else if(status == BluetoothSocket::STATUS_READ_ERROR)
	{
		logErr("iCP read error, disconnecting");
		removeFromSystem();
		delete this;
	}
	return 0;
}

bool IControlPad::dataHandler(const char *packetPtr, size_t size)
{
	auto packet = (const uchar*)packetPtr;
	uint bytesLeft = size;
	//logMsg("%d bytes ready", bytesToRead);
	do
	{
		if(function != FUNC_NONE)
		{
			if(packet[size-bytesLeft] != RESP_OKAY)
			{
				logErr("error: iCP didn't respond with OK");
				removeFromSystem();
				delete this;
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
			uint processBytes = std::min(bytesLeft, uint(sizeof inputBuffer - inputBufferPos));
			memcpy(&inputBuffer[inputBufferPos], &packet[size-bytesLeft], processBytes);
			//logDMsg("read %d bytes from iCP", len);
			inputBufferPos += processBytes;
			assert(inputBufferPos <= 6);

			// check if inputBuffer is complete
			if(inputBufferPos == 6)
			{
				//processNubDataForButtonEmulation((schar*)inputBuffer, player);
				iterateTimes(4, i)
				{
					if(axisKey[i].dispatch(inputBuffer[i], player, Input::Event::MAP_ICONTROLPAD, *this, Base::mainWindow()))
						Base::endIdleByUserActivity();
				}
				processBtnReport(&inputBuffer[4], player);
				inputBufferPos = 0;
			}
			bytesLeft -= processBytes;
		}
	} while(bytesLeft);

	//logDMsg("done reading iCP");
	return 1;
}

void IControlPad::processBtnReport(const char *btnData, uint player)
{
	using namespace Input;
	forEachInArray(iCPDataAccess, e)
	{
		bool oldState = prevBtnData[e->byteOffset] & e->mask,
			newState = btnData[e->byteOffset] & e->mask;
		if(oldState != newState)
		{
			//logMsg("%s %s @ iCP", e->name, newState ? "pushed" : "released");
			Base::endIdleByUserActivity();
			Event event{player, Event::MAP_ICONTROLPAD, e->keyEvent, e->sysKey, newState ? PUSHED : RELEASED, 0, 0, this};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

uint IControlPad::joystickAxisBits()
{
	return Device::AXIS_BITS_STICK_1 | Device::AXIS_BITS_STICK_2;
}

uint IControlPad::joystickAxisAsDpadBitsDefault()
{
	return Device::AXIS_BITS_STICK_1;
}

void IControlPad::setJoystickAxisAsDpadBits(uint axisMask)
{
	using namespace Input;
	if(joystickAxisAsDpadBits_ == axisMask)
		return;

	joystickAxisAsDpadBits_ = axisMask;
	logMsg("mapping joystick axes for player: %d", player);
	{
		bool on = axisMask & Device::AXIS_BIT_X;
		axisKey[0].lowKey = on ? Input::iControlPad::LEFT : Input::iControlPad::LNUB_LEFT;
		axisKey[0].lowSysKey = on ? Keycode::LEFT : Keycode::JS1_XAXIS_NEG;
		axisKey[0].highKey = on ? Input::iControlPad::RIGHT : Input::iControlPad::LNUB_RIGHT;
		axisKey[0].highSysKey = on ? Keycode::RIGHT : Keycode::JS1_XAXIS_POS;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_Y;
		axisKey[1].lowKey = on ? Input::iControlPad::UP : Input::iControlPad::LNUB_UP;
		axisKey[1].lowSysKey = on ? Keycode::UP : Keycode::JS1_YAXIS_NEG;
		axisKey[1].highKey = on ? Input::iControlPad::DOWN : Input::iControlPad::LNUB_DOWN;
		axisKey[1].highSysKey = on ? Keycode::DOWN : Keycode::JS1_YAXIS_POS;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_Z;
		axisKey[2].lowKey = on ? Input::iControlPad::LEFT : Input::iControlPad::RNUB_LEFT;
		axisKey[2].lowSysKey = on ? Keycode::LEFT : Keycode::JS2_XAXIS_NEG;
		axisKey[2].highKey = on ? Input::iControlPad::RIGHT : Input::iControlPad::RNUB_RIGHT;
		axisKey[2].highSysKey = on ? Keycode::RIGHT : Keycode::JS2_XAXIS_POS;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_RZ;
		axisKey[3].lowKey = on ? Input::iControlPad::UP : Input::iControlPad::RNUB_UP;
		axisKey[3].lowSysKey = on ? Keycode::UP : Keycode::JS2_YAXIS_NEG;
		axisKey[3].highKey = on ? Input::iControlPad::DOWN : Input::iControlPad::RNUB_DOWN;
		axisKey[3].highSysKey = on ? Keycode::DOWN : Keycode::JS2_YAXIS_POS;
	}
}
