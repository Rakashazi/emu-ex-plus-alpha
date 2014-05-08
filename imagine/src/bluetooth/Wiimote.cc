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

#define LOGTAG "Wiimote"
#include <imagine/bluetooth/Wiimote.hh>
#include <imagine/base/Base.hh>
#include <imagine/util/bits.h>
#include <imagine/util/algorithm.h>
#include "../input/private.hh"

using namespace IG;

const uchar Wiimote::btClass[3] = { 0x04, 0x25, 0x00 };
const uchar Wiimote::btClassDevOnly[3] = { 0x04, 0x05, 0x00 };
const uchar Wiimote::btClassRemotePlus[3] = { 0x08, 0x05, 0x00 };
static const char ccDataBytes = 6;
static const char nunchuckDataBytes = 6;
static const char proDataBytes = 10;

extern StaticArrayList<BluetoothInputDevice*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE * 2> btInputDevList;
StaticArrayList<Wiimote*, Input::MAX_BLUETOOTH_DEVS_PER_TYPE> Wiimote::devList;

static const Input::PackedInputAccess wiimoteDataAccess[] =
{
	{ 0, bit(0), Input::Wiimote::DOWN }, // map to sideways control
	{ 0, bit(1), Input::Wiimote::UP },
	{ 0, bit(2), Input::Wiimote::RIGHT },
	{ 0, bit(3), Input::Wiimote::LEFT },
	{ 0, bit(4), Input::Wiimote::PLUS },

	{ 1, bit(0), Input::Wiimote::_2 },
	{ 1, bit(1), Input::Wiimote::_1 },
	{ 1, bit(2), Input::Wiimote::B },
	{ 1, bit(3), Input::Wiimote::A },
	{ 1, bit(4), Input::Wiimote::MINUS },
	{ 1, bit(7), Input::Wiimote::HOME },
};

static const Input::PackedInputAccess wiimoteCCDataAccess[] =
{
	{ 4, bit(7), Input::WiiCC::RIGHT },
	{ 4, bit(6), Input::WiiCC::DOWN },
	{ 4, bit(5), Input::WiiCC::L },
	{ 4, bit(4), Input::WiiCC::MINUS },
	{ 4, bit(3), Input::WiiCC::HOME },
	{ 4, bit(2), Input::WiiCC::PLUS },
	{ 4, bit(1), Input::WiiCC::R },

	{ 5, bit(7), Input::WiiCC::ZL },
	{ 5, bit(6), Input::WiiCC::B },
	{ 5, bit(5), Input::WiiCC::Y },
	{ 5, bit(4), Input::WiiCC::A },
	{ 5, bit(3), Input::WiiCC::X },
	{ 5, bit(2), Input::WiiCC::ZR },
	{ 5, bit(1), Input::WiiCC::LEFT },
	{ 5, bit(0), Input::WiiCC::UP },
};

static const Input::PackedInputAccess wiimoteProDataAccess[] =
{
	{ 8, bit(7), Input::WiiCC::RIGHT },
	{ 8, bit(6), Input::WiiCC::DOWN },
	{ 8, bit(5), Input::WiiCC::L },
	{ 8, bit(4), Input::WiiCC::MINUS },
	{ 8, bit(3), Input::WiiCC::HOME },
	{ 8, bit(2), Input::WiiCC::PLUS },
	{ 8, bit(1), Input::WiiCC::R },

	{ 9, bit(7), Input::WiiCC::ZL },
	{ 9, bit(6), Input::WiiCC::B },
	{ 9, bit(5), Input::WiiCC::Y },
	{ 9, bit(4), Input::WiiCC::A },
	{ 9, bit(3), Input::WiiCC::X },
	{ 9, bit(2), Input::WiiCC::ZR },
	{ 9, bit(1), Input::WiiCC::LEFT },
	{ 9, bit(0), Input::WiiCC::UP },

	{ 10, bit(1), Input::WiiCC::LH },
	{ 10, bit(0), Input::WiiCC::RH },
};

static const Input::PackedInputAccess wiimoteNunchukDataAccess[] =
{
	{ 5, bit(1), Input::Wiimote::NUN_C },
	{ 5, bit(0), Input::Wiimote::NUN_Z },
};

uint Wiimote::findFreeDevId()
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

CallResult Wiimote::open(BluetoothAdapter &adapter)
{
	logMsg("opening Wiimote");
	ctlSock.onData() = intSock.onData() =
		[this](const char *packet, size_t size)
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
		if(devList.isFull() || btInputDevList.isFull() || Input::devList.isFull())
		{
			logErr("No space left in BT input device list");
			removeFromSystem();
			delete this;
			return 1;
		}
		devList.push_back(this);
		btInputDevList.push_back(this);
		setLEDs(player);
		requestStatus();
		devId = player;
		Input::addDevice(*this);
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("Wiimote connection error");
		if(Input::onDeviceChange)
			Input::onDeviceChange(*this, { Input::Device::Change::CONNECT_ERROR });
		close();
		delete this;
	}
	else if(status == BluetoothSocket::STATUS_READ_ERROR)
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
		if(extDevice.map())
		{
			Input::removeDevice(extDevice);
			extDevice = {};
		}
		removeDevice(*this);
		if(Input::onDeviceChange)
			Input::onDeviceChange(*this, { Input::Device::Change::REMOVED });
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

bool Wiimote::dataHandler(const char *packetPtr, size_t size)
{
	auto packet = (const uchar*)packetPtr;
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
			//assert(device);
			processCoreButtons(packet, player);
		}

		bcase 0x32:
		{
			//logMsg("got core+extension report");
			//assert(device);
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
			//assert(device);
			processProButtons(packet, player);
		}

		bcase 0x20:
		{
			logMsg("got status report, bits 0x%X", packet[4]);
			if(extension && !(packet[4] & bit(1)))
			{
				logMsg("extension disconnected");
				extension = EXT_NONE;
				sendDataMode(0x30);
				if(extDevice.map())
				{
					Input::removeDevice(extDevice);
					if(Input::onDeviceChange)
						Input::onDeviceChange(extDevice, { Input::Device::Change::REMOVED });
					extDevice = {};
				}
			}
			else if(!extension && (packet[4] & bit(1)))
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
					if(Input::onDeviceChange)
						Input::onDeviceChange(*this, { Input::Device::Change::ADDED });
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
						axisKey[0] = {31-8, 31+8, Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT};
						axisKey[1] = {31-8, 31+8, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP};
						axisKey[2] = {16-4, 16+4, Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT};
						axisKey[3] = {16-4, 16+4, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP};
						setJoystickAxisAsDpadBits(joystickAxisAsDpadBitsDefault());
						assert(!extDevice.map());
						extDevice = {player, Input::Event::MAP_WII_CC, Input::Device::TYPE_BIT_GAMEPAD, "Wii Classic Controller"};
						Input::addDevice(extDevice);
						if(identifiedType && Input::onDeviceChange)
							Input::onDeviceChange(extDevice, { Input::Device::Change::ADDED });
					}
					else if(memcmp(&packet[7], nunchukType, 6) == 0)
					{
						logMsg("extension is Nunchuk");
						extension = EXT_NUNCHUK;
						sendDataModeByExtension();
						memset(prevExtData, 0xFF, sizeof(prevExtData));
						axisKey[0] = {127-64, 127+64, Input::Wiimote::NUN_STICK_LEFT, Input::Wiimote::NUN_STICK_RIGHT};
						axisKey[1] = {127-64, 127+64, Input::Wiimote::NUN_STICK_DOWN, Input::Wiimote::NUN_STICK_UP};
						setJoystickAxisAsDpadBits(joystickAxisAsDpadBitsDefault());
					}
					else if(memcmp(&packet[8], wiiUProType, sizeof(ccType)) == 0)
					{
						logMsg("extension is Wii U Pro");
						extension = EXT_WIIU_PRO;
						sendDataModeByExtension();
						memset(prevExtData, 0xFF, sizeof(prevExtData));
						axisKey[0] = {2048-256, 2048+256, Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT};
						axisKey[1] = {2048-256, 2048+256, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP};
						axisKey[2] = {2048-256, 2048+256, Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT};
						axisKey[3] = {2048-256, 2048+256, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP};
						setJoystickAxisAsDpadBits(joystickAxisAsDpadBitsDefault());
						map_ = Input::Event::MAP_WII_CC;
						name_ = "Wii U Pro Controller";
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
						if(Input::onDeviceChange)
							Input::onDeviceChange(*this, { Input::Device::Change::ADDED });
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
		case 0: return bit(4);
		case 1: return bit(5);
		case 2: return bit(6);
		case 3: return bit(7);
		case 4: return bit(4) | bit(7);
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

//void Wiimote::processStickDataForButtonEmulation(int player, const uchar *data)
//{
//	using namespace Input;
//	int pos[4];
//	decodeCCSticks(data, pos[0], pos[1], pos[2], pos[3]);
//	//logMsg("CC sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
//	forEachInArray(stickBtn, e)
//	{
//		bool newState;
//		switch(e_i)
//		{
//			case 0: newState = pos[0] < 31-8; break;
//			case 1: newState = pos[0] > 31+8; break;
//			case 2: newState = pos[1] < 31-8; break;
//			case 3: newState = pos[1] > 31+8; break;
//
//			case 4: newState = pos[2] < 16-4; break;
//			case 5: newState = pos[2] > 16+4; break;
//			case 6: newState = pos[3] < 16-4; break;
//			case 7: newState = pos[3] > 16+4; break;
//			default: bug_branch("%d", (int)e_i); return;
//		}
//		if(*e != newState)
//		{
//			static const uint btnEvent[8] =
//			{
//				Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP,
//				Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP
//			};
//			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
//			Base::endIdleByUserActivity();
//			Event event{(uint)player, Event::MAP_WII_CC, (Key)btnEvent[e_i], newState ? PUSHED : RELEASED, 0, 0, &extDevice};
//			startKeyRepeatTimer(event);
//			dispatchInputEvent(event);
//		}
//		*e = newState;
//	}
//}

void Wiimote::processCoreButtons(const uchar *packet, uint player)
{
	using namespace Input;
	auto btnData = &packet[2];
	forEachInArray(wiimoteDataAccess, e)
	{
		int newState = e->updateState(prevBtnData, btnData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, e->keyEvent), newState ? "pushed" : "released", player);
			Base::endIdleByUserActivity();
			Event event{(uint)player, Event::MAP_WIIMOTE, (Key)e->keyEvent, newState ? PUSHED : RELEASED, 0, 0, this};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

void Wiimote::processClassicButtons(const uchar *packet, uint player)
{
	using namespace Input;
	auto ccData = &packet[4];
	//processStickDataForButtonEmulation(player, ccData);
	int stickPos[4];
	decodeCCSticks(ccData, stickPos[0], stickPos[1], stickPos[2], stickPos[3]);
	iterateTimes(4, i)
	{
		if(axisKey[i].dispatch(stickPos[i], player, Input::Event::MAP_WIIMOTE, *this, Base::mainWindow()))
			Base::endIdleByUserActivity();
	}
	forEachInArray(wiimoteCCDataAccess, e)
	{
		int newState = e->updateState(prevExtData, ccData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wiimote cc", buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			Base::endIdleByUserActivity();
			Event event{player, Event::MAP_WII_CC, (Key)e->keyEvent, !newState ? PUSHED : RELEASED, 0, 0, &extDevice};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevExtData, ccData, ccDataBytes);
}

//void Wiimote::processProStickDataForButtonEmulation(int player, const uchar *data)
//{
//	using namespace Input;
//	int pos[4];
//	decodeProSticks(data, pos[0], pos[1], pos[2], pos[3]);
//	//logMsg("Pro sticks left %dx%d right %dx%d", pos[0], pos[1], pos[2], pos[3]);
//	forEachInArray(stickBtn, e)
//	{
//		bool newState;
//		switch(e_i)
//		{
//			case 0: newState = pos[0] < 2048-256; break;
//			case 1: newState = pos[0] > 2048+256; break;
//			case 2: newState = pos[1] < 2048-256; break;
//			case 3: newState = pos[1] > 2048+256; break;
//			case 4: newState = pos[2] < 2048-256; break;
//			case 5: newState = pos[2] > 2048+256; break;
//			case 6: newState = pos[3] < 2048-256; break;
//			case 7: newState = pos[3] > 2048+256; break;
//			default: bug_branch("%d", (int)e_i); return;
//		}
//		if(*e != newState)
//		{
//			static const uint btnEvent[8] =
//			{
//				Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT, Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP,
//				Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT, Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP
//			};
//			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
//			Base::endIdleByUserActivity();
//			Event event{(uint)player, Event::MAP_WII_CC, (Key)btnEvent[e_i], newState ? PUSHED : RELEASED, 0, 0, this};
//			startKeyRepeatTimer(event);
//			dispatchInputEvent(event);
//		}
//		*e = newState;
//	}
//}

void Wiimote::processProButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *proData = &packet[4];
	//processProStickDataForButtonEmulation(player, proData);
	int stickPos[4];
	decodeProSticks(proData, stickPos[0], stickPos[1], stickPos[2], stickPos[3]);
	iterateTimes(4, i)
	{
		if(axisKey[i].dispatch(stickPos[i], player, Input::Event::MAP_WIIMOTE, *this, Base::mainWindow()))
			Base::endIdleByUserActivity();
	}
	forEachInArray(wiimoteProDataAccess, e)
	{
		int newState = e->updateState(prevExtData, proData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wii u pro", buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			Base::endIdleByUserActivity();
			Event event{player, Event::MAP_WII_CC, (Key)e->keyEvent, !newState ? PUSHED : RELEASED, 0, 0, this};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevExtData, proData, proDataBytes);
}

//void Wiimote::processNunchukStickDataForButtonEmulation(int player, const uchar *data)
//{
//	using namespace Input;
//	int pos[2] = { data[0], data[1] };
//	//logMsg("Nunchuk stick %dx%d", pos[0], pos[1]);
//	forEachInArray(stickBtn, e)
//	{
//		bool newState;
//		switch(e_i)
//		{
//			case 0: newState = pos[0] < 127-64; break;
//			case 1: newState = pos[0] > 127+64; break;
//			case 2: newState = pos[1] < 127-64; break;
//			case 3: newState = pos[1] > 127+64; break;
//			default: bug_branch("%d", (int)e_i); return;
//		}
//		if(*e != newState)
//		{
//			static const uint btnEvent[8] =
//			{
//				Input::Wiimote::NUN_STICK_LEFT, Input::Wiimote::NUN_STICK_RIGHT, Input::Wiimote::NUN_STICK_DOWN, Input::Wiimote::NUN_STICK_UP
//			};
//			//logMsg("%s %s @ wiimote %d", buttonName(Event::MAP_WIIMOTE, btnEvent[e_i]), newState ? "pushed" : "released", player);
//			Base::endIdleByUserActivity();
//			Event event{(uint)player, Event::MAP_WIIMOTE, (Key)btnEvent[e_i], newState ? PUSHED : RELEASED, 0, 0, this};
//			startKeyRepeatTimer(event);
//			dispatchInputEvent(event);
//		}
//		*e = newState;
//
//		if(e_i == 3)
//			break; // only use half the array
//	}
//}

void Wiimote::processNunchukButtons(const uchar *packet, uint player)
{
	using namespace Input;
	const uchar *nunData = &packet[4];
	//processNunchukStickDataForButtonEmulation(player, nunData);
	iterateTimes(2, i)
	{
		if(axisKey[i].dispatch(nunData[i], player, Input::Event::MAP_WIIMOTE, *this, Base::mainWindow()))
			Base::endIdleByUserActivity();
	}
	forEachInArray(wiimoteNunchukDataAccess, e)
	{
		int newState = e->updateState(prevExtData, nunData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote nunchuk",buttonName(Event::MAP_WIIMOTE, e->keyEvent), !newState ? "pushed" : "released");
			Base::endIdleByUserActivity();
			Event event{player, Event::MAP_WIIMOTE, (Key)e->keyEvent, !newState ? PUSHED : RELEASED, 0, 0, this};
			startKeyRepeatTimer(event);
			dispatchInputEvent(event);
		}
	}
	memcpy(prevExtData, nunData, nunchuckDataBytes);
}

uint Wiimote::joystickAxisBits()
{
	switch(extension)
	{
		case EXT_CC:
		case EXT_WIIU_PRO:
			return Device::AXIS_BITS_STICK_1 | Device::AXIS_BITS_STICK_2;
		case EXT_NUNCHUK:
			return Device::AXIS_BITS_STICK_1;
	}
	return 0;
}

uint Wiimote::joystickAxisAsDpadBitsDefault()
{
	switch(extension)
	{
		case EXT_CC:
		case EXT_WIIU_PRO:
		case EXT_NUNCHUK:
			return Device::AXIS_BITS_STICK_1;
	}
	return 0;
}

void Wiimote::setJoystickAxisAsDpadBits(uint axisMask)
{
	using namespace Input;
	if(joystickAxisAsDpadBits_ == axisMask)
		return;

	joystickAxisAsDpadBits_ = axisMask;
	logMsg("mapping joystick axes for player: %d", player);
	if(extension == EXT_NUNCHUK)
	{
		{
			bool on = axisMask & Device::AXIS_BIT_X;
			axisKey[0].lowKey = on ? Input::Wiimote::LEFT : Input::Wiimote::NUN_STICK_LEFT;
			axisKey[0].highKey = on ? Input::Wiimote::RIGHT : Input::Wiimote::NUN_STICK_RIGHT;
		}
		{
			bool on = axisMask & Device::AXIS_BIT_Y;
			axisKey[1].lowKey = on ? Input::Wiimote::DOWN : Input::Wiimote::NUN_STICK_DOWN;
			axisKey[1].highKey = on ? Input::Wiimote::UP : Input::Wiimote::NUN_STICK_UP;
		}
	}
	else
	{
		{
			bool on = axisMask & Device::AXIS_BIT_X;
			axisKey[0].lowKey = on ? Input::WiiCC::LEFT : Input::WiiCC::LSTICK_LEFT;
			axisKey[0].highKey = on ? Input::WiiCC::RIGHT : Input::WiiCC::LSTICK_RIGHT;
		}
		{
			bool on = axisMask & Device::AXIS_BIT_Y;
			axisKey[1].lowKey = on ? Input::WiiCC::DOWN : Input::WiiCC::LSTICK_DOWN;
			axisKey[1].highKey = on ? Input::WiiCC::UP : Input::WiiCC::LSTICK_UP;
		}
		{
			bool on = axisMask & Device::AXIS_BIT_Z;
			axisKey[2].lowKey = on ? Input::WiiCC::LEFT : Input::WiiCC::RSTICK_LEFT;
			axisKey[2].highKey = on ? Input::WiiCC::RIGHT : Input::WiiCC::RSTICK_RIGHT;
		}
		{
			bool on = axisMask & Device::AXIS_BIT_RZ;
			axisKey[3].lowKey = on ? Input::WiiCC::DOWN : Input::WiiCC::RSTICK_DOWN;
			axisKey[3].highKey = on ? Input::WiiCC::UP : Input::WiiCC::RSTICK_UP;
		}
	}
}
