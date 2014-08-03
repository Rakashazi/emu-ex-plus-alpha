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

#define LOGTAG "Input"
#include <android/input.h>
#include <android/asset_manager.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../../input/android/private.hh"

namespace Base
{

void processInputWithGetEvent(AInputQueue *inputQueue);
static void processInputWithHasEvents(AInputQueue *inputQueue);

AInputQueue *inputQueue = nullptr;
static int aHardKeyboardState = 0, aKeyboardType = ACONFIGURATION_KEYBOARD_NOKEYS, aHasHardKeyboard = 0;
void (*processInput)(AInputQueue *inputQueue) = Base::processInputWithHasEvents;
static bool trackballNav = false;

// NAVHIDDEN_* mirrors KEYSHIDDEN_*

static const char *hardKeyboardNavStateToStr(int state)
{
	switch(state)
	{
		case ACONFIGURATION_KEYSHIDDEN_ANY: return "undefined";
		case ACONFIGURATION_KEYSHIDDEN_NO: return "no";
		case ACONFIGURATION_KEYSHIDDEN_YES: return "yes";
		case ACONFIGURATION_KEYSHIDDEN_SOFT:  return "soft";
		default: return "unknown";
	}
}

bool hasHardKeyboard() { return aHasHardKeyboard; }
int hardKeyboardState() { return aHardKeyboardState; }

static void setHardKeyboardState(int hardKeyboardState)
{
	if(aHardKeyboardState != hardKeyboardState)
	{
		aHardKeyboardState = hardKeyboardState;
		logMsg("hard keyboard hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));
		Input::setBuiltInKeyboardState(aHardKeyboardState == ACONFIGURATION_KEYSHIDDEN_NO);
	}
}

static void setKeyboardType(int keyboardType)
{
	if(aKeyboardType != keyboardType)
	{
		logMsg("keyboard changed: %d", keyboardType);
		aKeyboardType = keyboardType;
	}
}

int keyboardType()
{
	return aKeyboardType;
}

bool hasTrackball()
{
	return trackballNav;
}

void initInputConfig(AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navigationState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	if(trackballNav)
		logMsg("detected trackball");

	aHardKeyboardState = Input::hasXperiaPlayGamepad() ? navigationState : hardKeyboardState;
	logMsg("keyboard/nav hidden: %s", hardKeyboardNavStateToStr(aHardKeyboardState));

	aKeyboardType = keyboard;
	if(aKeyboardType != ACONFIGURATION_KEYBOARD_NOKEYS)
		logMsg("keyboard type: %d", aKeyboardType);
}

void changeInputConfig(JNIEnv *jEnv, AConfiguration *config)
{
	auto hardKeyboardState = AConfiguration_getKeysHidden(config);
	auto navState = AConfiguration_getNavHidden(config);
	auto keyboard = AConfiguration_getKeyboard(config);
	//trackballNav = AConfiguration_getNavigation(config) == ACONFIGURATION_NAVIGATION_TRACKBALL;
	logMsg("config change, keyboard: %s, navigation: %s", hardKeyboardNavStateToStr(hardKeyboardState), hardKeyboardNavStateToStr(navState));
	setHardKeyboardState(Input::hasXperiaPlayGamepad() ? navState : hardKeyboardState);
}

static void handleInputEvent(AInputQueue *inputQueue, AInputEvent* event)
{
	//logMsg("input event start");
	if(unlikely(!deviceWindow()))
	{
		logMsg("ignoring input with uninitialized window");
		AInputQueue_finishEvent(inputQueue, event, 0);
		return;
	}
	if(unlikely(Input::sendInputToIME && AInputQueue_preDispatchEvent(inputQueue, event)))
	{
		//logMsg("input event used by pre-dispatch");
		return;
	}
	auto handled = Input::onInputEvent(event, *deviceWindow());
	AInputQueue_finishEvent(inputQueue, event, handled);
	//logMsg("input event end: %s", handled ? "handled" : "not handled");
}

// Use on Android 4.1+ to fix a possible ANR where the OS
// claims we haven't processed all input events even though we have.
// This only seems to happen under heavy input event load, like
// when using multiple joysticks. Everything seems to work
// properly if we keep calling AInputQueue_getEvent until
// it returns an error instead of using AInputQueue_hasEvents
// and no warnings are printed to logcat unlike earlier
// Android versions
void processInputWithGetEvent(AInputQueue *inputQueue)
{
	int events = 0;
	AInputEvent* event = nullptr;
	while(AInputQueue_getEvent(inputQueue, &event) >= 0)
	{
		handleInputEvent(inputQueue, event);
		events++;
	}
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
}

static void processInputWithHasEvents(AInputQueue *inputQueue)
{
	int events = 0;
	int32_t hasEventsRet;
	// Note: never call AInputQueue_hasEvents on first iteration since it may return 0 even if
	// events are present if they were pre-dispatched, leading to an endless stream of callbacks
	do
	{
		AInputEvent* event = nullptr;
		if(AInputQueue_getEvent(inputQueue, &event) < 0)
		{
			logWarn("error getting input event from queue");
			break;
		}
		handleInputEvent(inputQueue, event);
		events++;
	} while((hasEventsRet = AInputQueue_hasEvents(inputQueue)) == 1);
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
	if(hasEventsRet < 0)
	{
		logWarn("error %d in AInputQueue_hasEvents", hasEventsRet);
	}
}

}
