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
#include <android/api-level.h>
#include <imagine/time/Time.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "input.hh"
#include <android/input.h>

namespace Base
{

static constexpr int AINPUT_SOURCE_JOYSTICK = 0x01000010;
static constexpr int AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010;
static const char* aInputSourceToStr(uint32_t source);

static Input::AndroidInputDevice *deviceForInputId(Base::ApplicationContext ctx, Base::AndroidInputDeviceContainer &sysInputDev, int id)
{
	if(ctx.androidSDK() < 12)
	{
		// no multi-input device support
		assumeExpr(sysInputDev.size());
		return sysInputDev.front().get();
	}
	auto existingIt = std::find_if(sysInputDev.cbegin(), sysInputDev.cend(),
		[=](const auto &e) { return e->systemId() == id; });
	if(existingIt == sysInputDev.end())
	{
		return nullptr;
	}
	return existingIt->get();
}

static Time makeTimeFromMotionEvent(AInputEvent *event)
{
	return IG::Nanoseconds(AMotionEvent_getEventTime(event));
}

static Time makeTimeFromKeyEvent(AInputEvent *event)
{
	return IG::Nanoseconds(AKeyEvent_getEventTime(event));
}

static void mapKeycodesForSpecialDevices(const Input::Device &dev, int32_t &keyCode, int32_t &metaState,
	Input::Source &src, AInputEvent *event)
{
	using namespace Input;
	switch(dev.subtype())
	{
		bcase Device::SUBTYPE_XPERIA_PLAY:
		{
			if(!Config::MACHINE_IS_GENERIC_ARMV7)
				break;
			switch(keyCode)
			{
				bcase Keycode::BACK:
					if(metaState & AMETA_ALT_ON)
					{
						keyCode = Keycode::GAME_B;
						src = Source::GAMEPAD;
					}
				bcase Keycode::GAME_A ... Keycode::GAME_SELECT:
					src = Source::GAMEPAD;
			}
		}
		bcase Device::SUBTYPE_XBOX_360_CONTROLLER:
		{
			if(keyCode)
				break;
			// map d-pad on wireless controller adapter
			auto scanCode = AKeyEvent_getScanCode(event);
			switch(scanCode)
			{
				bcase 704: keyCode = Keycode::LEFT;
				bcase 705: keyCode = Keycode::RIGHT;
				bcase 706: keyCode = Keycode::UP;
				bcase 707: keyCode = Keycode::DOWN;
			}
		}
		bdefault: break;
	}
}

static const char *androidEventEnumToStr(uint32_t e)
{
	switch(e)
	{
		case AMOTION_EVENT_ACTION_DOWN: return "Down";
		case AMOTION_EVENT_ACTION_UP: return "Up";
		case AMOTION_EVENT_ACTION_MOVE: return "Move";
		case AMOTION_EVENT_ACTION_CANCEL: return "Cancel";
		case AMOTION_EVENT_ACTION_POINTER_DOWN: return "PDown";
		case AMOTION_EVENT_ACTION_POINTER_UP: return "PUp";
	}
	return "Unknown";
}

static const char *keyEventActionStr(uint32_t action)
{
	switch(action)
	{
		default: return "Unknown";
		case AKEY_EVENT_ACTION_DOWN: return "Down";
		case AKEY_EVENT_ACTION_UP: return "Up";
		case AKEY_EVENT_ACTION_MULTIPLE: return "Multiple";
	}
}

static bool isFromSource(int src, int srcTest)
{
	return (src & srcTest) == srcTest;
}

static void dispatchTouch(uint32_t idx, Input::Action action, TouchState &p, IG::Point2D<int> pos, Time time,
	bool isMouse, const Input::Device *device, Base::Window &win)
{
	//logMsg("pointer: %d action: %s @ %d,%d", idx, eventActionToStr(action), pos.x, pos.y);
	uint32_t metaState = action == Input::Action::RELEASED ? 0 : IG::bit(Input::Pointer::LBUTTON);
	auto src = isMouse ? Input::Source::MOUSE : Input::Source::TOUCHSCREEN;
	win.dispatchInputEvent(Input::Event{idx, Input::Map::POINTER, Input::Pointer::LBUTTON,
		metaState, action, pos.x, pos.y, (int)idx, src, time, device});
}

static bool processTouchEvent(TouchStateArray &m, int action, int x, int y, int pid, Time time, bool isMouse,
	const Input::Device *device, Base::Window &win)
{
	//logMsg("%s action: %s from id %d @ %d,%d @ time %llu",
	//	isMouse ? "mouse" : "touch", androidEventEnumToStr(action), pid, x, y, (unsigned long long)time.nSecs());
	auto pos = win.transformInputPos({x, y});
	switch(action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//logMsg("touch down for %d", pid);
			for(int i = 0; auto &p : m) // find a free touch element
			{
				if(p.id == -1)
				{
					p.id = pid;
					p.isTouching = true;
					dispatchTouch(i, Input::Action::PUSHED, p, pos, time, isMouse, device, win);
					break;
				}
				i++;
			}
		bcase AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_CANCEL:
			for(int i = 0; auto &p : m)
			{
				if(p.isTouching)
				{
					//logMsg("touch up for %d from gesture end", p_i);
					p.id = -1;
					p.isTouching = false;
					auto touchAction = action == AMOTION_EVENT_ACTION_UP ? Input::Action::RELEASED : Input::Action::CANCELED;
					dispatchTouch(i, touchAction, p, {x, y}, time, isMouse, device, win);
				}
				i++;
			}
		bcase AMOTION_EVENT_ACTION_POINTER_UP:
			//logMsg("touch up for %d", pid);
			for(int i = 0; auto &p : m) // find the touch element
			{
				if(p.id == pid)
				{
					p.id = -1;
					p.isTouching = false;
					dispatchTouch(i, Input::Action::RELEASED, p, pos, time, isMouse, device, win);
					break;
				}
				i++;
			}
		bdefault:
			// move event
			//logMsg("event id %d", action);
			for(int i = 0; auto &p : m) // find the touch element
			{
				if(p.id == pid)
				{
					dispatchTouch(i, Input::Action::MOVED, p, pos, time, isMouse, device, win);
					break;
				}
				i++;
			}
	}

	/*logMsg("pointer state:");
	for(auto &p : m)
	{
		if(p.id != -1)
			logMsg("id: %d x: %d y: %d", p.id, p.dragState.x, p.dragState.y);
	}*/

	return 1;
}

bool AndroidApplication::processInputEvent(AInputEvent* event, Base::Window &win)
{
	using namespace Input;
	auto type = AInputEvent_getType(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			auto source = AInputEvent_getSource(event);
			int eventAction = AMotionEvent_getAction(event);
			//logMsg("motion event action:%d source:%d", eventAction, source);
			switch(source & AINPUT_SOURCE_CLASS_MASK)
			{
				case AINPUT_SOURCE_CLASS_POINTER:
				{
					auto dev = deviceForInputId(win.appContext(), sysInputDev, AInputEvent_getDeviceId(event));
					if(!dev) [[unlikely]]
					{
						if(Config::DEBUG_BUILD)
							logWarn("discarding pointer input from unknown device ID: %d", AInputEvent_getDeviceId(event));
						return false;
					}
					bool isMouse = isFromSource(source, AINPUT_SOURCE_MOUSE);
					uint32_t action = eventAction & AMOTION_EVENT_ACTION_MASK;
					if(action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
					{
						// touch gesture ended
						processTouchEvent(m, action,
								AMotionEvent_getX(event, 0),
								AMotionEvent_getY(event, 0),
								AMotionEvent_getPointerId(event, 0),
								makeTimeFromMotionEvent(event), isMouse, dev, win);
						return true;
					}
					uint32_t actionPIdx = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					int pointers = AMotionEvent_getPointerCount(event);
					//logMsg("motion event action:%d source:%d pointers:%d:%d",
					//	eventAction, source, pointers, actionPIdx);
					iterateTimes(pointers, i)
					{
						int pAction = action;
						// a pointer not performing the action just needs its position updated
						if(actionPIdx != i)
						{
							//logMsg("non-action pointer idx %d", i);
							pAction = AMOTION_EVENT_ACTION_MOVE;
						}
						processTouchEvent(m, pAction,
							AMotionEvent_getX(event, i),
							AMotionEvent_getY(event, i),
							AMotionEvent_getPointerId(event, i),
							makeTimeFromMotionEvent(event), isMouse, dev, win);
					}
					return true;
				}
				case AINPUT_SOURCE_CLASS_NAVIGATION:
				{
					//logMsg("from trackball");
					auto x = AMotionEvent_getX(event, 0);
					auto y = AMotionEvent_getY(event, 0);
					auto time = makeTimeFromMotionEvent(event);
					int iX = x * 1000., iY = y * 1000.;
					auto pos = win.transformInputPos({iX, iY});
					//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);
					auto src = Source::KEYBOARD;
					if(eventAction == AMOTION_EVENT_ACTION_MOVE)
						win.dispatchInputEvent({0, Map::REL_POINTER, 0, 0, Action::MOVED_RELATIVE, pos.x, pos.y, 0, Source::NAVIGATION, time, nullptr});
					else
					{
						Key key = Keycode::ENTER;
						win.dispatchInputEvent({0, Map::REL_POINTER, key, key, eventAction == AMOTION_EVENT_ACTION_DOWN ? Action::PUSHED : Action::RELEASED, 0, 0, Source::KEYBOARD, time, nullptr});
					}
					return true;
				}
				case AINPUT_SOURCE_CLASS_JOYSTICK:
				{
					auto dev = deviceForInputId(win.appContext(), sysInputDev, AInputEvent_getDeviceId(event));
					if(!dev) [[unlikely]]
					{
						if(Config::DEBUG_BUILD)
							logWarn("discarding joystick input from unknown device ID: %d", AInputEvent_getDeviceId(event));
						return false;
					}
					auto enumID = dev->enumId();
					//logMsg("Joystick input from %s", dev->name());
					auto time = makeTimeFromMotionEvent(event);
					if(hasGetAxisValue())
					{
						for(auto &axis : dev->jsAxes())
						{
							auto pos = AMotionEvent_getAxisValue(event, axis.id, 0);
							//logMsg("axis %d with value: %f", axis.id, (double)pos);
							axis.keyEmu.dispatch(pos, enumID, Map::SYSTEM, time, *dev, win);
						}
					}
					else
					{
						// no getAxisValue, can only use 2 axis values (X and Y)
						iterateTimes(std::min((uint32_t)dev->jsAxes().size(), 2u), i)
						{
							auto pos = i ? AMotionEvent_getY(event, 0) : AMotionEvent_getX(event, 0);
							dev->jsAxes()[i].keyEmu.dispatch(pos, enumID, Map::SYSTEM, time, *dev, win);
						}
					}
					return true;
				}
				default:
				{
					//logWarn("from other source:%s, %dx%d", aInputSourceToStr(source), (int)AMotionEvent_getX(event, 0), (int)AMotionEvent_getY(event, 0));
					return false;
				}
			}
		}
		bcase AINPUT_EVENT_TYPE_KEY:
		{
			auto keyCode = AKeyEvent_getKeyCode(event);
			auto devID = AInputEvent_getDeviceId(event);
			auto repeatCount = AKeyEvent_getRepeatCount(event);
			auto source = AInputEvent_getSource(event);
			auto eventSource = isFromSource(source, AINPUT_SOURCE_GAMEPAD) ? Source::GAMEPAD : Source::KEYBOARD;
			if(Config::DEBUG_BUILD)
			{
				//logMsg("key event, code:%d id:%d repeat:%d action:%s source:%s", keyCode, devID, repeatCount,
				//	keyEventActionStr(AKeyEvent_getAction(event)), sourceStr(eventSource));
			}
			auto keyWasReallyRepeated =
				[](int devID, int mostRecentKeyEventDevID, int repeatCount)
				{
					// On Android 3.1+, 2 or more devices pushing the same
					// button may be considered a repeat event by the OS.
					// Filter out this case by checking that the previous
					// event came from the same device ID if it has
					// a repeat count.
					return repeatCount != 0 && devID == mostRecentKeyEventDevID;
				};
			if(!keyWasReallyRepeated(devID, mostRecentKeyEventDevID, repeatCount))
			{
				if(repeatCount)
				{
					//logDMsg("ignoring repeat count:%d from device:%d", repeatCount, devID);
				}
				repeatCount = 0;
			}
			mostRecentKeyEventDevID = devID;
			const AndroidInputDevice *dev = deviceForInputId(win.appContext(), sysInputDev, devID);
			if(!dev) [[unlikely]]
			{
				if(virtualDev)
				{
					//logWarn("re-mapping key event unknown device ID %d to Virtual", devID);
					dev = virtualDev;
				}
				else
				{
					logWarn("key event from unknown device ID:%d", devID);
					return false;
				}
			}
			auto metaState = AKeyEvent_getMetaState(event);
			mapKeycodesForSpecialDevices(*dev, keyCode, metaState, eventSource, event);
			if(!keyCode) [[unlikely]] // ignore "unknown" key codes
			{
				return false;
			}
			uint32_t shiftState = metaState & AMETA_SHIFT_ON;
			auto time = makeTimeFromKeyEvent(event);
			assert((uint32_t)keyCode < Keycode::COUNT);
			auto action = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? Action::RELEASED : Action::PUSHED;
			if(!dev->iCadeMode() || (dev->iCadeMode() && !processICadeKey(keyCode, action, time, *dev, win)))
			{
				cancelKeyRepeatTimer();
				Key key = keyCode & 0x1ff;
				return dispatchKeyInputEvent({dev->enumId(), Map::SYSTEM, key, key, action, shiftState, repeatCount, eventSource, time, dev}, win);
			}
			return true;
		}
	}
	logWarn("unhandled input event type %d", type);
	return false;
}

void AndroidApplication::processInputCommon(AInputQueue *inputQueue, AInputEvent* event)
{
	//logMsg("input event start");
	if(!deviceWindow()) [[unlikely]]
	{
		logMsg("ignoring input with uninitialized window");
		AInputQueue_finishEvent(inputQueue, event, 0);
		return;
	}
	if(eventsUseOSInputMethod() && AInputQueue_preDispatchEvent(inputQueue, event))
	{
		//logMsg("input event used by pre-dispatch");
		return;
	}
	auto handled = processInputEvent(event, *deviceWindow());
	AInputQueue_finishEvent(inputQueue, event, handled);
	//logMsg("input event end: %s", handled ? "handled" : "not handled");
}

void AndroidApplication::processInput(AInputQueue *queue)
{
	(this->*processInput_)(queue);
}

// Use on Android 4.1+ to fix a possible ANR where the OS
// claims we haven't processed all input events even though we have.
// This only seems to happen under heavy input event load, like
// when using multiple joysticks. Everything seems to work
// properly if we keep calling AInputQueue_getEvent until
// it returns an error instead of using AInputQueue_hasEvents
// and no warnings are printed to logcat unlike earlier
// Android versions
void AndroidApplication::processInputWithGetEvent(AInputQueue *inputQueue)
{
	int events = 0;
	AInputEvent* event = nullptr;
	while(AInputQueue_getEvent(inputQueue, &event) >= 0)
	{
		processInputCommon(inputQueue, event);
		events++;
	}
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
}

void AndroidApplication::processInputWithHasEvents(AInputQueue *inputQueue)
{
	int events = 0;
	int32_t hasEventsRet = 0;
	// Note: never call AInputQueue_hasEvents on first iteration since it may return 0 even if
	// events are present if they were pre-dispatched, leading to an endless stream of callbacks
	do
	{
		AInputEvent* event = nullptr;
		if(AInputQueue_getEvent(inputQueue, &event) < 0)
		{
			//logWarn("error getting input event from queue");
			break;
		}
		processInputCommon(inputQueue, event);
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

static const char* aInputSourceToStr(uint32_t source)
{
	switch(source)
	{
		case AINPUT_SOURCE_UNKNOWN: return "Unknown";
		case AINPUT_SOURCE_KEYBOARD: return "Keyboard";
		case AINPUT_SOURCE_DPAD: return "DPad";
		case AINPUT_SOURCE_TOUCHSCREEN: return "Touchscreen";
		case AINPUT_SOURCE_MOUSE: return "Mouse";
		case AINPUT_SOURCE_TRACKBALL: return "Trackball";
		case AINPUT_SOURCE_TOUCHPAD: return "Touchpad";
		case AINPUT_SOURCE_JOYSTICK: return "Joystick";
		case AINPUT_SOURCE_ANY: return "Any";
		default:  return "Unhandled value";
	}
}

void ApplicationContext::flushSystemInputEvents()
{
	application().flushSystemInputEvents();
}

void AndroidApplication::flushSystemInputEvents()
{
	auto queue = inputQueue;
	if(AInputQueue_hasEvents(queue))
	{
		processInput(queue);
	}
}

}
