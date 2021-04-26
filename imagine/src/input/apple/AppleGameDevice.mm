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

#define LOGTAG "GameController"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Input.hh>
#include <imagine/input/Device.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/coreFoundation.h>
#include <imagine/util/string.h>
#include <imagine/util/algorithm.h>
#import <GameController/GameController.h>

namespace Input
{

static const char *appleGCButtonName(Key k);

struct AppleGameDevice : public Device
{
	Base::ApplicationContext ctx{};
	GCController *gcController = nil;
	uint32_t joystickAxisAsDpadBits_ = 0;
	bool pushState[AppleGC::COUNT]{};
	
	AppleGameDevice(Base::ApplicationContext ctx, GCController *gcController, uint32_t enumId):
		Device{enumId, Map::APPLE_GAME_CONTROLLER, TYPE_BIT_GAMEPAD, [gcController.vendorName UTF8String]},
		ctx{ctx}, gcController{gcController}
	{
		auto extGamepad = gcController.extendedGamepad;
		if(extGamepad)
		{
			type_ |= TYPE_BIT_JOYSTICK;
			subtype_ = SUBTYPE_APPLE_EXTENDED_GAMEPAD;
			setJoystickAxisAsDpadBits(AXIS_BIT_X | AXIS_BIT_Y);
			setGamepadBlocks(gcController, extGamepad);
			setExtendedGamepadBlocks(gcController, extGamepad);
		}
		else
		{
			auto gamepad = gcController.gamepad;
			setGamepadBlocks(gcController, gamepad);
		}
		gcController.controllerPausedHandler =
			^(GCController *controller)
			{
				this->handleKey(AppleGC::PAUSE, Keycode::MENU, true, false);
				this->handleKey(AppleGC::PAUSE, Keycode::MENU, false, false);
			};
		logMsg("controller %d with vendor: %s", devId, name());
	}
	
	template <class T>
	void setGamepadBlocks(GCController *controller, T gamepad)
	{
		gamepad.buttonA.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::A, Keycode::GAME_A, value != 0.f);
			};
		gamepad.buttonB.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::B, Keycode::GAME_B, value != 0.f);
			};
		gamepad.buttonX.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::X, Keycode::GAME_X, value != 0.f);
			};
		gamepad.buttonY.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::Y, Keycode::GAME_Y, value != 0.f);
			};
		gamepad.leftShoulder.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::L1, Keycode::GAME_L1, value != 0.f);
			};
		gamepad.rightShoulder.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::R1, Keycode::GAME_R1, value != 0.f);
			};
		gamepad.dpad.up.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::UP, Keycode::UP, pressed);
			};
		gamepad.dpad.down.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::DOWN, Keycode::DOWN, pressed);
			};
		gamepad.dpad.left.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::LEFT, Keycode::LEFT, pressed);
			};
		gamepad.dpad.right.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::RIGHT, Keycode::RIGHT, pressed);
			};
	}
	
	void setExtendedGamepadBlocks(GCController *controller, GCExtendedGamepad *extGamepad)
	{
		extGamepad.leftTrigger.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::L2, Keycode::GAME_L2, pressed);
			};
		extGamepad.rightTrigger.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				this->handleKey(AppleGC::R2, Keycode::GAME_R2, pressed);
			};
		extGamepad.leftThumbstick.up.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_Y) ? AppleGC::UP : AppleGC::LSTICK_UP;
				Key sysKey = key == AppleGC::UP ? Keycode::UP : Keycode::JS1_YAXIS_NEG;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.leftThumbstick.down.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_Y) ? AppleGC::DOWN : AppleGC::LSTICK_DOWN;
				Key sysKey = key == AppleGC::DOWN ? Keycode::DOWN : Keycode::JS1_YAXIS_POS;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.leftThumbstick.left.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_X) ? AppleGC::LEFT : AppleGC::LSTICK_LEFT;
				Key sysKey = key == AppleGC::LEFT ? Keycode::LEFT : Keycode::JS1_XAXIS_NEG;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.leftThumbstick.right.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_X) ? AppleGC::RIGHT : AppleGC::LSTICK_RIGHT;
				Key sysKey = key == AppleGC::RIGHT ? Keycode::RIGHT : Keycode::JS1_XAXIS_POS;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.rightThumbstick.up.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_RZ) ? AppleGC::UP : AppleGC::RSTICK_UP;
				Key sysKey = key == AppleGC::UP ? Keycode::UP : Keycode::JS2_YAXIS_NEG;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.rightThumbstick.down.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_RZ) ? AppleGC::DOWN : AppleGC::RSTICK_DOWN;
				Key sysKey = key == AppleGC::DOWN ? Keycode::DOWN : Keycode::JS2_YAXIS_POS;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.rightThumbstick.left.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_Z) ? AppleGC::LEFT : AppleGC::RSTICK_LEFT;
				Key sysKey = key == AppleGC::LEFT ? Keycode::LEFT : Keycode::JS2_XAXIS_NEG;
				this->handleKey(key, sysKey, pressed);
			};
		extGamepad.rightThumbstick.right.valueChangedHandler =
			^(GCControllerButtonInput *button, float value, BOOL pressed)
			{
				Key key = (this->joystickAxisAsDpadBits_ & AXIS_BIT_Z) ? AppleGC::RIGHT : AppleGC::RSTICK_RIGHT;
				Key sysKey = key == AppleGC::RIGHT ? Keycode::RIGHT : Keycode::JS2_XAXIS_POS;
				this->handleKey(key, sysKey, pressed);
			};
	}
	
	void handleKey(Key key, Key sysKey, bool pressed, bool repeatable = true)
	{
		assert(key < AppleGC::COUNT);
		if(pushState[key] == pressed)
			return;
		auto time = IG::steadyClockTimestamp();
		pushState[key] = pressed;
		ctx.endIdleByUserActivity();
		Event event{enumId(), Map::APPLE_GAME_CONTROLLER, key, sysKey, pressed ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
		if(repeatable)
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		else
			ctx.application().dispatchKeyInputEvent(event);
		
	}

	void setJoystickAxisAsDpadBits(uint32_t axisMask) final
	{
		joystickAxisAsDpadBits_ = axisMask;
	}
	
	uint32_t joystickAxisAsDpadBits() final
	{
		return joystickAxisAsDpadBits_;
	}

	uint32_t joystickAxisBits() final
	{
		return subtype_ == SUBTYPE_APPLE_EXTENDED_GAMEPAD ? (AXIS_BITS_STICK_1 | AXIS_BITS_STICK_2) : 0;
	}

	uint32_t joystickAxisAsDpadBitsDefault() final
	{
		return AXIS_BITS_STICK_1;
	}
	
	const char *keyName(Key k) const final
	{
		return appleGCButtonName(k);
	}
	
	bool operator ==(AppleGameDevice const& rhs) const
	{
		return gcController == rhs.gcController;
	}
};

static std::vector<std::unique_ptr<AppleGameDevice>> gcList{};

static uint32_t findFreeDevId()
{
	uint32_t id[5]{};
	for(auto &e : gcList)
	{
		if(e->enumId() < std::size(id))
			id[e->enumId()] = 1;
	}
	for(const auto &e : id)
	{
		if(e == 0)
			return &e - id;
	}
	logWarn("too many devices to enumerate");
	return 0;
}

static AppleGameDevice *deviceForGCController(GCController *controller)
{
	for(auto &e : gcList)
	{
		if(e->gcController == controller)
			return e.get();
	}
	return nullptr;
}

static bool devListContainsController(GCController *controller)
{
	for(auto &e : gcList)
	{
		if(e->gcController == controller)
			return true;
	}
	return false;
}

static void addController(Base::ApplicationContext ctx, GCController *controller, bool notify)
{
	if(devListContainsController(controller))
	{
		logMsg("controller %p already in list", controller);
		return;
	}
	logMsg("adding controller: %p", controller);
	auto &gc = gcList.emplace_back(std::make_unique<AppleGameDevice>(ctx, controller, findFreeDevId()));
	ctx.application().addSystemInputDevice(*gc, notify);
}

static void removeController(Base::ApplicationContext ctx, GCController *controller)
{
	if(auto removedDev = IG::moveOutIf(gcList, [&](std::unique_ptr<AppleGameDevice> &dev){ return dev->gcController == controller; });
		removedDev)
	{
		logMsg("removing controller: %p", controller);
		ctx.application().removeSystemInputDevice(*removedDev, true);
		logMsg("name: %s", removedDev->name());
		return;
	}
}

void initAppleGameControllers(Base::ApplicationContext ctx)
{
	if(kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_7_0)
	{
		return; // needs at least iOS 7
	}
	logMsg("checking for game controllers");
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserverForName:GCControllerDidConnectNotification object:nil
		queue:nil usingBlock:
		^(NSNotification *note)
		{
			logMsg("game controller connected");
			GCController *controller = note.object;
			addController(ctx, controller, true);
		}];
	[center addObserverForName:GCControllerDidDisconnectNotification object:nil
		queue:nil usingBlock:
		^(NSNotification *note)
		{
			logMsg("game controller disconnected");
			GCController *controller = note.object;
			removeController(ctx, controller);
		}];
	for(GCController *controller in [GCController controllers])
	{
		logMsg("checking game controller: %p", controller);
		addController(ctx, controller, false);
	}
}

static const char *appleGCButtonName(Key k)
{
	switch(k)
	{
		case 0: return "None";
		case AppleGC::A: return "A";
		case AppleGC::B: return "B";
		case AppleGC::X: return "X";
		case AppleGC::Y: return "Y";
		case AppleGC::L1: return "L1";
		case AppleGC::L2: return "L2";
		case AppleGC::R1: return "R1";
		case AppleGC::R2: return "R2";
		case AppleGC::PAUSE: return "Pause";
		case AppleGC::UP: return "Up";
		case AppleGC::RIGHT: return "Right";
		case AppleGC::DOWN: return "Down";
		case AppleGC::LEFT: return "Left";
		case AppleGC::LSTICK_UP: return "L:Up";
		case AppleGC::LSTICK_RIGHT: return "L:Right";
		case AppleGC::LSTICK_DOWN: return "L:Down";
		case AppleGC::LSTICK_LEFT: return "L:Left";
		case AppleGC::RSTICK_UP: return "R:Up";
		case AppleGC::RSTICK_RIGHT: return "R:Right";
		case AppleGC::RSTICK_DOWN: return "R:Down";
		case AppleGC::RSTICK_LEFT: return "R:Left";
	}
	return "";
}

}
