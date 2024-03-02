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
#include <emuframework/EmuViewController.hh>
#include <emuframework/keyRemappingUtils.hh>
#undef Debugger
#include "MainSystem.hh"
#include "MainApp.hh"
#include <imagine/util/math.hh>
#include <imagine/util/container/array.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"2600.emu"};
const int EmuSystem::maxPlayers = 2;

constexpr auto consoleKeyInfo = makeArray<KeyInfo>
(
	Event::ConsoleSelect,
	Event::ConsoleReset,
	Event::ConsoleLeftDiffToggle,
	Event::ConsoleRightDiffToggle,
	Event::ConsoleColorToggle
);

constexpr std::array dpadKeyInfo = makeArray<KeyInfo>
(
	Event::LeftJoystickUp,
	Event::LeftJoystickRight,
	Event::LeftJoystickDown,
	Event::LeftJoystickLeft
);

constexpr std::array triggerKeyInfo = makeArray<KeyInfo>
(
	Event::LeftJoystickFire,
	Event::LeftJoystickFire5,
	Event::LeftJoystickFire9
);

constexpr auto turboTriggerKeyInfo = turbo(triggerKeyInfo);

constexpr std::array kbKeyInfo = makeArray<KeyInfo>
(
	Event::LeftKeyboard1,
	Event::LeftKeyboard2,
	Event::LeftKeyboard3,
	Event::LeftKeyboard4,
	Event::LeftKeyboard5,
	Event::LeftKeyboard6,
	Event::LeftKeyboard7,
	Event::LeftKeyboard8,
	Event::LeftKeyboard9,
	Event::LeftKeyboardStar,
	Event::LeftKeyboard0,
	Event::LeftKeyboardPound
);

constexpr auto jsKeyInfo = concatToArrayNow<dpadKeyInfo, triggerKeyInfo, turboTriggerKeyInfo>;
constexpr auto js2KeyInfo = transpose(jsKeyInfo, 1);
constexpr auto kb2KeyInfo = transpose(kbKeyInfo, 1);

std::span<const KeyCategory> A2600App::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Joystick", jsKeyInfo},
		KeyCategory{"Joystick 2", js2KeyInfo, 1},
		KeyCategory{"Console Switches", consoleKeyInfo},
		KeyCategory{"Keyboard", kbKeyInfo},
		KeyCategory{"Keyboard 2", kb2KeyInfo, 1},
	};
	return categories;
}

std::string_view A2600App::systemKeyCodeToString(KeyCode c)
{
	switch(c)
	{
		case Event::LeftJoystickUp: return "Up";
		case Event::LeftJoystickRight: return "Right";
		case Event::LeftJoystickDown: return "Down";
		case Event::LeftJoystickLeft: return "Left";
		case Event::LeftJoystickFire: return "Button 1";
		case Event::LeftJoystickFire5: return "Button 2";
		case Event::LeftJoystickFire9: return "Button 3";
		case Event::ConsoleSelect: return "Select";
		case Event::ConsoleReset: return "Reset";
		case Event::ConsoleLeftDiffToggle: return "Left (P1) Difficulty";
		case Event::ConsoleRightDiffToggle: return "Right (P2) Difficulty";
		case Event::ConsoleColorToggle: return "Color/B&W";
		case Event::LeftKeyboard1: return "1";
		case Event::LeftKeyboard2: return "2";
		case Event::LeftKeyboard3: return "3";
		case Event::LeftKeyboard4: return "4";
		case Event::LeftKeyboard5: return "5";
		case Event::LeftKeyboard6: return "6";
		case Event::LeftKeyboard7: return "7";
		case Event::LeftKeyboard8: return "8";
		case Event::LeftKeyboard9: return "9";
		case Event::LeftKeyboardStar: return "*";
		case Event::LeftKeyboard0: return "0";
		case Event::LeftKeyboardPound: return "#";
		default: return "";
	}
}

std::span<const KeyConfigDesc> A2600App::defaultKeyConfigs()
{
	using namespace IG::Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{::Event::LeftJoystickUp, Keycode::UP},
		KeyMapping{::Event::LeftJoystickRight, Keycode::RIGHT},
		KeyMapping{::Event::LeftJoystickDown, Keycode::DOWN},
		KeyMapping{::Event::LeftJoystickLeft, Keycode::LEFT},
		KeyMapping{::Event::LeftJoystickFire, Keycode::Z},
		KeyMapping{::Event::LeftJoystickFire5, Keycode::X},
		KeyMapping{::Event::LeftJoystickFire9, Keycode::C},
		KeyMapping{::Event::ConsoleSelect, Keycode::SPACE},
		KeyMapping{::Event::ConsoleReset, Keycode::ENTER},
		KeyMapping{::Event::ConsoleLeftDiffToggle, Keycode::A},
		KeyMapping{::Event::ConsoleRightDiffToggle, Keycode::S},
		KeyMapping{::Event::ConsoleColorToggle, Keycode::D},
		KeyMapping{::Event::LeftKeyboard1, Keycode::_1},
		KeyMapping{::Event::LeftKeyboard2, Keycode::_2},
		KeyMapping{::Event::LeftKeyboard3, Keycode::_3},
		KeyMapping{::Event::LeftKeyboard4, Keycode::_4},
		KeyMapping{::Event::LeftKeyboard5, Keycode::_5},
		KeyMapping{::Event::LeftKeyboard6, Keycode::_6},
		KeyMapping{::Event::LeftKeyboard7, Keycode::_7},
		KeyMapping{::Event::LeftKeyboard8, Keycode::_8},
		KeyMapping{::Event::LeftKeyboard9, Keycode::_9},
		KeyMapping{::Event::LeftKeyboardStar, Keycode::MINUS},
		KeyMapping{::Event::LeftKeyboard0, Keycode::_0},
		KeyMapping{::Event::LeftKeyboardPound, Keycode::EQUALS},
		KeyMapping{transpose(::Event::LeftKeyboard1, 1), Keycode::Q},
		KeyMapping{transpose(::Event::LeftKeyboard2, 1), Keycode::W},
		KeyMapping{transpose(::Event::LeftKeyboard3, 1), Keycode::E},
		KeyMapping{transpose(::Event::LeftKeyboard4, 1), Keycode::R},
		KeyMapping{transpose(::Event::LeftKeyboard5, 1), Keycode::T},
		KeyMapping{transpose(::Event::LeftKeyboard6, 1), Keycode::Y},
		KeyMapping{transpose(::Event::LeftKeyboard7, 1), Keycode::U},
		KeyMapping{transpose(::Event::LeftKeyboard8, 1), Keycode::I},
		KeyMapping{transpose(::Event::LeftKeyboard9, 1), Keycode::O},
		KeyMapping{transpose(::Event::LeftKeyboardStar, 1), Keycode::P},
		KeyMapping{transpose(::Event::LeftKeyboard0, 1), Keycode::LEFT_BRACKET},
		KeyMapping{transpose(::Event::LeftKeyboardPound, 1), Keycode::RIGHT_BRACKET},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{::Event::LeftJoystickUp, Keycode::UP},
		KeyMapping{::Event::LeftJoystickRight, Keycode::RIGHT},
		KeyMapping{::Event::LeftJoystickDown, Keycode::DOWN},
		KeyMapping{::Event::LeftJoystickLeft, Keycode::LEFT},
		KeyMapping{::Event::LeftJoystickFire, Keycode::GAME_A},
		KeyMapping{::Event::LeftJoystickFire5, Keycode::GAME_X},
		KeyMapping{::Event::ConsoleSelect, Keycode::GAME_R1},
		KeyMapping{::Event::ConsoleReset, Keycode::GAME_START},
		KeyMapping{::Event::ConsoleLeftDiffToggle, Keycode::GAME_Y},
		KeyMapping{::Event::ConsoleRightDiffToggle, Keycode::GAME_B},
		KeyMapping{::Event::ConsoleColorToggle, Keycode::GAME_L1},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{::Event::LeftJoystickUp, WiimoteKey::UP},
		KeyMapping{::Event::LeftJoystickRight, WiimoteKey::RIGHT},
		KeyMapping{::Event::LeftJoystickDown, WiimoteKey::DOWN},
		KeyMapping{::Event::LeftJoystickLeft, WiimoteKey::LEFT},
		KeyMapping{::Event::LeftJoystickFire, WiimoteKey::_1},
		KeyMapping{::Event::LeftJoystickFire5, WiimoteKey::_2},
		KeyMapping{::Event::ConsoleSelect, WiimoteKey::MINUS},
		KeyMapping{::Event::ConsoleReset, WiimoteKey::PLUS},
		KeyMapping{::Event::ConsoleLeftDiffToggle, WiimoteKey::A},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool A2600App::allowsTurboModifier(KeyCode c)
{
	switch(c)
	{
		case Event::LeftJoystickFire ... Event::LeftJoystickFire9:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	constexpr F2Size imageSize{512, 256};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc A2600App::vControllerAssetDesc(KeyInfo key) const
{
	static constexpr struct VirtualControllerAssets
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

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(key[0])
	{
		case Event::LeftJoystickFire: return virtualControllerAssets.jsBtn1;
		case Event::LeftJoystickFire5: return virtualControllerAssets.jsBtn2;
		case Event::LeftJoystickFire9: return virtualControllerAssets.jsBtn3;
		case Event::LeftKeyboard1 ... Event::LeftKeyboardPound:
			return (&virtualControllerAssets.one)[key[0] - to_underlying(Event::LeftKeyboard1)];
		case Event::ConsoleSelect: return virtualControllerAssets.select;
		case Event::ConsoleLeftDiffToggle: return virtualControllerAssets.p1Diff;
		case Event::ConsoleRightDiffToggle: return virtualControllerAssets.p2Diff;
		case Event::ConsoleReset: return virtualControllerAssets.reset;
		case Event::ConsoleColorToggle: return virtualControllerAssets.colorBW;
		default: return virtualControllerAssets.blank;
	}
}

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

void A2600System::handleInputAction(EmuApp *app, InputAction act)
{
	auto &ev = osystem.eventHandler().event();
	switch(act.code)
	{
		case Event::ConsoleLeftDiffToggle:
			if(!act.isPushed())
				break;
			p1DiffB ^= true;
			if(app)
			{
				app->postMessage(1, false, p1DiffB ? "P1 Difficulty -> B" : "P1 Difficulty -> A");
			}
			ev.set(Event::ConsoleLeftDiffB, p1DiffB);
			ev.set(Event::ConsoleLeftDiffA, !p1DiffB);
			break;
		case Event::ConsoleRightDiffToggle:
			if(!act.isPushed())
				break;
			p2DiffB ^= true;
			if(app)
			{
				app->postMessage(1, false, p2DiffB ? "P2 Difficulty -> B" : "P2 Difficulty -> A");
			}
			ev.set(Event::ConsoleRightDiffB, p2DiffB);
			ev.set(Event::ConsoleRightDiffA, !p2DiffB);
			break;
		case Event::ConsoleColorToggle:
			if(!act.isPushed())
				break;
			vcsColor ^= true;
			if(app)
			{
				app->postMessage(1, false, vcsColor ? "Color Switch -> Color" : "Color Switch -> B&W");
			}
			ev.set(Event::ConsoleColor, vcsColor);
			ev.set(Event::ConsoleBlackWhite, !vcsColor);
			break;
		default:
		{
			auto e = [&] -> Event::Type
			{
				bool isLeftPort = act.flags.deviceId == 0;
				if(isLeftPort)
				{
					switch(act.code)
					{
						case Event::LeftJoystickRight: return jsRightMap[0];
						case Event::LeftJoystickLeft: return jsLeftMap[0];
						case Event::LeftJoystickFire: return jsFireMap[0];
					}
				}
				else
				{
					switch(act.code)
					{
						case Event::LeftJoystickUp: return Event::RightJoystickUp;
						case Event::LeftJoystickRight: return jsRightMap[1];
						case Event::LeftJoystickDown: return Event::RightJoystickDown;
						case Event::LeftJoystickLeft: return jsLeftMap[1];
						case Event::LeftJoystickFire: return jsFireMap[1];
						case Event::LeftKeyboard1 ... Event::LeftKeyboardPound:
							return Event::Type(act.code + (Event::RightKeyboard1 - Event::LeftKeyboard1));
					}
				}
				return Event::Type(act.code);
			}();
			ev.set(e, act.isPushed());
			break;
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
	static constexpr std::array js1ButtonCodes{KeyCode(Event::LeftJoystickFire)};
	static constexpr std::array js2ButtonCodes{KeyCode(Event::LeftJoystickFire5)};
	static constexpr std::array js3ButtonCodes{KeyCode(Event::LeftJoystickFire9)};
	static constexpr std::array kbButtonCodes
	{
		KeyCode(Event::LeftKeyboard1),
		KeyCode(Event::LeftKeyboard2),
		KeyCode(Event::LeftKeyboard3),
		KeyCode(Event::LeftKeyboard4),
		KeyCode(Event::LeftKeyboard5),
		KeyCode(Event::LeftKeyboard6),
		KeyCode(Event::LeftKeyboard7),
		KeyCode(Event::LeftKeyboard8),
		KeyCode(Event::LeftKeyboard9),
		KeyCode(Event::LeftKeyboardStar),
		KeyCode(Event::LeftKeyboard0),
		KeyCode(Event::LeftKeyboardPound),
	};
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
	updateVirtualDPad(app, console, (PaddleRegionMode)optionPaddleAnalogRegion.value());
	updateJoytickMapping(app, type);
	Controller &currentController = console.leftController();
	if(currentController.type() == type)
	{
		log.info("using controller type:{}", asString(type));
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
		log.info("current controller name in console object:%s", console.leftController().name());
	}
	log.info("set controller to type:{}", asString(type));
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
	auto regionMode = (PaddleRegionMode)optionPaddleAnalogRegion.value();
	if(regionMode == PaddleRegionMode::OFF)
		return false;
	auto &app = osystem.app();
	int regionXStart = 0;
	int regionXEnd = app.viewController().inputView.viewRect().size().x;
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
	//log.debug("set paddle position:{}", pos);
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
	static constexpr std::array jsComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Joystick Buttons", triggerKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Keyboard Buttons", kbKeyInfo, InputComponent::button, RB2DO, {.altConfig = true, .rowSize = 3}},
		InputComponentDesc{"Select", {&consoleKeyInfo[0], 1}, InputComponent::button, LB2DO},
		InputComponentDesc{"Reset", {&consoleKeyInfo[1], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"Console Buttons", consoleKeyInfo, InputComponent::button, RB2DO, {.altConfig = true}},
	};

	static constexpr SystemInputDeviceDesc jsDesc{"Joystick", jsComponents};

	return jsDesc;
}

}
