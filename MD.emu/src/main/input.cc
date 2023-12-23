/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/keyRemappingUtils.hh>
#include <imagine/util/math.hh>
#include "MainSystem.hh"
#include "MainApp.hh"
#include "input.h"
#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "io_ctrl.h"

namespace EmuEx
{

const int EmuSystem::maxPlayers = 4;

enum class MdKey : KeyCode
{
	Up = 1,
	Right = 4,
	Down = 2,
	Left = 3,
	Mode = 12,
	Start = 8,
	A = 7,
	B = 5,
	C = 6,
	X = 11,
	Y = 10,
	Z = 9,
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	MdKey::Up,
	MdKey::Right,
	MdKey::Down,
	MdKey::Left
);

constexpr auto centerKeyInfo = makeArray<KeyInfo>
(
	MdKey::Mode,
	MdKey::Start
);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	MdKey::A,
	MdKey::B,
	MdKey::C,
	MdKey::X,
	MdKey::Y,
	MdKey::Z
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, centerKeyInfo, faceKeyInfo, turboFaceKeyInfo>;
constexpr auto gp2KeyInfo = transpose(gpKeyInfo, 1);
constexpr auto gp3KeyInfo = transpose(gpKeyInfo, 2);
constexpr auto gp4KeyInfo = transpose(gpKeyInfo, 3);

std::span<const KeyCategory> MdApp::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
		KeyCategory{"Gamepad 2", gp2KeyInfo, 1},
		KeyCategory{"Gamepad 3", gp3KeyInfo, 2},
		KeyCategory{"Gamepad 4", gp4KeyInfo, 3},
	};
	return categories;
}

std::string_view MdApp::systemKeyCodeToString(KeyCode c)
{
	switch(MdKey(c))
	{
		case MdKey::Up: return "Up";
		case MdKey::Right: return "Right";
		case MdKey::Down: return "Down";
		case MdKey::Left: return "Left";
		case MdKey::Mode: return "Mode";
		case MdKey::Start: return "Start";
		case MdKey::A: return "A";
		case MdKey::B: return "B";
		case MdKey::C: return "C";
		case MdKey::X: return "X";
		case MdKey::Y: return "Y";
		case MdKey::Z: return "Z";
		default: return "";
	}
}

std::span<const KeyConfigDesc> MdApp::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{MdKey::Up, Keycode::UP},
		KeyMapping{MdKey::Right, Keycode::RIGHT},
		KeyMapping{MdKey::Down, Keycode::DOWN},
		KeyMapping{MdKey::Left, Keycode::LEFT},
		KeyMapping{MdKey::Mode, Keycode::SPACE},
		KeyMapping{MdKey::Start, Keycode::ENTER},
		KeyMapping{MdKey::A, Keycode::Z},
		KeyMapping{MdKey::B, Keycode::X},
		KeyMapping{MdKey::C, Keycode::C},
		KeyMapping{MdKey::X, Keycode::A},
		KeyMapping{MdKey::Y, Keycode::S},
		KeyMapping{MdKey::Z, Keycode::D},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{MdKey::Up, Keycode::UP},
		KeyMapping{MdKey::Right, Keycode::RIGHT},
		KeyMapping{MdKey::Down, Keycode::DOWN},
		KeyMapping{MdKey::Left, Keycode::LEFT},
		KeyMapping{MdKey::Mode, Keycode::GAME_SELECT},
		KeyMapping{MdKey::Start, Keycode::GAME_START},
		KeyMapping{MdKey::A, Keycode::GAME_X},
		KeyMapping{MdKey::B, Keycode::GAME_A},
		KeyMapping{MdKey::C, Keycode::GAME_B},
		KeyMapping{MdKey::X, Keycode::GAME_L1},
		KeyMapping{MdKey::Y, Keycode::GAME_Y},
		KeyMapping{MdKey::Z, Keycode::GAME_R1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{MdKey::Up, WiimoteKey::UP},
		KeyMapping{MdKey::Right, WiimoteKey::RIGHT},
		KeyMapping{MdKey::Down, WiimoteKey::DOWN},
		KeyMapping{MdKey::Left, WiimoteKey::LEFT},
		KeyMapping{MdKey::A, WiimoteKey::_1},
		KeyMapping{MdKey::B, WiimoteKey::_2},
		KeyMapping{MdKey::C, WiimoteKey::B},
		KeyMapping{MdKey::C, WiimoteKey::A},
		KeyMapping{MdKey::Mode, WiimoteKey::MINUS},
		KeyMapping{MdKey::Start, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool MdApp::allowsTurboModifier(KeyCode c)
{
	switch(MdKey(c))
	{
		case MdKey::Up ... MdKey::Mode:
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

constexpr struct VirtualControllerAssets
{
	AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

	a{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 0}, {2, 2}})},
	b{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 0}, {2, 2}})},
	c{AssetFileID::gamepadOverlay,     gpImageCoords({{4, 2}, {2, 2}})},
	x{AssetFileID::gamepadOverlay,     gpImageCoords({{6, 2}, {2, 2}})},
	y{AssetFileID::gamepadOverlay,     gpImageCoords({{0, 4}, {2, 2}})},
	z{AssetFileID::gamepadOverlay,     gpImageCoords({{2, 4}, {2, 2}})},
	mode{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
	start{AssetFileID::gamepadOverlay, gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},

	blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
} virtualControllerAssets;

AssetDesc MdApp::vControllerAssetDesc(KeyInfo key) const
{
	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(MdKey(key[0]))
	{
		case MdKey::A: return virtualControllerAssets.a;
		case MdKey::B: return virtualControllerAssets.b;
		case MdKey::C: return virtualControllerAssets.c;
		case MdKey::X: return virtualControllerAssets.x;
		case MdKey::Y: return virtualControllerAssets.y;
		case MdKey::Z: return virtualControllerAssets.z;
		case MdKey::Mode: return virtualControllerAssets.mode;
		case MdKey::Start: return virtualControllerAssets.start;
		default: return virtualControllerAssets.blank;
	}
}

constexpr std::array m3MissingCodes{KeyCode(MdKey::Mode), KeyCode(MdKey::A), KeyCode(MdKey::X), KeyCode(MdKey::Y), KeyCode(MdKey::Z)};
constexpr std::array md6BtnExtraCodes{KeyCode(MdKey::Mode), KeyCode(MdKey::X), KeyCode(MdKey::Y), KeyCode(MdKey::Z)};

void MdSystem::handleInputAction(EmuApp *, InputAction a)
{
	auto player = a.flags.deviceId;
	auto key = MdKey(a.code);
	if((input.system[1] == SYSTEM_MENACER && (key == MdKey::B || key == MdKey::C)) ||
		(input.system[1] == SYSTEM_JUSTIFIER && (key == MdKey::Start)))
	{
		player = 1;
	}
	uint16 &padData = input.pad[playerIdxMap[player]];
	padData = setOrClearBits(padData, bit(a.code - 1), a.isPushed());
}

static void updateGunPos(IG::WindowRect gameRect, const Input::MotionEvent &e, int idx)
{
	if(gameRect.overlaps(e.pos()))
	{
		int xRel = e.pos().x - gameRect.x, yRel = e.pos().y - gameRect.y;
		input.analog[idx][0] = IG::remap(xRel, 0, gameRect.xSize(), 0, bitmap.viewport.w);
		input.analog[idx][1] = IG::remap(yRel, 0, gameRect.ySize(), 0, bitmap.viewport.h);
	}
	else
	{
		// offscreen
		input.analog[idx][0] = input.analog[idx][1] = -1000;
	}
}

bool MdSystem::onPointerInputStart(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect gameRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	updateGunPos(gameRect, e, gunDevIdx);
	input.pad[gunDevIdx] |= INPUT_A;
	logMsg("gun pushed @ %d,%d, on MD %d,%d", e.pos().x, e.pos().y, input.analog[gunDevIdx][0], input.analog[gunDevIdx][1]);
	return true;
}

bool MdSystem::onPointerInputUpdate(const Input::MotionEvent &e, Input::DragTrackerState dragState,
	Input::DragTrackerState prevDragState, IG::WindowRect gameRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	updateGunPos(gameRect, e, gunDevIdx);
	return true;
}

bool MdSystem::onPointerInputEnd(const Input::MotionEvent &e, Input::DragTrackerState, IG::WindowRect)
{
	if(input.dev[gunDevIdx] != DEVICE_LIGHTGUN)
		return false;
	input.pad[gunDevIdx] = IG::clearBits(input.pad[gunDevIdx], (uint16)INPUT_A);
	return true;
}

void MdSystem::clearInputBuffers(EmuInputView &)
{
	IG::fill(input.pad);
	for(auto &analog : input.analog)
	{
		IG::fill(analog);
	}
}

const char *mdInputSystemToStr(uint8 system)
{
	switch(system)
	{
		case NO_SYSTEM: return "unconnected";
		case SYSTEM_MD_GAMEPAD: return "gamepad";
		case SYSTEM_MS_GAMEPAD: return "sms gamepad";
		case SYSTEM_MOUSE: return "mouse";
		case SYSTEM_MENACER: return "menacer";
		case SYSTEM_JUSTIFIER: return "justifier";
		case SYSTEM_TEAMPLAYER: return "team-player";
		case SYSTEM_LIGHTPHASER: return "light-phaser";
		default : return "unknown";
	}
}

static bool inputPortWasAutoSetByGame(unsigned port)
{
	return old_system[port] != -1;
}

void MdSystem::setupSmsInput(EmuApp &app)
{
	// first port may be set in rom loading code
	if(!input.system[0])
		input.system[0] = SYSTEM_MS_GAMEPAD;
	input.system[1] = SYSTEM_MS_GAMEPAD;
	io_init();
	app.setDisabledInputKeys(m3MissingCodes);
	for(auto i : iotaCount(2))
	{
		logMsg("attached %s to port %d", mdInputSystemToStr(input.system[i]), i);
	}
	gunDevIdx = 0;
	auto &vCtrl = app.defaultVController();
	if(input.dev[0] == DEVICE_LIGHTGUN && vCtrl.inputPlayer() != 1)
	{
		savedVControllerPlayer = vCtrl.inputPlayer();
		vCtrl.setInputPlayer(1);
	}
}

void MdSystem::setupMdInput(EmuApp &app)
{
	if(cart.special & HW_J_CART)
	{
		input.system[0] = input.system[1] = SYSTEM_MD_GAMEPAD;
		playerIdxMap[2] = 5;
		playerIdxMap[3] = 6;
	}
	else if(optionMultiTap)
	{
		input.system[0] = SYSTEM_TEAMPLAYER;
		input.system[1] = 0;

		playerIdxMap[1] = 1;
		playerIdxMap[2] = 2;
		playerIdxMap[3] = 3;
	}
	else
	{
		for(auto i : iotaCount(2))
		{
			if(mdInputPortDev[i] == -1) // user didn't specify device, go with auto settings
			{
				if(!inputPortWasAutoSetByGame(i))
					input.system[i] = SYSTEM_MD_GAMEPAD;
				else
				{
					logMsg("input port %d set by game detection", i);
					input.system[i] = old_system[i];
				}
			}
			else
				input.system[i] = mdInputPortDev[i];
			logMsg("attached %s to port %d%s", mdInputSystemToStr(input.system[i]), i, mdInputPortDev[i] == -1 ? " (auto)" : "");
		}
	}
	io_init();
	gunDevIdx = 4;
	if(option6BtnPad)
		app.unsetDisabledInputKeys();
	else
		app.setDisabledInputKeys(md6BtnExtraCodes);
}

void MdSystem::setupInput(EmuApp &app)
{
	if(!hasContent())
	{
		if(option6BtnPad)
			app.unsetDisabledInputKeys();
		else
			app.setDisabledInputKeys(md6BtnExtraCodes);
		return;
	}
	if(savedVControllerPlayer != -1)
	{
		app.defaultVController().setInputPlayer(std::exchange(savedVControllerPlayer, -1));
	}
	IG::fill(playerIdxMap);
	playerIdxMap[0] = 0;
	playerIdxMap[1] = 4;

	unsigned mdPad = option6BtnPad ? DEVICE_PAD6B : DEVICE_PAD3B;
	for(auto i : iotaCount(4))
		config.input[i].padtype = mdPad;

	if(system_hw == SYSTEM_PBC)
	{
		setupSmsInput(app);
	}
	else
	{
		setupMdInput(app);
	}
}

SystemInputDeviceDesc MdSystem::inputDeviceDesc(int idx) const
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Mode", {&centerKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Start", {&centerKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Mode/Start", centerKeyInfo, InputComponent::button, CB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};

	return gamepadDesc;
}


}
