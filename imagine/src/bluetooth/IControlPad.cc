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

extern StaticArrayList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticArrayList<IControlPad*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> IControlPad::devList;

static const Input::PackedInputAccess iCPDataAccess[] =
{
	{ 0, bit(2), Input::iControlPad::LEFT },
	{ 0, bit(1), Input::iControlPad::RIGHT },
	{ 0, bit(3), Input::iControlPad::DOWN },
	{ 0, bit(0), Input::iControlPad::UP },
	{ 0, bit(4), Input::iControlPad::L },

	{ 1, bit(3), Input::iControlPad::A },
	{ 1, bit(4), Input::iControlPad::X },
	{ 1, bit(5), Input::iControlPad::B },
	{ 1, bit(6), Input::iControlPad::R },
	{ 1, bit(0), Input::iControlPad::SELECT },
	{ 1, bit(2), Input::iControlPad::Y },
	{ 1, bit(1), Input::iControlPad::START },
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
		if(Input::onDeviceChange)
			Input::onDeviceChange(*this, { Input::Device::Change::REMOVED });
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
		if(Input::onDeviceChange)
			Input::onDeviceChange(*this, { Input::Device::Change::ADDED });
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("iCP connection error");
		if(Input::onDeviceChange)
			Input::onDeviceChange(*this, { Input::Device::Change::CONNECT_ERROR });
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
			Event event{player, Event::MAP_ICONTROLPAD, (Key)e->keyEvent, newState ? PUSHED : RELEASED, 0, 0, this};
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
		axisKey[0].highKey = on ? Input::iControlPad::RIGHT : Input::iControlPad::LNUB_RIGHT;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_Y;
		axisKey[1].lowKey = on ? Input::iControlPad::UP : Input::iControlPad::LNUB_UP;
		axisKey[1].highKey = on ? Input::iControlPad::DOWN : Input::iControlPad::LNUB_DOWN;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_Z;
		axisKey[2].lowKey = on ? Input::iControlPad::LEFT : Input::iControlPad::RNUB_LEFT;
		axisKey[2].highKey = on ? Input::iControlPad::RIGHT : Input::iControlPad::RNUB_RIGHT;
	}
	{
		bool on = axisMask & Device::AXIS_BIT_RZ;
		axisKey[3].lowKey = on ? Input::iControlPad::UP : Input::iControlPad::RNUB_UP;
		axisKey[3].highKey = on ? Input::iControlPad::DOWN : Input::iControlPad::RNUB_DOWN;
	}
}

//void IControlPad::processNubDataForButtonEmulation(const schar *nubData, uint player)
//{
//	using namespace Input;
//	//logMsg("iCP nubs %d %d %d %d", (int)nubData[0], (int)nubData[1], (int)nubData[2], (int)nubData[3]);
//	forEachInArray(nubBtn, e)
//	{
//		bool newState;
//		if(e_i % 2)
//		{
//			newState = nubData[e_i/2] > nubDeadzone;
//		}
//		else
//		{
//			newState = nubData[e_i/2] < -nubDeadzone;
//		}
//		if(*e != newState)
//		{
//			static const uint nubBtnEvent[8] =
//			{
//				Input::iControlPad::LNUB_LEFT, Input::iControlPad::LNUB_RIGHT, Input::iControlPad::LNUB_UP, Input::iControlPad::LNUB_DOWN,
//				Input::iControlPad::RNUB_LEFT, Input::iControlPad::RNUB_RIGHT, Input::iControlPad::RNUB_UP, Input::iControlPad::RNUB_DOWN
//			};
//			Base::endIdleByUserActivity();
//			Event event{player, Event::MAP_ICONTROLPAD, (Key)nubBtnEvent[e_i], newState ? PUSHED : RELEASED, 0, 0, this};
//			startKeyRepeatTimer(event);
//			dispatchInputEvent(event);
//		}
//		*e = newState;
//	}
//}
