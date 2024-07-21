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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/time/Time.hh>
#include <imagine/input/Event.hh>
#include <imagine/input/apple/AppleGameDevice.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/coreFoundation.h>
#include <imagine/util/algorithm.h>
#import <GameController/GameController.h>

namespace IG::Input
{

constexpr SystemLogger log{"GameController"};

static const char *appleGCButtonName(Key k);

AppleGameDevice::AppleGameDevice(ApplicationContext ctx, GCController *gcController):
	BaseDevice{0, Map::APPLE_GAME_CONTROLLER, {.gamepad = true}, [gcController.vendorName UTF8String]},
	ctx{ctx}, gcController_{(void*)CFBridgingRetain(gcController)}
{
	if(gcController.extendedGamepad)
	{
		typeFlags_.joystick = true;
		subtype_ = Subtype::APPLE_EXTENDED_GAMEPAD;
		axis[0] = {Input::AxisId::X};
		axis[1] = {Input::AxisId::Y};
		axis[2] = {Input::AxisId::Z};
		axis[3] = {Input::AxisId::RZ};
	}
	log.info("controller vendor:{}", name_);
}

AppleGameDevice::~AppleGameDevice()
{
	CFRelease(gcController_);
}

template <class T>
void AppleGameDevice::setGamepadBlocks(Device& dev, GCController*, T gamepad)
{
	gamepad.buttonA.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::A, value != 0.f);
		};
	gamepad.buttonB.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::B, value != 0.f);
		};
	gamepad.buttonX.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::X, value != 0.f);
		};
	gamepad.buttonY.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::Y, value != 0.f);
		};
	gamepad.leftShoulder.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::L1, value != 0.f);
		};
	gamepad.rightShoulder.valueChangedHandler =
		^(GCControllerButtonInput*, float value, [[maybe_unused]] BOOL pressed)
		{
			this->handleKey(dev, AppleGC::R1, value != 0.f);
		};
	gamepad.dpad.up.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::UP, pressed);
		};
	gamepad.dpad.down.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::DOWN, pressed);
		};
	gamepad.dpad.left.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::LEFT, pressed);
		};
	gamepad.dpad.right.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::RIGHT, pressed);
		};
}

void AppleGameDevice::setExtendedGamepadBlocks(Device &dev, GCController*, GCExtendedGamepad* extGamepad)
{
	extGamepad.leftTrigger.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::L2, pressed);
		};
	extGamepad.rightTrigger.valueChangedHandler =
		^(GCControllerButtonInput*, [[maybe_unused]] float value, BOOL pressed)
		{
			this->handleKey(dev, AppleGC::R2, pressed);
		};
	extGamepad.leftThumbstick.xAxis.valueChangedHandler =
		^(GCControllerAxisInput*, float value)
		{
			if(axis[0].dispatchInputEvent(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), dev, ctx.mainWindow()))
				ctx.endIdleByUserActivity();
		};
	extGamepad.leftThumbstick.yAxis.valueChangedHandler =
		^(GCControllerAxisInput*, float value)
		{
			if(axis[1].dispatchInputEvent(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), dev, ctx.mainWindow()))
				ctx.endIdleByUserActivity();
		};
	extGamepad.rightThumbstick.xAxis.valueChangedHandler =
		^(GCControllerAxisInput*, float value)
		{
			if(axis[2].dispatchInputEvent(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), dev, ctx.mainWindow()))
				ctx.endIdleByUserActivity();
		};
	extGamepad.rightThumbstick.yAxis.valueChangedHandler =
		^(GCControllerAxisInput*, float value)
		{
			if(axis[3].dispatchInputEvent(value, Map::APPLE_GAME_CONTROLLER, SteadyClock::now(), dev, ctx.mainWindow()))
				ctx.endIdleByUserActivity();
		};
}

void AppleGameDevice::handleKey(Device &dev, Key key, bool pressed, bool repeatable)
{
	if(pushState[key] == pressed)
		return;
	auto time = SteadyClock::now();
	pushState[key] = pressed;
	ctx.endIdleByUserActivity();
	KeyEvent event{Map::APPLE_GAME_CONTROLLER, key, pressed ? Action::PUSHED : Action::RELEASED, 0, 0, Source::GAMEPAD, time, &dev};
	if(repeatable)
		ctx.application().dispatchRepeatableKeyInputEvent(event);
	else
		ctx.application().dispatchKeyInputEvent(event);
}

const char *AppleGameDevice::keyName(Key k) const
{
	return appleGCButtonName(k);
}

bool AppleGameDevice::operator==(AppleGameDevice const& rhs) const
{
	return gcController_ == rhs.gcController_;
}

void AppleGameDevice::setKeys(Device &dev)
{
	auto extGamepad = gcController().extendedGamepad;
	if(extGamepad)
	{
		setGamepadBlocks(dev, gcController(), extGamepad);
		setExtendedGamepadBlocks(dev, gcController(), extGamepad);
	}
	else
	{
		auto gamepad = gcController().gamepad;
		setGamepadBlocks(dev, gcController(), gamepad);
	}
	gcController().controllerPausedHandler =
		^(GCController*)
		{
			this->handleKey(dev, AppleGC::PAUSE, true, false);
			this->handleKey(dev, AppleGC::PAUSE, false, false);
		};
}

static AppleGameDevice *deviceForGCController(Application &app, GCController *controller)
{
	for(auto &devPtr : app.inputDevices())
	{
		auto gameDevPtr = std::get_if<AppleGameDevice>(devPtr.get());
		if(gameDevPtr && gameDevPtr->gcController() == controller)
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
		log.info("controller {} already in list", (__bridge void*)controller);
		return;
	}
	log.info("adding controller:{}", (__bridge void*)controller);
	auto &dev = ctx.application().addInputDevice(ctx, std::make_unique<Input::Device>(std::in_place_type<AppleGameDevice>, ctx, controller), notify);
	getAs<AppleGameDevice>(dev).setKeys(dev);
}

static void removeController(ApplicationContext ctx, Application &app, GCController *controller)
{
	app.removeInputDeviceIf(ctx,
		[&](auto &devPtr)
		{
			auto gameDevPtr = std::get_if<AppleGameDevice>(devPtr.get());
			return gameDevPtr && gameDevPtr->gcController() == controller;
		}, true);
}

void initAppleGameControllers(ApplicationContext ctx)
{
	if(kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber_iOS_7_0)
	{
		return; // needs at least iOS 7
	}
	log.info("checking for game controllers");
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserverForName:GCControllerDidConnectNotification object:nil
		queue:nil usingBlock:
		^(NSNotification *note)
		{
			log.info("game controller connected");
			GCController *controller = note.object;
			addController(ctx, controller, true);
		}];
	[center addObserverForName:GCControllerDidDisconnectNotification object:nil
		queue:nil usingBlock:
		^(NSNotification *note)
		{
			log.info("game controller disconnected");
			GCController *controller = note.object;
			removeController(ctx, ctx.application(), controller);
		}];
	for(GCController *controller in [GCController controllers])
	{
		log.info("checking game controller:{}", (__bridge void*)controller);
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
