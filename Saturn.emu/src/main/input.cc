/*  This file is part of Saturn.emu.

	Saturn.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Saturn.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Saturn.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include <ss/smpc_iodevice.h>
#include <ss/input/gamepad.h>
#include <ss/input/3dpad.h>
#include <ss/input/mouse.h>
#include <ss/input/wheel.h>
#include <ss/input/mission.h>
#include <ss/input/gun.h>
#include <ss/input/keyboard.h>
#include <ss/input/jpkeyboard.h>
#include <mednafen-emuex/MDFNUtils.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

[[maybe_unused]] constexpr SystemLogger log{"Saturn.emu"};
const int EmuSystem::maxPlayers = 12;

enum class SaturnKey : KeyCode
{
	Up = 5,
	Right = 8,
	Down = 6,
	Left = 7,
	Start = 12,
	A = 11,
	B = 9,
	C = 10,
	X = 3,
	Y = 2,
	Z = 1,
	L = 16,
	R = 4,
};

struct GunData
{
	int16 x, y;
	uint8_t trigger:1, start:1, offscreen:1;
};

static GunData &asGunData(uint64_t &inputData) { return reinterpret_cast<GunData&>(inputData); }

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	SaturnKey::Up,
	SaturnKey::Right,
	SaturnKey::Down,
	SaturnKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	SaturnKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	SaturnKey::A,
	SaturnKey::B,
	SaturnKey::C,
	SaturnKey::X,
	SaturnKey::Y,
	SaturnKey::Z
);

constexpr auto faceLRKeyInfo = makeArray<KeyInfo>
(
	SaturnKey::A,
	SaturnKey::B,
	SaturnKey::C,
	SaturnKey::X,
	SaturnKey::Y,
	SaturnKey::Z,
	SaturnKey::L,
	SaturnKey::R
);

constexpr auto turboFaceKeyInfo = turbo(faceLRKeyInfo);

constexpr auto lKeyInfo = makeArray<KeyInfo>(SaturnKey::L);
constexpr auto rKeyInfo = makeArray<KeyInfo>(SaturnKey::R);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceLRKeyInfo, turboFaceKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);
constexpr auto gp3KeyInfo = transpose(gpKeyInfo, 2);
constexpr auto gp4KeyInfo = transpose(gpKeyInfo, 3);
constexpr auto gp5KeyInfo = transpose(gpKeyInfo, 4);
constexpr auto gp6KeyInfo = transpose(gpKeyInfo, 5);
constexpr auto gp7KeyInfo = transpose(gpKeyInfo, 6);
constexpr auto gp8KeyInfo = transpose(gpKeyInfo, 7);
constexpr auto gp9KeyInfo = transpose(gpKeyInfo, 8);
constexpr auto gp10KeyInfo = transpose(gpKeyInfo, 9);
constexpr auto gp11KeyInfo = transpose(gpKeyInfo, 10);
constexpr auto gp12KeyInfo = transpose(gpKeyInfo, 11);

std::span<const KeyCategory> SaturnApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad",    gpKeyInfo},
		KeyCategory{"Gamepad 2",  gp2KeyInfo, 1},
		KeyCategory{"Gamepad 3",  gp3KeyInfo, 2},
		KeyCategory{"Gamepad 4",  gp4KeyInfo, 3},
		KeyCategory{"Gamepad 5",  gp5KeyInfo, 4},
		KeyCategory{"Gamepad 6",  gp6KeyInfo, 5},
		KeyCategory{"Gamepad 7",  gp7KeyInfo, 6},
		KeyCategory{"Gamepad 8",  gp8KeyInfo, 7},
		KeyCategory{"Gamepad 9",  gp9KeyInfo, 8},
		KeyCategory{"Gamepad 10", gp10KeyInfo, 9},
		KeyCategory{"Gamepad 11", gp11KeyInfo, 10},
		KeyCategory{"Gamepad 12", gp12KeyInfo, 11},
	};
	return categories;
}

std::string_view SaturnApp::systemKeyCodeToString(KeyCode c)
{
	switch(SaturnKey(c))
	{
		case SaturnKey::Up: return "Up";
		case SaturnKey::Right: return "Right";
		case SaturnKey::Down: return "Down";
		case SaturnKey::Left: return "Left";
		case SaturnKey::Start: return "Start";
		case SaturnKey::A: return "A";
		case SaturnKey::B: return "B";
		case SaturnKey::C: return "C";
		case SaturnKey::X: return "X";
		case SaturnKey::Y: return "Y";
		case SaturnKey::Z: return "Z";
		case SaturnKey::L: return "L";
		case SaturnKey::R: return "R";
		default: return "";
	}
}

std::span<const KeyConfigDesc> SaturnApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{SaturnKey::Up, Keycode::UP},
		KeyMapping{SaturnKey::Right, Keycode::RIGHT},
		KeyMapping{SaturnKey::Down, Keycode::DOWN},
		KeyMapping{SaturnKey::Left, Keycode::LEFT},
		KeyMapping{SaturnKey::Start, Keycode::ENTER},
		KeyMapping{SaturnKey::A, Keycode::Z},
		KeyMapping{SaturnKey::B, Keycode::X},
		KeyMapping{SaturnKey::C, Keycode::C},
		KeyMapping{SaturnKey::X, Keycode::A},
		KeyMapping{SaturnKey::Y, Keycode::S},
		KeyMapping{SaturnKey::Z, Keycode::D},
		KeyMapping{SaturnKey::L, Keycode::Q},
		KeyMapping{SaturnKey::R, Keycode::E},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{SaturnKey::Up, Keycode::UP},
		KeyMapping{SaturnKey::Right, Keycode::RIGHT},
		KeyMapping{SaturnKey::Down, Keycode::DOWN},
		KeyMapping{SaturnKey::Left, Keycode::LEFT},
		KeyMapping{SaturnKey::Start, Keycode::GAME_START},
		KeyMapping{SaturnKey::A, Keycode::GAME_X},
		KeyMapping{SaturnKey::B, Keycode::GAME_A},
		KeyMapping{SaturnKey::C, Keycode::GAME_B},
		KeyMapping{SaturnKey::X, Keycode::GAME_L1},
		KeyMapping{SaturnKey::Y, Keycode::GAME_Y},
		KeyMapping{SaturnKey::Z, Keycode::GAME_R1},
		KeyMapping{SaturnKey::L, Keycode::GAME_L2},
		KeyMapping{SaturnKey::R, Keycode::GAME_R2},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{SaturnKey::Up, WiimoteKey::UP},
		KeyMapping{SaturnKey::Right, WiimoteKey::RIGHT},
		KeyMapping{SaturnKey::Down, WiimoteKey::DOWN},
		KeyMapping{SaturnKey::Left, WiimoteKey::LEFT},
		KeyMapping{SaturnKey::A, WiimoteKey::_1},
		KeyMapping{SaturnKey::B, WiimoteKey::_2},
		KeyMapping{SaturnKey::C, WiimoteKey::A},
		KeyMapping{SaturnKey::X, WiimoteKey::B},
		KeyMapping{SaturnKey::Y, WiimoteKey::MINUS},
		KeyMapping{SaturnKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap, genericGamepadModifierAppKeyCodeMap>();
}

bool SaturnApp::allowsTurboModifier(KeyCode c)
{
	switch(SaturnKey(c))
	{
		case SaturnKey::Z ... SaturnKey::R:
		case SaturnKey::B ... SaturnKey::A:
		case SaturnKey::L:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{256, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc SaturnApp::vControllerAssetDesc(KeyInfo key) const
{
	constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 0}, {2, 2}})},
		c{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 2}, {2, 2}})},
		x{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 2}, {2, 2}})},
		y{AssetFileID::gamepadOverlay,     gpImageCoords({{0, 4}, {2, 2}})},
		z{AssetFileID::gamepadOverlay,     gpImageCoords({{2, 4}, {2, 2}})},
		l{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 4}, {2, 2}})},
		r{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 4}, {2, 2}})},
		start{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
		blank{AssetFileID::gamepadOverlay, gpImageCoords({{2, 6}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(SaturnKey(key[0]))
	{
		case SaturnKey::A: return virtualControllerAssets.a;
		case SaturnKey::B: return virtualControllerAssets.b;
		case SaturnKey::C: return virtualControllerAssets.c;
		case SaturnKey::X: return virtualControllerAssets.x;
		case SaturnKey::Y: return virtualControllerAssets.y;
		case SaturnKey::Z: return virtualControllerAssets.z;
		case SaturnKey::L: return virtualControllerAssets.l;
		case SaturnKey::R: return virtualControllerAssets.r;
		case SaturnKey::Start: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

void SaturnSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	assumeExpr(player < maxPlayers);
	if(inputConfig.devs[player] == InputDeviceType::gun)
	{
		if(SaturnKey(a.code) == SaturnKey::Start)
		{
			asGunData(inputBuff[player]).start = a.state == Input::Action::PUSHED;
		}
	}
	else
	{
		auto oppositeKey = [](SaturnKey key)
		{
			switch(key)
			{
				case SaturnKey::Up:    return SaturnKey::Down;
				case SaturnKey::Right: return SaturnKey::Left;
				case SaturnKey::Down:  return SaturnKey::Up;
				case SaturnKey::Left:  return SaturnKey::Right;
				default: return SaturnKey{};
			}
		}(SaturnKey(a.code));
		if(a.state == Input::Action::PUSHED && int(oppositeKey)) // un-set opposite directions, otherwise many games report a gamepad disconnect
		{
			inputBuff[player] = clearBits(inputBuff[player], bit(to_underlying(oppositeKey) - 1));
		}
		inputBuff[player] = setOrClearBits(inputBuff[player], bit(a.code - 1), a.state == Input::Action::PUSHED);
	}
}

void SaturnSystem::clearInputBuffers(EmuInputView &)
{
	inputBuff = {};
}

SystemInputDeviceDesc SaturnSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Face Buttons + Inline L/R", faceLRKeyInfo, InputComponent::button, RB2DO, {.altConfig = true}},
		InputComponentDesc{"L", lKeyInfo, InputComponent::trigger, LB2DO},
		InputComponentDesc{"R", rKeyInfo, InputComponent::trigger, RB2DO},
		InputComponentDesc{"Start", centerKeyInfo, InputComponent::button, RB2DO},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}

static const char *toString(InputDeviceType dev)
{
	using enum InputDeviceType;
	switch(dev)
	{
		case none: return "none";
		case gamepad: return "gamepad";
		case multipad: return "3dpad";
		case mouse: return "mouse";
		case wheel: return "wheel";
		case mission: return "mission";
		case dmission: return "dmission";
		case gun: return "gun";
		case keyboard: return "keyboard";
		case jpkeyboard: return "jpkeyboard";
	}
	std::unreachable();
}

void SaturnSystem::applyInputConfig(InputConfig config, EmuApp &app)
{
	if(!CDInterfaces.size())
		return; // no content
	using namespace MDFN_IEN_SS;
	SMPC_SetMultitap(0, config.multitaps[0]);
	SMPC_SetMultitap(1, config.multitaps[1]);
	for(auto [idx, dev] : enumerate(config.devs))
	{
		mdfnGameInfo.SetInput(idx, toString(dev), reinterpret_cast<uint8*>(&inputBuff[idx]));
	}
	if(inputConfig.devs[0] == InputDeviceType::gun)
	{
		constexpr std::array gunDisabledKeys{
			KeyCode(SaturnKey::A), KeyCode(SaturnKey::B),	KeyCode(SaturnKey::C),
			KeyCode(SaturnKey::X), KeyCode(SaturnKey::Y), KeyCode(SaturnKey::Z),
			KeyCode(SaturnKey::L), KeyCode(SaturnKey::R)};
		app.setDisabledInputKeys(gunDisabledKeys);
		app.defaultVController().setGamepadDPadIsEnabled(false);
	}
	else
	{
		app.unsetDisabledInputKeys();
		app.defaultVController().setGamepadDPadIsEnabled(true);
	}
	currStateSize = stateSizeMDFN(); // changing input devices affects state size
}

void SaturnSystem::setInputConfig(InputConfig config, EmuApp &app)
{
	inputConfig = config;
	applyInputConfig(config, app);
}

bool SaturnSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, WRect gameRect)
{
	if(inputConfig.devs[0] != InputDeviceType::gun)
		return false;
	auto &gunData = asGunData(inputBuff[0]);
	if(gameRect.overlaps(e.pos()) && espec.DisplayRect.h)
	{
		auto xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
		auto xGun = remap(xRel, 0, gameRect.xSize(), 0.f, mdfnGameInfo.mouse_scale_x) + mdfnGameInfo.mouse_offs_x;
		auto yGun = remap(yRel, 0, gameRect.ySize(), 0.f, mdfnGameInfo.mouse_scale_y) + mdfnGameInfo.mouse_offs_y;
		//log.debug("gun pushed @ {},{}, mapped to {},{}", e.pos().x, e.pos().y, xGun, yGun);
		gunData.x = xGun;
		gunData.y = yGun;
		gunData.trigger = 1;
	}
	else
	{
		//log.debug("gun offscreen");
		gunData.offscreen = 1;
	}
	return true;
}

bool SaturnSystem::onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, WRect)
{
	if(inputConfig.devs[0] != InputDeviceType::gun)
		return false;
	auto &gunData = asGunData(inputBuff[0]);
	gunData.trigger = 0;
	gunData.offscreen = 0;
	return true;
}

}
