#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuInput.hh>
#include <emuframework/EmuSystem.hh>
#include <array>
#include <string_view>

namespace EmuEx
{

using namespace IG;

enum class AppKeyCode : KeyCode
{
	openMenu = 1,
	openContent,
	openSystemActions,
	saveState,
	loadState,
	decStateSlot,
	incStateSlot,
	fastForward,
	toggleFastForward,
	slowMotion,
	toggleSlowMotion,
	takeScreenshot,
	turboModifier,
	exitApp,
	rewind,
	softReset,
	hardReset,
	resetMenu,
	closeContent,
};

constexpr struct AppKeys
{
	KeyInfo
	openMenu = KeyInfo::appKey(AppKeyCode::openMenu),
	openContent = KeyInfo::appKey(AppKeyCode::openContent),
	closeContent = KeyInfo::appKey(AppKeyCode::closeContent),
	openSystemActions = KeyInfo::appKey(AppKeyCode::openSystemActions),
	saveState = KeyInfo::appKey(AppKeyCode::saveState),
	loadState = KeyInfo::appKey(AppKeyCode::loadState),
	decStateSlot = KeyInfo::appKey(AppKeyCode::decStateSlot),
	incStateSlot = KeyInfo::appKey(AppKeyCode::incStateSlot),
	fastForward = KeyInfo::appKey(AppKeyCode::fastForward),
	toggleFastForward = KeyInfo::appKey(AppKeyCode::toggleFastForward),
	slowMotion = KeyInfo::appKey(AppKeyCode::slowMotion),
	toggleSlowMotion = KeyInfo::appKey(AppKeyCode::toggleSlowMotion),
	rewind = KeyInfo::appKey(AppKeyCode::rewind),
	takeScreenshot = KeyInfo::appKey(AppKeyCode::takeScreenshot),
	turboModifier = KeyInfo::appKey(AppKeyCode::turboModifier),
	softReset = KeyInfo::appKey(AppKeyCode::softReset),
	hardReset = KeyInfo::appKey(AppKeyCode::hardReset),
	resetMenu = KeyInfo::appKey(AppKeyCode::resetMenu),
	exitApp = KeyInfo::appKey(AppKeyCode::exitApp);

	constexpr const KeyInfo *data() const { return &openMenu; }
	static constexpr size_t size() { return sizeof(AppKeys) / sizeof(KeyInfo); }
} appKeys;

constexpr KeyCategory appKeyCategory{"In-Emulation Actions", appKeys};

constexpr std::array genericGamepadAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
	KeyMapping{appKeys.fastForward, Input::Keycode::GAME_R2},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK},
};

constexpr std::array genericGamepadModifierAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
	KeyMapping{appKeys.fastForward, {Input::Keycode::GAME_R2, Input::Keycode::JS2_XAXIS_POS}},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK},
};

constexpr std::array genericWiimoteAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
};

constexpr std::array genericKeyboardAppKeyCodeMap
{
	KeyMapping{appKeys.openContent, {Input::Keycode::LCTRL, Input::Keycode::_2}},
	KeyMapping{appKeys.openSystemActions, Input::Keycode::MENU},
	KeyMapping{appKeys.saveState, {Input::Keycode::LCTRL, Input::Keycode::_1}},
	KeyMapping{appKeys.loadState, {Input::Keycode::LCTRL, Input::Keycode::_4}},
	KeyMapping{appKeys.decStateSlot, {Input::Keycode::LCTRL, Input::Keycode::LEFT_BRACKET}},
	KeyMapping{appKeys.incStateSlot, {Input::Keycode::LCTRL, Input::Keycode::RIGHT_BRACKET}},
	KeyMapping{appKeys.fastForward, {Input::Keycode::LCTRL, Input::Keycode::GRAVE}},
	KeyMapping{appKeys.rewind, {Input::Keycode::LCTRL, Input::Keycode::R}},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK_KEY},
};

constexpr std::array rightUIKeys{appKeys.openMenu};
constexpr std::array leftUIKeys{appKeys.toggleFastForward};
constexpr std::array rewindUIKeys{appKeys.rewind};

constexpr InputComponentDesc rightUIComponents{"Open Menu", rightUIKeys, InputComponent::ui, RT2DO};
constexpr InputComponentDesc leftUIComponents{"Toggle Fast-forward", leftUIKeys, InputComponent::ui, LT2DO};
constexpr InputComponentDesc rewindUIComponents{"Rewind One State", rewindUIKeys, InputComponent::ui, LT2DO};

std::string_view toString(AppKeyCode);

constexpr std::array playerNumStrings{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};

}
