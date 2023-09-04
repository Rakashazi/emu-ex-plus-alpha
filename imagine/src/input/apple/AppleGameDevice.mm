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
#include <imagine/input/Event.hh>
#include <imagine/input/Device.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/coreFoundation.h>
#include <imagine/util/algorithm.h>
#import <GameController/GameController.h>

namespace IG::Input
{

static const char *appleGCButtonName(Key k);

struct AppleGameDevice : public Device
{
	ApplicationContext ctx{};
	GCController *gcController = nil;
	Input::Axis axis[4];
	bool pushState[AppleGC::COUNT]{};
	
	AppleGameDevice(ApplicationContext ctx, GCController *gcController):
		Device{0, Map::APPLE_GAME_CONTROLLER, {.gamepad = true}, [gcController.vendorName UTF8String]},
		ctx{ctx}, gcController{gcController}
	{
		auto extGamepad = gcController.extendedGamepad;
		if(extGamepad)
		{
			typeFlags_.joystick = true;
			subtype_ = Subtype::APPLE_EXTENDED_GAMEPAD;
			axis[0] = {*this, Input::AxisId::X};
			axis[1] = {*this, Input::AxisId::Y};
			axis[2] = {*this, Input::AxisId::Z};
			axis[3] = {*this, Input::AxisId::RZ};
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
		logMsg("controller vendor:%s", name().data());
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
		extGamepad.leftThumbstick.xAxis.valueChangedHandler =
			^(GCControllerAxisInput *, float value)
			{
				if(axis[0].update(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), *this, ctx.mainWindow()))
					ctx.endIdleByUserActivity();
			};
		extGamepad.leftThumbstick.yAxis.valueChangedHandler =
			^(GCControllerAxisInput *, float value)
			{
				if(axis[1].update(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), *this, ctx.mainWindow()))
					ctx.endIdleByUserActivity();
			};
		extGamepad.rightThumbstick.xAxis.valueChangedHandler =
			^(GCControllerAxisInput *, float value)
			{
				if(axis[2].update(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), *this, ctx.mainWindow()))
					ctx.endIdleByUserActivity();
			};
		extGamepad.rightThumbstick.yAxis.valueChangedHandler =
			^(GCControllerAxisInput *, float value)
			{
				if(axis[3].update(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), *this, ctx.mainWindow()))
					ctx.endIdleByUserActivity();
			};
	}
	
	void handleKey(Key key, Key sysKey, bool pressed, bool repeatable = true)
	{
		assert(key < AppleGC::COUNT);
		if(pushState[key] == pressed)
			return;
		auto time = SteadyClock::now();
		pushState[key] = pressed;
		ctx.endIdleByUserActivity();
		KeyEvent event{Map::APPLE_GAME_CONTROLLER, key, sysKey, pressed ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, this};
		if(repeatable)
			ctx.application().dispatchRepeatableKeyInputEvent(event);
		else
			ctx.application().dispatchKeyInputEvent(event);
		
	}

	std::span<Input::Axis> motionAxes()
	{
		return subtype_ == Subtype::APPLE_EXTENDED_GAMEPAD ? axis : std::span<Input::Axis>{};
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

static AppleGameDevice *asAppleGameDevice(Input::Device &dev)
{
	return dev.map() == Map::APPLE_GAME_CONTROLLER ? static_cast<AppleGameDevice*>(&dev) : nullptr;
}

static AppleGameDevice *deviceForGCController(Application &app, GCController *controller)
{
	for(auto &devPtr : app.inputDevices())
	{
		auto gameDevPtr = asAppleGameDevice(*devPtr);
		if(gameDevPtr && gameDevPtr->gcController == controller)
			return gameDevPtr;
	}
	return nullptr;
}

static bool devListContainsController(Application &app, GCController *controller)
{
	return deviceForGCController(app, controller);
}

static void addController(ApplicationContext ctx, GCController *controller, bool notify)
{
	if(devListContainsController(ctx.application(), controller))
	{
		logMsg("controller %p already in list", controller);
		return;
	}
	logMsg("adding controller: %p", controller);
	ctx.application().addInputDevice(ctx, std::make_unique<AppleGameDevice>(ctx, controller), notify);
}

static void removeController(ApplicationContext ctx, Application &app, GCController *controller)
{
	app.removeInputDeviceIf(ctx,
		[&](auto &devPtr)
		{
			auto gameDevPtr = asAppleGameDevice(*devPtr);
			return gameDevPtr && gameDevPtr->gcController == controller;
		}, true);
}

void initAppleGameControllers(ApplicationContext ctx)
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
			removeController(ctx, ctx.application(), controller);
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

std::pair<Input::Key, Input::Key> appleJoystickKeys(Input::AxisId axisId)
{
	switch(axisId)
	{
		case Input::AxisId::X: return {Input::AppleGC::LSTICK_LEFT, Input::AppleGC::LSTICK_RIGHT};
		case Input::AxisId::Y: return {Input::AppleGC::LSTICK_DOWN, Input::AppleGC::LSTICK_UP};
		case Input::AxisId::Z: return {Input::AppleGC::RSTICK_LEFT, Input::AppleGC::RSTICK_RIGHT};
		case Input::AxisId::RZ: return {Input::AppleGC::RSTICK_DOWN, Input::AppleGC::RSTICK_UP};
		default: return {};
	}
}

}
