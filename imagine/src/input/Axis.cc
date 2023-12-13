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

#include <imagine/input/Axis.hh>
#include <imagine/input/Device.hh>

namespace IG::Input
{

static std::pair<Key, Key> joystickKeys(AxisId axisId)
{
	switch(axisId)
	{
		case AxisId::X: return {Keycode::JS1_XAXIS_NEG, Keycode::JS1_XAXIS_POS};
		case AxisId::Y: return {Keycode::JS1_YAXIS_NEG, Keycode::JS1_YAXIS_POS};
		case AxisId::Z: return {Keycode::JS2_XAXIS_NEG, Keycode::JS2_XAXIS_POS};
		case AxisId::RZ: return {Keycode::JS2_YAXIS_NEG, Keycode::JS2_YAXIS_POS};
		case AxisId::RX: return {Keycode::JS3_XAXIS_NEG, Keycode::JS3_XAXIS_POS};
		case AxisId::RY: return {Keycode::JS3_YAXIS_NEG, Keycode::JS3_YAXIS_POS};
		case AxisId::HAT0X:
		case AxisId::HAT1X:
		case AxisId::HAT2X:
		case AxisId::HAT3X: return {Keycode::JS_POV_XAXIS_NEG, Keycode::JS_POV_XAXIS_POS};
		case AxisId::HAT0Y:
		case AxisId::HAT1Y:
		case AxisId::HAT2Y:
		case AxisId::HAT3Y: return {Keycode::JS_POV_YAXIS_NEG, Keycode::JS_POV_YAXIS_POS};
		case AxisId::RUDDER: return {Keycode::JS_RUDDER_AXIS_NEG, Keycode::JS_RUDDER_AXIS_POS};
		case AxisId::WHEEL: return {Keycode::JS_WHEEL_AXIS_NEG, Keycode::JS_WHEEL_AXIS_POS};
		case AxisId::LTRIGGER:
		case AxisId::BRAKE : return {0, Keycode::GAME_L2};
		case AxisId::RTRIGGER:
		case AxisId::GAS : return {0, Keycode::GAME_R2};
	}
	return {};
}

static std::pair<Key, Key> joystickKeys(Map map, AxisId axisId)
{
	switch(map)
	{
		case Map::SYSTEM: return joystickKeys(axisId);
		#ifdef CONFIG_INPUT_BLUETOOTH
		case Map::WIIMOTE:
		case Map::WII_CC: return ::IG::Wiimote::joystickKeys(map, axisId);
		case Map::ICONTROLPAD: return ::IG::IControlPad::joystickKeys(axisId);
		case Map::ZEEMOTE: return ::IG::Zeemote::joystickKeys(axisId);
		#endif
		#ifdef CONFIG_BLUETOOTH_SERVER
		case Map::PS3PAD: return ::IG::PS3Controller::joystickKeys(axisId);
		#endif
		#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
		case Map::APPLE_GAME_CONTROLLER: return appleJoystickKeys(axisId);
		#endif
		default: return {};
	}
}

Axis::Axis(Map map, AxisId id, float scaler):
	scaler{scaler},
	keyEmu
	{
		joystickKeys(map, id),
		joystickKeys(id)
	},
	id_{id} {}

static std::pair<Key, Key> emulatedDirectionKeys(Map map, AxisId id, bool invertY)
{
	auto keys = directionKeys(map);
	std::pair<Key, Key> yKeys = {keys.up, keys.down};
	if(invertY)
		std::swap(yKeys.first, yKeys.second);
	switch(id)
	{
		case AxisId::X:
		case AxisId::RX:
		case AxisId::Z:
		case AxisId::HAT0X:
		case AxisId::HAT1X:
		case AxisId::HAT2X:
		case AxisId::HAT3X: return {keys.left, keys.right};
		case AxisId::Y:
		case AxisId::RY:
		case AxisId::RZ:
		case AxisId::HAT0Y:
		case AxisId::HAT1Y:
		case AxisId::HAT2Y:
		case AxisId::HAT3Y: return yKeys;
		default: return {};
	}
}

void Axis::setEmulatesDirectionKeys(Map map, bool on)
{
	if(on)
	{
		const bool invertY = Config::Input::BLUETOOTH && map != Map::SYSTEM;
		auto dpadKeys = emulatedDirectionKeys(map, id(), invertY);
		if(!dpadKeys.first)
			return;
		keyEmu.key = dpadKeys;
		keyEmu.sysKey = emulatedDirectionKeys(Map::SYSTEM, id(), invertY);
	}
	else
	{
		keyEmu.key = joystickKeys(map, id());
		keyEmu.sysKey = joystickKeys(id());
	}
}

bool Axis::emulatesDirectionKeys() const
{
	return keyEmu.sysKey != joystickKeys(id());
}

AxisFlags Axis::idBit() const
{
	switch(id())
	{
		case AxisId::X: return {.x = true};
		case AxisId::Y: return {.y = true};
		case AxisId::Z: return {.z = true};
		case AxisId::RX: return {.rx = true};
		case AxisId::RY: return {.ry = true};
		case AxisId::RZ: return {.rz = true};
		case AxisId::HAT0X:
		case AxisId::HAT1X:
		case AxisId::HAT2X:
		case AxisId::HAT3X: return {.hatX = true};
		case AxisId::HAT0Y:
		case AxisId::HAT1Y:
		case AxisId::HAT2Y:
		case AxisId::HAT3Y: return {.hatY = true};
		case AxisId::LTRIGGER: return {.lTrigger = true};
		case AxisId::RTRIGGER: return {.rTrigger = true};
		case AxisId::RUDDER: return {.rudder = true};
		case AxisId::WHEEL: return {.wheel = true};
		case AxisId::GAS: return {.gas = true};
		case AxisId::BRAKE: return {.brake = true};
		default: return {};
	}
}

bool Axis::isTrigger() const
{
	switch(id())
	{
		case AxisId::LTRIGGER:
		case AxisId::RTRIGGER:
		case AxisId::GAS:
		case AxisId::BRAKE:
			return true;
		default: return false;
	}
}

bool Axis::update(float pos, Map map, SteadyClockTimePoint time, const Device &dev, Window &win, bool normalized)
{
	if(!normalized)
		pos *= scaler;
	return keyEmu.dispatch(pos, map, time, dev, win);
}

bool Axis::dispatchInputEvent(float pos, Map map, SteadyClockTimePoint time, const Device &dev, Window &win)
{
	if(!win.dispatchInputEvent(MotionEvent{map, 0,
		uint32_t(id()), Action::MOVED, pos, 0, 0, Source::JOYSTICK, time, &dev}))
	{
		return update(pos, map, time, dev, win, true);
	}
	return true;
}


AxisKeyEmu::UpdateKeys AxisKeyEmu::update(float pos)
{
	UpdateKeys keys;
	auto newState = (pos <= limit.first) ? -1 :
		(pos >= limit.second) ? 1 :
		0;
	if(newState != state)
	{
		const bool stateHigh = (state > 0);
		const bool stateLow = (state < 0);
		keys.released = stateHigh ? key.second : stateLow ? key.first : 0;
		keys.sysReleased = stateHigh ? sysKey.second : stateLow ? sysKey.first : 0;
		const bool newStateHigh = (newState > 0);
		const bool newStateLow = (newState < 0);
		keys.pushed = newStateHigh ? key.second : newStateLow ? key.first : 0;
		keys.sysPushed = newStateHigh ? sysKey.second : newStateLow ? sysKey.first : 0;
		keys.updated = true;
		state = (int8_t)newState;
	}
	return keys;
}

bool AxisKeyEmu::dispatch(float pos, Map map, SteadyClockTimePoint time, const Device &dev, Window &win)
{
	auto updateKeys = update(pos);
	auto src = Source::GAMEPAD;
	if(!updateKeys.updated)
	{
		return false; // no change
	}
	if(updateKeys.released)
	{
		win.dispatchRepeatableKeyInputEvent(KeyEvent{map, updateKeys.released, updateKeys.sysReleased, Action::RELEASED, 0, 0, src, time, &dev});
	}
	if(updateKeys.pushed)
	{
		KeyEvent event{map, updateKeys.pushed, updateKeys.sysPushed, Action::PUSHED, 0, 0, src, time, &dev};
		win.dispatchRepeatableKeyInputEvent(event);
	}
	return true;
}

}
