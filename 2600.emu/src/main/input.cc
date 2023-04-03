/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <EventHandler.hxx>
#include <OSystem.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#undef Debugger
#include "MainSystem.hh"
#include "MainApp.hh"
#include <imagine/util/math/space.hh>
#include <imagine/util/container/array.hh>

namespace EmuEx
{

enum
{
	vcsKeyIdxUp = Controls::systemKeyMapStart,
	vcsKeyIdxRight,
	vcsKeyIdxDown,
	vcsKeyIdxLeft,
	vcsKeyIdxLeftUp,
	vcsKeyIdxRightUp,
	vcsKeyIdxRightDown,
	vcsKeyIdxLeftDown,
	vcsKeyIdxJSBtn1,
	vcsKeyIdxJSBtn1Turbo,
	vcsKeyIdxJSBtn2,
	vcsKeyIdxJSBtn2Turbo,
	vcsKeyIdxJSBtn3,
	vcsKeyIdxJSBtn3Turbo,

	vcsKeyIdxUp2,
	vcsKeyIdxRight2,
	vcsKeyIdxDown2,
	vcsKeyIdxLeft2,
	vcsKeyIdxLeftUp2,
	vcsKeyIdxRightUp2,
	vcsKeyIdxRightDown2,
	vcsKeyIdxLeftDown2,
	vcsKeyIdxJSBtn1P2,
	vcsKeyIdxJSBtn1P2Turbo,
	vcsKeyIdxJSBtn2P2,
	vcsKeyIdxJSBtn2P2Turbo,
	vcsKeyIdxJSBtn3P2,
	vcsKeyIdxJSBtn3P2Turbo,

	vcsKeyIdxSelect,
	vcsKeyIdxReset,
	vcsKeyIdxP1Diff,
	vcsKeyIdxP2Diff,
	vcsKeyIdxColorBW,
	vcsKeyIdxKeyboard1Base,
	vcsKeyIdxKeyboard2Base = vcsKeyIdxKeyboard1Base + 12,
};

constexpr std::array<unsigned, 4> dpadButtonCodes
{
	vcsKeyIdxUp,
	vcsKeyIdxRight,
	vcsKeyIdxDown,
	vcsKeyIdxLeft,
};

constexpr std::array<unsigned, 2> consoleButtonCodes
{
	vcsKeyIdxSelect,
	vcsKeyIdxReset,
};

constexpr std::array<unsigned, 3> jsButtonCodes
{
	vcsKeyIdxJSBtn1,
	vcsKeyIdxJSBtn2,
	vcsKeyIdxJSBtn3,
};

constexpr std::array<unsigned, 12> kbButtonCodes
{
	vcsKeyIdxKeyboard1Base,
	vcsKeyIdxKeyboard1Base + 1,
	vcsKeyIdxKeyboard1Base + 2,
	vcsKeyIdxKeyboard1Base + 3,
	vcsKeyIdxKeyboard1Base + 4,
	vcsKeyIdxKeyboard1Base + 5,
	vcsKeyIdxKeyboard1Base + 6,
	vcsKeyIdxKeyboard1Base + 7,
	vcsKeyIdxKeyboard1Base + 8,
	vcsKeyIdxKeyboard1Base + 9,
	vcsKeyIdxKeyboard1Base + 10,
	vcsKeyIdxKeyboard1Base + 11,
};

constexpr std::array jsComponents
{
	InputComponentDesc{"D-Pad", dpadButtonCodes, InputComponent::dPad, LB2DO},
	InputComponentDesc{"Joystick Buttons", jsButtonCodes, InputComponent::button, RB2DO},
	InputComponentDesc{"Keyboard Buttons", kbButtonCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::altConfig | InputComponentFlagsMask::rowSize3},
	InputComponentDesc{"Select", {&consoleButtonCodes[0], 1}, InputComponent::button, LB2DO},
	InputComponentDesc{"Reset", {&consoleButtonCodes[1], 1}, InputComponent::button, RB2DO},
	InputComponentDesc{"Console Buttons", consoleButtonCodes, InputComponent::button, RB2DO, InputComponentFlagsMask::altConfig},
};

constexpr SystemInputDeviceDesc jsDesc{"Joystick", jsComponents};

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr FP imageSize{512, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

constexpr struct VirtualControllerAssets
{
	AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

	jsBtn1{AssetFileID::gamepadOverlay, gpImageCoords({{4, 0}, {2, 2}})},
	jsBtn2{AssetFileID::gamepadOverlay, gpImageCoords({{6, 0}, {2, 2}})},
	jsBtn3{AssetFileID::gamepadOverlay, gpImageCoords({{6, 4}, {2, 2}})},

	one{AssetFileID::gamepadOverlay,   gpImageCoords({{10, 0}, {2, 2}})},
	two{AssetFileID::gamepadOverlay,   gpImageCoords({{12, 0}, {2, 2}})},
	three{AssetFileID::gamepadOverlay, gpImageCoords({{14, 0}, {2, 2}})},
	four{AssetFileID::gamepadOverlay,  gpImageCoords({{4,  2}, {2, 2}})},
	five{AssetFileID::gamepadOverlay,  gpImageCoords({{6,  2}, {2, 2}})},
	six{AssetFileID::gamepadOverlay,   gpImageCoords({{8,  2}, {2, 2}})},
	seven{AssetFileID::gamepadOverlay, gpImageCoords({{10, 2}, {2, 2}})},
	eight{AssetFileID::gamepadOverlay, gpImageCoords({{12, 2}, {2, 2}})},
	nine{AssetFileID::gamepadOverlay,  gpImageCoords({{14, 2}, {2, 2}})},
	star{AssetFileID::gamepadOverlay,  gpImageCoords({{0,  4}, {2, 2}})},
	zero{AssetFileID::gamepadOverlay,  gpImageCoords({{8,  0}, {2, 2}})},
	pound{AssetFileID::gamepadOverlay, gpImageCoords({{2,  4}, {2, 2}})},

	select{AssetFileID::gamepadOverlay,  gpImageCoords({{0, 6}, {2, 1}}), {1, 2}},
	p1Diff{AssetFileID::gamepadOverlay,  gpImageCoords({{2, 6}, {2, 1}}), {1, 2}},
	p2Diff{AssetFileID::gamepadOverlay,  gpImageCoords({{4, 6}, {2, 1}}), {1, 2}},
	reset{AssetFileID::gamepadOverlay,   gpImageCoords({{0, 7}, {2, 1}}), {1, 2}},
	colorBW{AssetFileID::gamepadOverlay, gpImageCoords({{2, 7}, {2, 1}}), {1, 2}},

	blank{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})};
} virtualControllerAssets;

static_assert(offsetof(VirtualControllerAssets, one) + 11 * sizeof(AssetDesc) == offsetof(VirtualControllerAssets, pound),
	"keyboard assets must be in sequence");

AssetDesc A2600App::vControllerAssetDesc(unsigned key) const
{
	switch(key)
	{
		case 0: return virtualControllerAssets.dpad;
		case vcsKeyIdxJSBtn1:
		case vcsKeyIdxJSBtn1Turbo: return virtualControllerAssets.jsBtn1;
		case vcsKeyIdxJSBtn2:
		case vcsKeyIdxJSBtn2Turbo: return virtualControllerAssets.jsBtn2;
		case vcsKeyIdxJSBtn3:
		case vcsKeyIdxJSBtn3Turbo: return virtualControllerAssets.jsBtn3;
		case vcsKeyIdxKeyboard1Base ... vcsKeyIdxKeyboard1Base + 11:
			return (&virtualControllerAssets.one)[key - vcsKeyIdxKeyboard1Base];
		case vcsKeyIdxSelect: return virtualControllerAssets.select;
		case vcsKeyIdxP1Diff: return virtualControllerAssets.p1Diff;
		case vcsKeyIdxP2Diff: return virtualControllerAssets.p2Diff;
		case vcsKeyIdxReset: return virtualControllerAssets.reset;
		case vcsKeyIdxColorBW: return virtualControllerAssets.colorBW;
		default: return virtualControllerAssets.blank;
	}
}

const int EmuSystem::maxPlayers = 2;

void A2600System::clearInputBuffers(EmuInputView &)
{
	Event &ev = osystem.eventHandler().event();
	ev.clear();

	ev.set(Event::ConsoleLeftDiffB, p1DiffB);
	ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
	ev.set(Event::ConsoleRightDiffB, p2DiffB);
	ev.set(Event::ConsoleRightDiffA, !p2DiffB);
	ev.set(Event::ConsoleColor, vcsColor);
	ev.set(Event::ConsoleBlackWhite, !vcsColor);
}

void A2600System::updateJoytickMapping(EmuApp &app, Controller::Type type)
{
	if(type == Controller::Type::Paddles)
	{
		jsFireMap = {Event::LeftPaddleAFire, Event::LeftPaddleBFire};
		jsLeftMap = {Event::LeftPaddleAIncrease, Event::LeftPaddleBIncrease};
		jsRightMap = {Event::LeftPaddleADecrease, Event::LeftPaddleBDecrease};
	}
	else
	{
		jsFireMap = {Event::LeftJoystickFire, Event::RightJoystickFire};
		jsLeftMap = {Event::LeftJoystickLeft, Event::RightJoystickLeft};
		jsRightMap = {Event::LeftJoystickRight, Event::RightJoystickRight};
	}
}

static bool isJoystickButton(unsigned input)
{
	switch(input)
	{
		case vcsKeyIdxJSBtn1 ... vcsKeyIdxJSBtn3Turbo:
		case vcsKeyIdxJSBtn1P2 ... vcsKeyIdxJSBtn3P2Turbo:
			return true;
		default: return false;
	}
}

InputAction A2600System::translateInputAction(InputAction action)
{
	if(!isJoystickButton(action.key))
		action.setTurboFlag(false);
	action.key = [&] -> unsigned
	{
		switch(action.key)
		{
			case vcsKeyIdxUp: return Event::LeftJoystickUp;
			case vcsKeyIdxRight: return jsRightMap[0];
			case vcsKeyIdxDown: return Event::LeftJoystickDown;
			case vcsKeyIdxLeft: return jsLeftMap[0];
			case vcsKeyIdxLeftUp: return Event::LeftJoystickLeft | (Event::LeftJoystickUp << 8);
			case vcsKeyIdxRightUp: return Event::LeftJoystickRight | (Event::LeftJoystickUp << 8);
			case vcsKeyIdxRightDown: return Event::LeftJoystickRight | (Event::LeftJoystickDown << 8);
			case vcsKeyIdxLeftDown: return Event::LeftJoystickLeft | (Event::LeftJoystickDown << 8);
			case vcsKeyIdxJSBtn1Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn1: return jsFireMap[0];
			case vcsKeyIdxJSBtn2Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn2: return Event::LeftJoystickFire5;
			case vcsKeyIdxJSBtn3Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn3: return Event::LeftJoystickFire9;

			case vcsKeyIdxUp2: return Event::RightJoystickUp;
			case vcsKeyIdxRight2: return jsRightMap[1];
			case vcsKeyIdxDown2: return Event::RightJoystickDown;
			case vcsKeyIdxLeft2: return jsLeftMap[1];
			case vcsKeyIdxLeftUp2: return Event::RightJoystickLeft | (Event::RightJoystickUp << 8);
			case vcsKeyIdxRightUp2: return Event::RightJoystickRight | (Event::RightJoystickUp << 8);
			case vcsKeyIdxRightDown2: return Event::RightJoystickRight | (Event::RightJoystickDown << 8);
			case vcsKeyIdxLeftDown2: return Event::RightJoystickLeft | (Event::RightJoystickDown << 8);
			case vcsKeyIdxJSBtn1P2Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn1P2: return jsFireMap[1];
			case vcsKeyIdxJSBtn2P2Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn2P2: return Event::RightJoystickFire5;
			case vcsKeyIdxJSBtn3P2Turbo: action.setTurboFlag(true); [[fallthrough]];
			case vcsKeyIdxJSBtn3P2: return Event::RightJoystickFire9;

			case vcsKeyIdxSelect: return Event::ConsoleSelect;
			case vcsKeyIdxP1Diff: return Event::Combo1; // toggle P1 diff
			case vcsKeyIdxP2Diff: return Event::Combo2; // toggle P2 diff
			case vcsKeyIdxColorBW: return Event::Combo3; // toggle Color/BW
			case vcsKeyIdxReset: return Event::ConsoleReset;
			case vcsKeyIdxKeyboard1Base ... vcsKeyIdxKeyboard1Base + 11:
				return Event::LeftKeyboard1 + (action.key - vcsKeyIdxKeyboard1Base);
			case vcsKeyIdxKeyboard2Base ... vcsKeyIdxKeyboard2Base + 11:
				return Event::RightKeyboard1 + (action.key - vcsKeyIdxKeyboard2Base);
		}
		bug_unreachable("invalid key");
	}();
	return action;
}

void A2600System::handleInputAction(EmuApp *app, InputAction a)
{
	auto &ev = osystem.eventHandler().event();
	auto event1 = a.key & 0xFF;
	bool isPushed = a.state == Input::Action::PUSHED;

	//logMsg("got key %d", emuKey);

	switch(event1)
	{
		case Event::Combo1:
			if(!isPushed)
				break;
			p1DiffB ^= true;
			if(app)
			{
				app->postMessage(1, false, p1DiffB ? "P1 Difficulty -> B" : "P1 Difficulty -> A");
			}
			ev.set(Event::ConsoleLeftDiffB, p1DiffB);
			ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
			break;
		case Event::Combo2:
			if(!isPushed)
				break;
			p2DiffB ^= true;
			if(app)
			{
				app->postMessage(1, false, p2DiffB ? "P2 Difficulty -> B" : "P2 Difficulty -> A");
			}
			ev.set(Event::ConsoleRightDiffB, p2DiffB);
			ev.set(Event::ConsoleRightDiffA, !p2DiffB);
			break;
		case Event::Combo3:
			if(!isPushed)
				break;
			vcsColor ^= true;
			if(app)
			{
				app->postMessage(1, false, vcsColor ? "Color Switch -> Color" : "Color Switch -> B&W");
			}
			ev.set(Event::ConsoleColor, vcsColor);
			ev.set(Event::ConsoleBlackWhite, !vcsColor);
			break;
		case Event::LeftKeyboard1 ... Event::RightKeyboardPound:
			ev.set(Event::Type(event1), isPushed);
			break;
		default:
			ev.set(Event::Type(event1), isPushed);
			auto event2 = a.key >> 8;
			if(event2) // extra event for diagonals
			{
				ev.set(Event::Type(event2), isPushed);
			}
	}
}

static void updateVirtualDPad(EmuApp &app, Console &console, PaddleRegionMode mode)
{
	auto leftController = console.leftController().type();
	if(leftController == Controller::Type::Paddles)
	{
		app.defaultVController().setGamepadDPadIsEnabled(mode == PaddleRegionMode::OFF);
	}
	else
	{
		app.defaultVController().setGamepadDPadIsEnabled(leftController != Controller::Type::Keyboard);
	}
}

void A2600System::updatePaddlesRegionMode(EmuApp &app, PaddleRegionMode mode)
{
	optionPaddleAnalogRegion = (uint8_t)mode;
	updateVirtualDPad(app, osystem.console(), mode);
}

void A2600System::setControllerType(EmuApp &app, Console &console, Controller::Type type)
{
	static constexpr std::array<unsigned, 2> js1ButtonCodes{vcsKeyIdxJSBtn1, vcsKeyIdxJSBtn1Turbo};
	static constexpr std::array<unsigned, 2> js2ButtonCodes{vcsKeyIdxJSBtn2, vcsKeyIdxJSBtn2Turbo};
	static constexpr std::array<unsigned, 2> js3ButtonCodes{vcsKeyIdxJSBtn3, vcsKeyIdxJSBtn3Turbo};
	if(type == Controller::Type::Unknown)
		type = autoDetectedInput1;
	if(type == Controller::Type::Genesis)
	{
		app.setDisabledInputKeys(concatToArrayNow<kbButtonCodes, js3ButtonCodes>);
	}
	else if(type == Controller::Type::BoosterGrip)
	{
		app.setDisabledInputKeys(kbButtonCodes);
	}
	else if(type == Controller::Type::Keyboard)
	{
		app.setDisabledInputKeys(concatToArrayNow<js1ButtonCodes, js2ButtonCodes, js3ButtonCodes>);
	}
	else // joystick
	{
		app.setDisabledInputKeys(concatToArrayNow<kbButtonCodes, js2ButtonCodes, js3ButtonCodes>);
	}
	updateVirtualDPad(app, console, (PaddleRegionMode)optionPaddleAnalogRegion.val);
	updateJoytickMapping(app, type);
	Controller &currentController = console.leftController();
	if(currentController.type() == type)
	{
		logMsg("using controller type:%s", asString(type));
		return;
	}
	auto props = console.properties();
	props.set(PropType::Controller_Left, Controller::getPropName(type));
	props.set(PropType::Controller_Right, Controller::getPropName(type));
	const string& md5 = props.get(PropType::Cart_MD5);
	console.setProperties(props);
	console.setControllers(md5);
	if(Config::DEBUG_BUILD)
	{
		logMsg("current controller name in console object:%s", console.leftController().name().c_str());
	}
	logMsg("set controller to type:%s", asString(type));
}

Controller::Type limitToSupportedControllerTypes(Controller::Type type)
{
	switch(type)
	{
		case Controller::Type::Joystick:
		case Controller::Type::Genesis:
		case Controller::Type::BoosterGrip:
		case Controller::Type::Keyboard:
		case Controller::Type::Paddles:
			return type;
		default:
			return Controller::Type::Joystick;
	}
}

const char *asString(Controller::Type type)
{
	switch(type)
	{
		case Controller::Type::Joystick: return "Joystick";
		case Controller::Type::Genesis: return "Genesis Gamepad";
		case Controller::Type::BoosterGrip: return "Booster Grip";
		case Controller::Type::Keyboard: return "Keyboard";
		case Controller::Type::Paddles: return "Paddles";
		default: return "Auto";
	}
}

bool A2600System::updatePaddle(Input::DragTrackerState dragState)
{
	auto regionMode = (PaddleRegionMode)optionPaddleAnalogRegion.val;
	if(regionMode == PaddleRegionMode::OFF)
		return false;
	auto &app = osystem.app();
	int regionXStart = 0;
	int regionXEnd = app.viewController().inputView().viewRect().size().x;
	if(regionMode == PaddleRegionMode::LEFT)
	{
		regionXEnd /= 2;
	}
	else if(regionMode == PaddleRegionMode::RIGHT)
	{
		regionXStart = regionXEnd / 2;
	}
	auto pos = IG::remap(dragState.pos().x, regionXStart, regionXEnd, -32768 / 2, 32767 / 2);
	pos = std::clamp(pos, -32768, 32767);
	auto evType = app.defaultVController().inputPlayer() == 0 ? Event::LeftPaddleAAnalog : Event::LeftPaddleBAnalog;
	osystem.eventHandler().event().set(evType, pos);
	//logMsg("set paddle position:%d", pos);
	return true;
}

bool A2600System::onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState dragState, IG::WindowRect)
{
	switch(osystem.console().leftController().type())
	{
		case Controller::Type::Paddles:
		{
			return updatePaddle(dragState);
		}
		default:
			return false;
	}
}

bool A2600System::onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState dragState,
	Input::DragTrackerState, IG::WindowRect)
{
	switch(osystem.console().leftController().type())
	{
		case Controller::Type::Paddles:
		{
			return updatePaddle(dragState);
		}
		default:
			return false;
	}
}

SystemInputDeviceDesc A2600System::inputDeviceDesc(int idx) const
{
	return jsDesc;
}

}
