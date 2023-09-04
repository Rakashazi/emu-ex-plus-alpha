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
#include <imagine/base/Application.hh>
#include <imagine/base/Error.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/bit.hh>
#include <imagine/util/ranges.hh>
#include "../input/PackedInputAccess.hh"

namespace IG
{

static constexpr char ccDataBytes = 6;
static constexpr char nunchuckDataBytes = 6;
static constexpr char proDataBytes = 10;

namespace Input
{

static const PackedInputAccess wiimoteDataAccess[] =
{
	{ 0, bit(0), Wiimote::DOWN, Keycode::DOWN }, // map to sideways control
	{ 0, bit(1), Wiimote::UP, Keycode::UP },
	{ 0, bit(2), Wiimote::RIGHT, Keycode::RIGHT },
	{ 0, bit(3), Wiimote::LEFT, Keycode::LEFT },
	{ 0, bit(4), Wiimote::PLUS, Keycode::GAME_START },

	{ 1, bit(0), Wiimote::_2, Keycode::GAME_B },
	{ 1, bit(1), Wiimote::_1, Keycode::GAME_A },
	{ 1, bit(2), Wiimote::B, Keycode::GAME_X },
	{ 1, bit(3), Wiimote::A, Keycode::GAME_C },
	{ 1, bit(4), Wiimote::MINUS, Keycode::GAME_SELECT },
	{ 1, bit(7), Wiimote::HOME, Keycode::MENU },
};

static const PackedInputAccess wiimoteCCDataAccess[] =
{
	{ 4, bit(7), WiiCC::RIGHT, Keycode::RIGHT },
	{ 4, bit(6), WiiCC::DOWN, Keycode::DOWN },
	{ 4, bit(5), WiiCC::L, Keycode::GAME_L1 },
	{ 4, bit(4), WiiCC::MINUS, Keycode::GAME_SELECT },
	{ 4, bit(3), WiiCC::HOME, Keycode::MENU },
	{ 4, bit(2), WiiCC::PLUS, Keycode::GAME_START },
	{ 4, bit(1), WiiCC::R, Keycode::GAME_R1 },

	{ 5, bit(7), WiiCC::ZL, Keycode::GAME_L2 },
	{ 5, bit(6), WiiCC::B, Keycode::GAME_A },
	{ 5, bit(5), WiiCC::Y, Keycode::GAME_X },
	{ 5, bit(4), WiiCC::A, Keycode::GAME_B },
	{ 5, bit(3), WiiCC::X, Keycode::GAME_Y },
	{ 5, bit(2), WiiCC::ZR, Keycode::GAME_R2 },
	{ 5, bit(1), WiiCC::LEFT, Keycode::LEFT },
	{ 5, bit(0), WiiCC::UP, Keycode::UP },
};

static const PackedInputAccess wiimoteProDataAccess[] =
{
	{ 8, bit(7), WiiCC::RIGHT, Keycode::RIGHT },
	{ 8, bit(6), WiiCC::DOWN, Keycode::DOWN },
	{ 8, bit(5), WiiCC::L, Keycode::GAME_L1 },
	{ 8, bit(4), WiiCC::MINUS, Keycode::GAME_SELECT },
	{ 8, bit(3), WiiCC::HOME, Keycode::MENU },
	{ 8, bit(2), WiiCC::PLUS, Keycode::GAME_START },
	{ 8, bit(1), WiiCC::R, Keycode::GAME_R1 },

	{ 9, bit(7), WiiCC::ZL, Keycode::GAME_L2 },
	{ 9, bit(6), WiiCC::B, Keycode::GAME_A },
	{ 9, bit(5), WiiCC::Y, Keycode::GAME_X },
	{ 9, bit(4), WiiCC::A, Keycode::GAME_B },
	{ 9, bit(3), WiiCC::X, Keycode::GAME_Y },
	{ 9, bit(2), WiiCC::ZR, Keycode::GAME_R2 },
	{ 9, bit(1), WiiCC::LEFT, Keycode::LEFT },
	{ 9, bit(0), WiiCC::UP, Keycode::UP },

	{ 10, bit(1), WiiCC::LH, Keycode::GAME_LEFT_THUMB },
	{ 10, bit(0), WiiCC::RH, Keycode::GAME_RIGHT_THUMB },
};

static const PackedInputAccess wiimoteNunchukDataAccess[] =
{
	{ 5, bit(1), Wiimote::NUN_C, Keycode::GAME_Y },
	{ 5, bit(0), Wiimote::NUN_Z, Keycode::GAME_Z },
};

}

static const char *wiimoteButtonName(Input::Key k)
{
	switch(k)
	{
		case 0: return "None";
		case Input::Wiimote::_1: return "1";
		case Input::Wiimote::_2: return "2";
		case Input::Wiimote::A: return "A";
		case Input::Wiimote::B: return "B";
		case Input::Wiimote::PLUS: return "+";
		case Input::Wiimote::MINUS: return "-";
		case Input::Wiimote::HOME: return "Home";
		case Input::Wiimote::UP: return "Up";
		case Input::Wiimote::RIGHT: return "Right";
		case Input::Wiimote::DOWN: return "Down";
		case Input::Wiimote::LEFT: return "Left";
		case Input::Wiimote::NUN_C: return "C";
		case Input::Wiimote::NUN_Z: return "Z";
		case Input::Wiimote::NUN_STICK_LEFT: return "N:Left";
		case Input::Wiimote::NUN_STICK_RIGHT: return "N:Right";
		case Input::Wiimote::NUN_STICK_UP: return "N:Up";
		case Input::Wiimote::NUN_STICK_DOWN: return "N:Down";
	}
	return "";
}

static const char *wiiCCButtonName(Input::Key k)
{
	using namespace IG::Input;
	switch(k)
	{
		case 0: return "None";
		case WiiCC::A: return "A";
		case WiiCC::B: return "B";
		case WiiCC::PLUS: return "+";
		case WiiCC::MINUS: return "-";
		case WiiCC::HOME: return "Home";
		case WiiCC::L: return "L";
		case WiiCC::R: return "R";
		case WiiCC::ZL: return "ZL";
		case WiiCC::ZR: return "ZR";
		case WiiCC::X: return "X";
		case WiiCC::Y: return "Y";
		case WiiCC::LH: return "LH";
		case WiiCC::RH: return "RH";
		case WiiCC::LSTICK_LEFT: return "L:Left";
		case WiiCC::LSTICK_RIGHT: return "L:Right";
		case WiiCC::LSTICK_UP: return "L:Up";
		case WiiCC::LSTICK_DOWN: return "L:Down";
		case WiiCC::RSTICK_LEFT: return "R:Left";
		case WiiCC::RSTICK_RIGHT: return "R:Right";
		case WiiCC::RSTICK_UP: return "R:Up";
		case WiiCC::RSTICK_DOWN: return "R:Down";
		case WiiCC::UP: return "Up";
		case WiiCC::RIGHT: return "Right";
		case WiiCC::DOWN: return "Down";
		case WiiCC::LEFT: return "Left";
	}
	return "";
}

static const char *wiiKeyName(Input::Key k, Input::Map map)
{
	switch(map)
	{
		case Input::Map::WII_CC: return wiiCCButtonName(k);
		default: return wiimoteButtonName(k);
	}
}

Wiimote::Wiimote(ApplicationContext ctx, BluetoothAddr addr):
	BluetoothInputDevice{ctx, Input::Map::WIIMOTE, {.gamepad = true}, "Wiimote"},
	ctlSock{ctx}, intSock{ctx},
	addr{addr}
{}

const char *Wiimote::keyName(Input::Key k) const
{
	return wiiKeyName(k, map_);
}

const char *Wiimote::ExtDevice::keyName(Input::Key k) const
{
	return wiiKeyName(k, map_);
}

IG::ErrorCode Wiimote::open(BluetoothAdapter &adapter)
{
	logMsg("opening Wiimote");
	ctlSock.onData() = intSock.onData() =
		[this](const char *packet, size_t size)
		{
			return dataHandler(packet, size);
		};
	ctlSock.onStatus() = intSock.onStatus() =
		[this](BluetoothSocket &sock, uint32_t status)
		{
			return statusHandler(sock, status);
		};
	if(ctlSock.openL2cap(adapter, addr, 17))
	{
		logErr("error opening control socket");
		return {EIO};
	}
	return {};
}

uint32_t Wiimote::statusHandler(BluetoothSocket &sock, uint32_t status)
{
	if(status == BluetoothSocket::STATUS_OPENED && &sock == (BluetoothSocket*)&ctlSock)
	{
		logMsg("opened Wiimote control socket, opening interrupt socket");
		intSock.openL2cap(*BluetoothAdapter::defaultAdapter(ctx), addr, 19);
		return 0; // don't add ctlSock to event loop
	}
	else if(status == BluetoothSocket::STATUS_OPENED && &sock == (BluetoothSocket*)&intSock)
	{
		logMsg("Wiimote opened successfully");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
		setLEDs(enumId());
		requestStatus();
		return BluetoothSocket::OPEN_USAGE_READ_EVENTS;
	}
	else if(status == BluetoothSocket::STATUS_CONNECT_ERROR)
	{
		logErr("Wiimote connection error");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
	}
	else if(status == BluetoothSocket::STATUS_READ_ERROR)
	{
		logErr("Wiimote read error");
		ctx.application().bluetoothInputDeviceStatus(ctx, *this, status);
	}
	return 1;
}

Wiimote::~Wiimote()
{
	removeExtendedDevice();
}

void Wiimote::writeReg(uint8_t offset, uint8_t val)
{
	uint8_t toWrite[23] = { 0xa2, 0x16, 0x04, 0xA4, 0x00, offset, 0x01, val }; // extra 15 bytes padding
	intSock.write(toWrite, sizeof(toWrite));
}

void Wiimote::readReg(uint32_t offset, uint8_t size)
{
	uint8_t toRead[8] = { 0xa2, 0x17, 0x04,
			uint8_t((offset & 0xFF0000) >> 16), uint8_t((offset & 0xFF00) >> 8), uint8_t(offset & 0xFF), 0x00, size };
	logMsg("read reg %X %X %X", toRead[3], toRead[4], toRead[5]);
	intSock.write(toRead, sizeof(toRead));
}

void Wiimote::setLEDs(uint8_t player)
{
	logMsg("setting LEDs for player %d", player);
	const uint8_t setLEDs[] = { 0xa2, 0x11, playerLEDs(player) };
	intSock.write(setLEDs, sizeof(setLEDs));
}

void Wiimote::requestStatus()
{
	logMsg("requesting status");
	const uint8_t reqStatus[] = { 0xa2, 0x15, 0x00 };
	intSock.write(reqStatus, sizeof(reqStatus));
}

void Wiimote::sendDataMode(uint8_t mode)
{
	logMsg("setting mode 0x%X", mode);
	uint8_t setMode[] = { 0xa2, 0x12, 0x00, mode };
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
		case EXT_CC:
		case EXT_NUNCHUK:
			return sendDataMode(0x32);
		case EXT_WIIU_PRO:
			return sendDataMode(0x34);
		default:
			return sendDataMode(0x30);
	}
}

bool Wiimote::dataHandler(const char *packetPtr, size_t size)
{
	using namespace IG::Input;
	auto packet = (const uint8_t*)packetPtr;
	if(packet[0] != 0xa1) [[unlikely]]
	{
		logWarn("Unknown report in Wiimote packet");
		return 1;
	}
	auto time = SteadyClock::now();
	switch(packet[1])
	{
		case 0x30:
		{
			//logMsg("got core report");
			//assert(device);
			processCoreButtons(packet, time);
			break;
		}

		case 0x32:
		{
			//logMsg("got core+extension report");
			//assert(device);
			processCoreButtons(packet, time);
			switch(extension)
			{
				case EXT_CC:
					processClassicButtons(packet, time); break;
				case EXT_NUNCHUK:
					processNunchukButtons(packet, time); break;
			}
			break;
		}

		case 0x34:
		{
			//logMsg("got core+extension19 report");
			//assert(device);
			processProButtons(packet, time);
			break;
		}

		case 0x20:
		{
			logMsg("got status report, bits 0x%X", packet[4]);
			if(extension && !(packet[4] & bit(1)))
			{
				logMsg("extension disconnected");
				extension = EXT_NONE;
				sendDataMode(0x30);
				removeExtendedDevice();
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
					ctx.application().dispatchInputDeviceChange(ctx, *this, Input::DeviceChange::added);
					identifiedType = 1;
				}
			}
			break;
		}

		case 0x21:
		{
			logMsg("got read report from addr %X %X", packet[5], packet[6]);
			switch(function)
			{
				case FUNC_GET_EXT_TYPE:
				{
					// CCs can have 0 or 1 in first byte, check only last 5 bytes
					const uint8_t ccType[5] {/*0x00,*/ 0x00, 0xA4, 0x20, 0x01, 0x01};
					const uint8_t wiiUProType[5] {/*0x00,*/ 0x00, 0xA4, 0x20, 0x01, 0x20};
					const uint8_t nunchukType[6] {0x00, 0x00, 0xA4, 0x20, 0x00, 0x00};
					logMsg("ext type: %X %X %X %X %X %X",
						packet[7], packet[8], packet[9], packet[10], packet[11], packet[12]);
					if(memcmp(&packet[8], ccType, sizeof(ccType)) == 0)
					{
						logMsg("extension is CC");
						extension = EXT_CC;
						sendDataModeByExtension();
						std::ranges::fill(prevExtData, 0xFF);
						static constexpr float axisClassicLScaler = 1./31.;
						static constexpr float axisClassicRScaler = 1./15.;
						axis[0] = {*this, Input::AxisId::X, axisClassicLScaler};
						axis[1] = {*this, Input::AxisId::Y, axisClassicLScaler};
						axis[2] = {*this, Input::AxisId::Z, axisClassicRScaler};
						axis[3] = {*this, Input::AxisId::RZ, axisClassicRScaler};
						assert(!extDevicePtr);
						extDevicePtr = &ctx.application().addInputDevice(ctx,
							std::make_unique<ExtDevice>(Input::Map::WII_CC, DeviceTypeFlags{.gamepad = true}, "Wii Classic Controller"), true);
					}
					else if(memcmp(&packet[7], nunchukType, 6) == 0)
					{
						logMsg("extension is Nunchuk");
						extension = EXT_NUNCHUK;
						sendDataModeByExtension();
						std::ranges::fill(prevExtData, 0xFF);
						static constexpr float axisNunchukScaler = 1./127.;
						axis[0] = {*this, Input::AxisId::X, axisNunchukScaler};
						axis[1] = {*this, Input::AxisId::Y, axisNunchukScaler};
					}
					else if(memcmp(&packet[8], wiiUProType, sizeof(ccType)) == 0)
					{
						logMsg("extension is Wii U Pro");
						extension = EXT_WIIU_PRO;
						sendDataModeByExtension();
						std::ranges::fill(prevExtData, 0xFF);
						static constexpr float axisClassicProScaler = 1./2047.;
						axis[0] = {*this, Input::AxisId::X, axisClassicProScaler};
						axis[1] = {*this, Input::AxisId::Y, axisClassicProScaler};
						axis[2] = {*this, Input::AxisId::Z, axisClassicProScaler};
						axis[3] = {*this, Input::AxisId::RZ, axisClassicProScaler};
						map_ = Input::Map::WII_CC;
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
						ctx.application().dispatchInputDeviceChange(ctx, *this, Input::DeviceChange::added);
						identifiedType = 1;
					}
				}
			}
			break;
		}

		case 0x22:
		{
			logMsg("ack output report, %X %X", packet[4], packet[5]);
			switch(function)
			{
				case FUNC_INIT_EXT:
					initExtensionPart2(); break;
				case FUNC_INIT_EXT_DONE:
					readReg(0xa400fa, 6);
					function = FUNC_GET_EXT_TYPE;
					logMsg("done extension init, getting type");
					break;
			}
			break;
		}

		default:
		{
			logMsg("unhandled packet type %d from wiimote", packet[1]);
		}
	}
	return 1;
}

uint8_t Wiimote::playerLEDs(int player)
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

void Wiimote::decodeCCSticks(const uint8_t *ccSticks, int &lX, int &lY, int &rX, int &rY)
{
	lX = (ccSticks[0] & 0x3F) - 31;
	lY = (ccSticks[1] & 0x3F) - 31;
	rX = ((ccSticks[0] & 0xC0) >> 3 | (ccSticks[1] & 0xC0) >> 5 | (ccSticks[2] & 0x80) >> 7) - 15;
	rY = (ccSticks[2] & 0x1F) - 15;
}

void Wiimote::decodeProSticks(const uint8_t *ccSticks, int &lX, int &lY, int &rX, int &rY)
{
	lX = (ccSticks[0] | (ccSticks[1] << 8)) - 2047;
	lY = (ccSticks[4] | (ccSticks[5] << 8)) - 2047;
	rX = (ccSticks[2] | (ccSticks[3] << 8)) - 2047;
	rY = (ccSticks[6] | (ccSticks[7] << 8)) - 2047;
}

void Wiimote::processCoreButtons(const uint8_t *packet, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	auto btnData = &packet[2];
	for(auto &e : wiimoteDataAccess)
	{
		int newState = e.updateState(prevBtnData, btnData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote %d", buttonName(Map::WIIMOTE, e.keyEvent), newState ? "pushed" : "released", player);
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::WIIMOTE, e.keyEvent, e.sysKey, newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevBtnData, btnData, sizeof(prevBtnData));
}

void Wiimote::processClassicButtons(const uint8_t *packet, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	auto ccData = &packet[4];
	int stickPos[4];
	decodeCCSticks(ccData, stickPos[0], stickPos[1], stickPos[2], stickPos[3]);
	for(auto i : iotaCount(4))
	{
		if(axis[i].update(stickPos[i], Map::WII_CC, time, *this, ctx.mainWindow()))
			ctx.endIdleByUserActivity();
	}
	for(auto &e : wiimoteCCDataAccess)
	{
		int newState = e.updateState(prevExtData, ccData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wiimote cc", buttonName(Map::WIIMOTE, e.keyEvent), !newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::WII_CC, e.keyEvent, e.sysKey, !newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, extDevicePtr};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevExtData, ccData, ccDataBytes);
}

void Wiimote::processProButtons(const uint8_t *packet, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	const uint8_t *proData = &packet[4];
	int stickPos[4];
	decodeProSticks(proData, stickPos[0], stickPos[1], stickPos[2], stickPos[3]);
	for(auto i : iotaCount(4))
	{
		if(axis[i].update(stickPos[i], Map::WII_CC, time, *this, ctx.mainWindow()))
			ctx.endIdleByUserActivity();
	}
	for(auto &e : wiimoteProDataAccess)
	{
		int newState = e.updateState(prevExtData, proData);
		if(newState != -1)
		{
			// note: buttons are 0 when pushed
			//logMsg("%s %s @ wii u pro", buttonName(Map::WIIMOTE, e.keyEvent), !newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::WII_CC, e.keyEvent, e.sysKey, !newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevExtData, proData, proDataBytes);
}

void Wiimote::processNunchukButtons(const uint8_t *packet, SteadyClockTimePoint time)
{
	using namespace IG::Input;
	const uint8_t *nunData = &packet[4];
	for(auto i : iotaCount(2))
	{
		if(axis[i].update(int(nunData[i]) - 127, Map::WIIMOTE, time, *this, ctx.mainWindow()))
			ctx.endIdleByUserActivity();
	}
	for(auto &e : wiimoteNunchukDataAccess)
	{
		int newState = e.updateState(prevExtData, nunData);
		if(newState != -1)
		{
			//logMsg("%s %s @ wiimote nunchuk",buttonName(Map::WIIMOTE, e.keyEvent), !newState ? "pushed" : "released");
			ctx.endIdleByUserActivity();
			KeyEvent event{Map::WIIMOTE, e.keyEvent, e.sysKey, !newState ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		}
	}
	memcpy(prevExtData, nunData, nunchuckDataBytes);
}

bool Wiimote::isSupportedClass(std::array<uint8_t, 3> devClass)
{
	return devClass == btClass
		|| devClass == btClassDevOnly
		|| devClass == btClassRemotePlus;
}

void Wiimote::removeExtendedDevice()
{
	if(extDevicePtr)
	{
		ctx.application().removeInputDevice(ctx, *extDevicePtr);
		extDevicePtr = {};
	}
}

std::span<Input::Axis> Wiimote::motionAxes()
{
	return axis;
}

std::pair<Input::Key, Input::Key> Wiimote::joystickKeys(Input::Map map, Input::AxisId axisId)
{
	if(map == Input::Map::WII_CC)
	{
		switch(axisId)
		{
			case Input::AxisId::X: return {Input::WiiCC::LSTICK_LEFT, Input::WiiCC::LSTICK_RIGHT};
			case Input::AxisId::Y: return {Input::WiiCC::LSTICK_DOWN, Input::WiiCC::LSTICK_UP};
			case Input::AxisId::Z: return {Input::WiiCC::RSTICK_LEFT, Input::WiiCC::RSTICK_RIGHT};
			case Input::AxisId::RZ: return {Input::WiiCC::RSTICK_DOWN, Input::WiiCC::RSTICK_UP};
			default: return {};
		}
	}
	else
	{
		switch(axisId)
		{
			case Input::AxisId::X: return {Input::Wiimote::NUN_STICK_LEFT, Input::Wiimote::NUN_STICK_RIGHT};
			case Input::AxisId::Y: return {Input::Wiimote::NUN_STICK_DOWN, Input::Wiimote::NUN_STICK_UP};
			default: return {};
		}
	}
}

}
