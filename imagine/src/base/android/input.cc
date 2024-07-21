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
#include <imagine/util/ranges.hh>
#include <imagine/util/algorithm.h>
#include <imagine/base/android/AndroidInputDevice.hh>
#include <android/input.h>

namespace IG
{

extern int32_t (*AMotionEvent_getActionButton_)(const AInputEvent* motion_event);

static const char* aInputSourceToStr(uint32_t source);

Input::Device* AndroidApplication::inputDeviceForId(int id) const
{
	return findPtr(inputDev, [=](const auto &e) { return e->map() == Input::Map::SYSTEM && e->id() == id; });
}

std::pair<Input::Device*, int> AndroidApplication::inputDeviceForEvent(AInputEvent *event)
{
	if(hasMultipleInputDeviceSupport())
	{
		int id = AInputEvent_getDeviceId(event);
		return {inputDeviceForId(id), id};
	}
	else
	{
		return {builtinKeyboardDev, -1};
	}
}

static auto makeTimeFromMotionEvent(AInputEvent *event)
{
	return SteadyClockTimePoint{Nanoseconds{AMotionEvent_getEventTime(event)}};
}

static auto makeTimeFromKeyEvent(AInputEvent *event)
{
	return SteadyClockTimePoint{Nanoseconds{AKeyEvent_getEventTime(event)}};
}

static std::pair<int32_t, Input::Source> mapKeycodesForSpecialDevices(const Input::Device &dev,
	int32_t keyCode, int32_t metaState, Input::Source src)
{
	using namespace IG::Input;
	switch(dev.subtype())
	{
		case DeviceSubtype::XPERIA_PLAY:
		{
			if(!Config::MACHINE_IS_GENERIC_ARMV7)
				break;
			switch(keyCode)
			{
				case Keycode::BACK:
					if(metaState & AMETA_ALT_ON)
						return {Keycode::GAME_B, Source::GAMEPAD};
					else
						break;
				case Keycode::GAME_A ... Keycode::GAME_SELECT:
					return {keyCode, Source::GAMEPAD};
			}
			break;
		}
		default:
			switch(keyCode) // map volume/media keys to keyboard source
			{
				case Keycode::VOL_UP ... Keycode::VOL_DOWN:
				case Keycode::MEDIA_PLAY_PAUSE ... Keycode::MUTE:
					return {keyCode, Source::KEYBOARD};
			}
			break;
	}
	return {keyCode, src};
}

constexpr const char *keyEventActionStr(uint32_t action)
{
	switch(action)
	{
		default: return "Unknown";
		case AKEY_EVENT_ACTION_DOWN: return "Down";
		case AKEY_EVENT_ACTION_UP: return "Up";
		case AKEY_EVENT_ACTION_MULTIPLE: return "Multiple";
	}
}

// Implementation of missing NDK function equivalent of MotionEvent.getActionButton()
// by accessing mActionButton data directly using known offsets,
// or by calling user-set function pointer when SDK >= 33
static int32_t AMotionEvent_getActionButtonCompat(const AInputEvent* event, int32_t sdkVersion)
{
	if(sdkVersion >= 33)
	{
		return AMotionEvent_getActionButton_(event);
	}
	static const bool ptrIs64Bits = sizeof(void*) == 8;
	auto asIntPtr = (const int32_t *)event;
	switch(sdkVersion)
	{
		case 23 ... 28: return asIntPtr[ptrIs64Bits ? 5  : 4];
		case 29:        return asIntPtr[ptrIs64Bits ? 6  : 5];
		case 30 ... 32: return asIntPtr[ptrIs64Bits ? 15 : 14];
	}
	return AMOTION_EVENT_BUTTON_PRIMARY; // can't determine button, fall back to primary
}

static bool isFromSource(int src, int srcTest)
{
	return (src & srcTest) == srcTest;
}

static Input::Action touchEventAction(uint32_t e)
{
	using namespace IG::Input;
	switch(e)
	{
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
		case AMOTION_EVENT_ACTION_DOWN:   return Action::PUSHED;
		case AMOTION_EVENT_ACTION_POINTER_UP:
		case AMOTION_EVENT_ACTION_UP:     return Action::RELEASED;
		case AMOTION_EVENT_ACTION_CANCEL: return Action::CANCELED;
		case AMOTION_EVENT_ACTION_MOVE:   return Action::MOVED;
		default:
			logWarn("unknown touch motion event action:%u", e);
			return Action::MOVED;
	}
}

static std::pair<Input::Action, Input::Key> mouseEventAction(uint32_t e, AInputEvent* event, int sdk)
{
	using namespace IG::Input;
	switch(e)
	{
		case AMOTION_EVENT_ACTION_BUTTON_PRESS:
			if(auto actionBtn = AMotionEvent_getActionButtonCompat(event, sdk);
				actionBtn == AMotionEvent_getButtonState(event)) // first button press already processed in ACTION_DOWN
				return {Action::MOVED, 0};
			else
				return {Action::PUSHED, actionBtn};
		case AMOTION_EVENT_ACTION_DOWN:           return {Action::PUSHED, AMotionEvent_getButtonState(event)};
		case AMOTION_EVENT_ACTION_BUTTON_RELEASE: return {Action::RELEASED, AMotionEvent_getActionButtonCompat(event, sdk)};
		case AMOTION_EVENT_ACTION_UP:             return {Action::RELEASED, Pointer::ALL_BUTTONS};
		case AMOTION_EVENT_ACTION_CANCEL:         return {Action::CANCELED, 0};
		case AMOTION_EVENT_ACTION_HOVER_ENTER:    return {Action::ENTER_VIEW, 0};
		case AMOTION_EVENT_ACTION_HOVER_EXIT:     return {Action::EXIT_VIEW, 0};
		case AMOTION_EVENT_ACTION_HOVER_MOVE:
		case AMOTION_EVENT_ACTION_MOVE:           return {Action::MOVED, 0};
		case AMOTION_EVENT_ACTION_SCROLL:
			if(auto vScroll = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, 0);
				vScroll > 0.f)
				return {Action::SCROLL_UP, 0};
			else if(vScroll < 0.f)
				return {Action::SCROLL_DOWN, 0};
			return {Action::MOVED, 0};
		default:
			logWarn("unknown mouse motion event action:%u", e);
			return {Action::MOVED, 0};
	}
}

bool AndroidApplication::processInputEvent(AInputEvent* event, Input::Device *devPtr, int devId, Window &win)
{
	using namespace IG::Input;
	auto type = AInputEvent_getType(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			auto source = AInputEvent_getSource(event);
			auto actionBits = AMotionEvent_getAction(event);
			auto actionCode = actionBits & AMOTION_EVENT_ACTION_MASK;
			switch(source & AINPUT_SOURCE_CLASS_MASK)
			{
				case AINPUT_SOURCE_CLASS_POINTER:
				{
					if(!devPtr) [[unlikely]]
					{
						logWarn("pointer motion event from unknown device ID: %d", devId);
						return false;
					}
					auto src = isFromSource(source, AINPUT_SOURCE_MOUSE) ? Input::Source::MOUSE : Input::Source::TOUCHSCREEN;
					size_t actionPIdx = actionBits >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					auto pointers = AMotionEvent_getPointerCount(event);
					uint32_t metaState = AMotionEvent_getMetaState(event);
					assumeExpr(pointers >= 1);
					if(src == Input::Source::TOUCHSCREEN)
					{
						bool handled = false;
						auto action = touchEventAction(actionCode);
						//logMsg("touch motion event: id:%d (%s) action:%s pointers:%d:%d",
						//	devId, devPtr->name().data(), actionStr(action), (int)pointers, actionPIdx);
						for(auto i : iotaCount(pointers))
						{
							auto pAction = action;
							// a pointer not performing the action just needs its position updated
							if(actionPIdx != i)
							{
								//logMsg("non-action pointer idx %d", i);
								pAction = Input::Action::MOVED;
							}
							auto pos = win.transformInputPos({AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)});
							auto pId = AMotionEvent_getPointerId(event, i);
							handled |= win.dispatchInputEvent(MotionEvent{Input::Map::POINTER, Input::Pointer::LBUTTON,
								metaState, pAction, pos.x, pos.y, pId, src, makeTimeFromMotionEvent(event), devPtr});
						}
						return handled;
					}
					else // mouse
					{
						auto [action, btnState] = mouseEventAction(actionCode, event, win.appContext().androidSDK());
						//logMsg("mouse motion event: id:%d (%s) action:%s buttons:%d", devId, devPtr->name().data(), actionStr(action), btnState);
						auto pos = win.transformInputPos({AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0)});
						auto pId = AMotionEvent_getPointerId(event, 0);
						return win.dispatchInputEvent(MotionEvent{Input::Map::POINTER, btnState,
							metaState, action, pos.x, pos.y, pId, src, makeTimeFromMotionEvent(event), devPtr});
					}
				}
				case AINPUT_SOURCE_CLASS_NAVIGATION:
				{
					//logMsg("from trackball");
					auto x = AMotionEvent_getX(event, 0);
					auto y = AMotionEvent_getY(event, 0);
					auto time = makeTimeFromMotionEvent(event);
					float iX = x * 1000.f, iY = y * 1000.f;
					auto pos = win.transformInputPos({iX, iY});
					//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);
					if(actionCode == AMOTION_EVENT_ACTION_MOVE)
						win.dispatchInputEvent(MotionEvent{Map::REL_POINTER, 0, 0, Action::MOVED_RELATIVE, pos.x, pos.y, 0, Source::NAVIGATION, time, nullptr});
					else
					{
						Key key = Keycode::ENTER;
						win.dispatchInputEvent(KeyEvent{Map::REL_POINTER, key, actionCode == AMOTION_EVENT_ACTION_DOWN ? Action::PUSHED : Action::RELEASED, 0, 0, Source::KEYBOARD, time, nullptr});
					}
					return true;
				}
				case AINPUT_SOURCE_CLASS_JOYSTICK:
				{
					if(!devPtr) [[unlikely]]
					{
						logWarn("joystick motion event from unknown device ID:%d", devId);
						return false;
					}
					//logMsg("joystick motion event: id:%d (%s)", devId, devPtr->name().data());
					auto time = makeTimeFromMotionEvent(event);
					auto &axes = getAs<AndroidInputDevice>(*devPtr).jsAxes();
					if(hasGetAxisValue())
					{
						for(auto &axis : axes)
						{
							auto pos = AMotionEvent_getAxisValue(event, (int32_t)axis.id(), 0);
							//logMsg("axis %d with value: %f", axis.id, (double)pos);
							axis.dispatchInputEvent(pos, Input::Map::SYSTEM, time, *devPtr, win);
						}
					}
					else
					{
						// no getAxisValue, can only use 2 axis values (X and Y)
						for(auto i : iotaCount(std::min(axes.size(), 2uz)))
						{
							auto pos = i ? AMotionEvent_getY(event, 0) : AMotionEvent_getX(event, 0);
							axes[i].dispatchInputEvent(pos, Input::Map::SYSTEM, time, *devPtr, win);
						}
					}
					return true;
				}
				default:
				{
					if(Config::DEBUG_BUILD)
						logWarn("motion event from other source:%s, %dx%d", aInputSourceToStr(source), (int)AMotionEvent_getX(event, 0), (int)AMotionEvent_getY(event, 0));
					return false;
				}
			}
		}
		case AINPUT_EVENT_TYPE_KEY:
		{
			auto keyCode = AKeyEvent_getKeyCode(event);
			auto repeatCount = AKeyEvent_getRepeatCount(event);
			auto source = AInputEvent_getSource(event);
			auto eventSource = isFromSource(source, AINPUT_SOURCE_GAMEPAD) ? Source::GAMEPAD : Source::KEYBOARD;
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
			if(!keyWasReallyRepeated(devId, mostRecentKeyEventDevID, repeatCount))
			{
				if(repeatCount)
				{
					//logDMsg("ignoring repeat count:%d from device:%d", repeatCount, devID);
				}
				repeatCount = 0;
			}
			mostRecentKeyEventDevID = devId;
			if(!devPtr) [[unlikely]]
			{
				if(virtualDev)
				{
					//logWarn("re-mapping key event unknown device ID %d to Virtual", devID);
					devPtr = virtualDev;
				}
				else
				{
					logWarn("key event from unknown device ID:%d", devId);
					return false;
				}
			}
			if(Config::DEBUG_BUILD)
			{
				//logMsg("key event: code:%d id:%d (%s) repeat:%d action:%s source:%s", keyCode, devId, devPtr->name().data(),
				//	repeatCount, keyEventActionStr(AKeyEvent_getAction(event)), sourceStr(eventSource));
			}
			auto metaState = AKeyEvent_getMetaState(event);
			auto [mappedKeyCode, mappedSource] = mapKeycodesForSpecialDevices(*devPtr, keyCode, metaState, eventSource);
			keyCode = mappedKeyCode;
			eventSource = mappedSource;
			if(!keyCode) [[unlikely]] // ignore "unknown" key codes
			{
				return false;
			}
			auto time = makeTimeFromKeyEvent(event);
			assert((uint32_t)keyCode < Keycode::COUNT);
			auto action = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? Action::RELEASED : Action::PUSHED;
			cancelKeyRepeatTimer();
			Key key = keyCode & 0x1ff;
			return dispatchKeyInputEvent({Map::SYSTEM, key, action, (uint32_t)metaState,
				repeatCount, eventSource, time, devPtr}, win);
		}
	}
	logWarn("unknown input event type:%d", type);
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
	auto [devPtr, devId] = inputDeviceForEvent(event);
	// Don't pre-dispatch physical device events since IMEs like SwiftKey
	// can intercept and replace them with virtual events
	if(devId == -1 && AInputQueue_preDispatchEvent(inputQueue, event))
	{
		//logMsg("event used by pre-dispatch");
		return;
	}
	auto handled = processInputEvent(event, devPtr, devId, *deviceWindow());
	AInputQueue_finishEvent(inputQueue, event, handled);
	//logMsg("input event end: %s", handled ? "handled" : "not handled");
}

void AndroidApplication::processInput(AInputQueue *queue)
{
	if constexpr(Config::ENV_ANDROID_MIN_SDK >= 12)
	{
		processInputWithGetEvent(queue);
	}
	else
	{
		(this->*processInput_)(queue);
	}
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
	int events{};
	AInputEvent* event{};
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
	int events{};
	int32_t hasEventsRet{};
	// Note: never call AInputQueue_hasEvents on first iteration since it may return 0 even if
	// events are present if they were pre-dispatched, leading to an endless stream of callbacks
	do
	{
		AInputEvent* event{};
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

void AndroidApplication::flushSystemInputEvents()
{
	auto queue = inputQueue;
	if(AInputQueue_hasEvents(queue))
	{
		processInput(queue);
	}
}

bool AndroidApplication::hasPendingInputQueueEvents() const
{
	return AInputQueue_hasEvents(inputQueue);
}

}
