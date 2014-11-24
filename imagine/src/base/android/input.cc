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
#include <dlfcn.h>
#include <imagine/base/Base.hh>
#include <imagine/input/DragPointer.hh>
#include <imagine/logger/logger.h>
#include "internal.hh"
#include "android.hh"
#include "../../input/private.hh"
#include "AndroidInputDevice.hh"

namespace Input
{

void (*processInput)(AInputQueue *inputQueue) = processInputWithHasEvents;
static const int AINPUT_SOURCE_JOYSTICK = 0x01000010;

static struct TouchState
{
	constexpr TouchState() {}
	int id = -1;
	DragPointer dragState;
	bool isTouching = false;
} m[Config::Input::MAX_POINTERS];
static uint numCursors = sizeofArray(m);

#if CONFIG_ENV_ANDROID_MINSDK < 12
using AMotionEvent_getAxisValueProto = float (__NDK_FPABI__ *)(const AInputEvent* motion_event, int32_t axis, size_t pointer_index);
static AMotionEvent_getAxisValueProto AMotionEvent_getAxisValue{};
static bool hasGetAxisValue()
{
	return likely(AMotionEvent_getAxisValue);
}
#else
static bool hasGetAxisValue()
{
	return true;
}
#endif

static AndroidInputDevice *sysDeviceForInputId(int osId)
{
	for(auto &e : sysInputDev)
	{
		if(e->osId == osId)
		{
			return e;
		}
	}
	return nullptr;
}

static const Device *deviceForInputId(int osId)
{
	if(Base::androidSDK() < 12)
	{
		// no multi-input device support
		return &genericKeyDev;
	}
	for(auto &e : sysInputDev)
	{
		if(e->osId == osId)
		{
			return e;
		}
	}
	return nullptr;
}

static void mapKeycodesForSpecialDevices(const Device &dev, int32_t &keyCode, int32_t &metaState, AInputEvent *event)
{
	switch(dev.subtype())
	{
		bcase Device::SUBTYPE_XPERIA_PLAY:
		{
			if(Config::MACHINE_IS_GENERIC_ARMV7 && unlikely(keyCode == (int)Keycode::BACK && (metaState & AMETA_ALT_ON)))
			{
				keyCode = Keycode::GAME_B;
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

static void dispatchTouch(uint idx, uint action, TouchState &p, IG::Point2D<int> pos, Time time, bool isTouch)
{
	//logMsg("pointer: %d action: %s @ %d,%d", idx, eventActionToStr(action), pos.x, pos.y);
	p.dragState.pointerEvent(Pointer::LBUTTON, action, pos);
	Base::mainWindow().dispatchInputEvent(Event{idx, Event::MAP_POINTER, Pointer::LBUTTON, action, pos.x, pos.y, isTouch, time, nullptr});
}

static bool processTouchEvent(int action, int x, int y, int pid, Time time, bool isTouch)
{
	//logMsg("%s action: %s from id %d @ %d,%d @ time %f",
	//	isTouch ? "touch" : "mouse", pid, x, y, androidEventEnumToStr(action), (double)time);
	auto pos = transformInputPos(Base::mainWindow(), {x, y});
	switch(action)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			//logMsg("touch down for %d", pid);
			iterateTimes(sizeofArray(m), i) // find a free touch element
			{
				if(m[i].id == -1)
				{
					auto &p = m[i];
					p.id = pid;
					p.isTouching = true;
					dispatchTouch(i, PUSHED, p, pos, time, isTouch);
					break;
				}
			}
		bcase AMOTION_EVENT_ACTION_UP:
		//case AMOTION_EVENT_ACTION_CANCEL: // calling code always uses AMOTION_EVENT_ACTION_UP
			forEachInArray(m, p)
			{
				if(p->isTouching)
				{
					//logMsg("touch up for %d from gesture end", p_i);
					int x = p->dragState.x;
					int y = p->dragState.y;
					p->id = -1;
					p->isTouching = false;
					dispatchTouch(p_i, RELEASED, *p, {x, y}, time, isTouch);
				}
			}
		bcase AMOTION_EVENT_ACTION_POINTER_UP:
			//logMsg("touch up for %d", pid);
			iterateTimes(sizeofArray(m), i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					p.id = -1;
					p.isTouching = false;
					dispatchTouch(i, RELEASED, p, pos, time, isTouch);
					break;
				}
			}
		bdefault:
			// move event
			//logMsg("event id %d", action);
			iterateTimes(sizeofArray(m), i) // find the touch element
			{
				if(m[i].id == pid)
				{
					auto &p = m[i];
					dispatchTouch(i, MOVED, p, pos, time, isTouch);
					break;
				}
			}
	}

	/*logMsg("pointer state:");
	iterateTimes(sizeofArray(m), i)
	{
		if(m[i].id != -1)
			logMsg("id: %d x: %d y: %d inWin: %d", m[i].id, m[i].dragState.x, m[i].dragState.y, m[i].s.inWin);
	}*/

	return 1;
}

static bool processInputEvent(AInputEvent* event, Base::Window &win)
{
	auto type = AInputEvent_getType(event);
	auto source = AInputEvent_getSource(event);
	switch(type)
	{
		case AINPUT_EVENT_TYPE_MOTION:
		{
			int eventAction = AMotionEvent_getAction(event);
			//logMsg("get motion event action %d", eventAction);
			bool isTouch = false;
			switch(source)
			{
				case AINPUT_SOURCE_TRACKBALL:
				{
					//logMsg("from trackball");
					auto x = AMotionEvent_getX(event, 0);
					auto y = AMotionEvent_getY(event, 0);
					auto time = AMotionEvent_getEventTime(event);
					int iX = x * 1000., iY = y * 1000.;
					auto pos = transformInputPos(Base::mainWindow(), {iX, iY});
					//logMsg("trackball ev %s %f %f", androidEventEnumToStr(action), x, y);

					if(eventAction == AMOTION_EVENT_ACTION_MOVE)
						Base::mainWindow().dispatchInputEvent({0, Event::MAP_REL_POINTER, 0, MOVED_RELATIVE, pos.x, pos.y, false, time, nullptr});
					else
					{
						Key key = Keycode::ENTER;
						Base::mainWindow().dispatchInputEvent({0, Event::MAP_REL_POINTER, key, key, eventAction == AMOTION_EVENT_ACTION_DOWN ? PUSHED : RELEASED, 0, time, nullptr});
					}
					return 1;
				}
				case AINPUT_SOURCE_TOUCHPAD: // TODO
				{
					//logMsg("from touchpad");
					return 0;
				}
				case AINPUT_SOURCE_TOUCHSCREEN:
				{
					isTouch = true;
					// fall-through to case AINPUT_SOURCE_MOUSE
				}
				case AINPUT_SOURCE_MOUSE:
				{
					//logMsg("from touchscreen or mouse");
					uint action = eventAction & AMOTION_EVENT_ACTION_MASK;
					if(action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
					{
						// touch gesture ended
						processTouchEvent(AMOTION_EVENT_ACTION_UP,
								AMotionEvent_getX(event, 0),
								AMotionEvent_getY(event, 0),
								AMotionEvent_getPointerId(event, 0),
								AMotionEvent_getEventTime(event), isTouch);
						return 1;
					}
					uint actionPIdx = eventAction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
					int pointers = AMotionEvent_getPointerCount(event);
					iterateTimes(pointers, i)
					{
						int pAction = action;
						// a pointer not performing the action just needs its position updated
						if(actionPIdx != i)
						{
							//logMsg("non-action pointer idx %d", i);
							pAction = AMOTION_EVENT_ACTION_MOVE;
						}
						processTouchEvent(pAction,
							AMotionEvent_getX(event, i),
							AMotionEvent_getY(event, i),
							AMotionEvent_getPointerId(event, i),
							AMotionEvent_getEventTime(event), isTouch);
					}
					return 1;
				}
				case AINPUT_SOURCE_JOYSTICK: // Joystick
				{
					auto dev = sysDeviceForInputId(AInputEvent_getDeviceId(event));
					if(unlikely(!dev))
					{
						logWarn("discarding joystick input from unknown device ID: %d", AInputEvent_getDeviceId(event));
						return 0;
					}
					auto enumID = dev->enumId();
					//logMsg("Joystick input from %s", dev->name());

					if(hasGetAxisValue())
					{
						for(auto &axis : dev->axis)
						{
							auto pos = AMotionEvent_getAxisValue(event, axis.id, 0);
							//logMsg("axis %d with value: %f", axis.id, (double)pos);
							axis.keyEmu.dispatch(pos, enumID, Event::MAP_SYSTEM, *dev, win);
						}
					}
					else
					{
						// no getAxisValue, can only use 2 axis values (X and Y)
						iterateTimes(std::min(dev->axis.size(), (uint)2), i)
						{
							auto pos = i ? AMotionEvent_getY(event, 0) : AMotionEvent_getX(event, 0);
							dev->axis[i].keyEmu.dispatch(pos, enumID, Event::MAP_SYSTEM, *dev, win);
						}
					}
					return 1;
				}
				default:
				{
					//logWarn("from other source: %s, %dx%d", aInputSourceToStr(source), (int)AMotionEvent_getX(event, 0), (int)AMotionEvent_getY(event, 0));
					return 0;
				}
			}
		}
		bcase AINPUT_EVENT_TYPE_KEY:
		{
			auto keyCode = AKeyEvent_getKeyCode(event);
			//logMsg("key event, code: %d id: %d source: 0x%X repeat: %d action: %d", keyCode, AInputEvent_getDeviceId(event), source, AKeyEvent_getRepeatCount(event), AKeyEvent_getAction(event));
			if(!handleVolumeKeys &&
				(keyCode == (int)Keycode::VOL_UP || keyCode == (int)Keycode::VOL_DOWN))
			{
				return 0;
			}

			if(allowOSKeyRepeats || AKeyEvent_getRepeatCount(event) == 0)
			{
				auto dev = deviceForInputId(AInputEvent_getDeviceId(event));
				if(unlikely(!dev))
				{
					assert(virtualDev);
					//logWarn("re-mapping unknown device ID %d to Virtual", AInputEvent_getDeviceId(event));
					dev = virtualDev;
				}
				auto metaState = AKeyEvent_getMetaState(event);
				mapKeycodesForSpecialDevices(*dev, keyCode, metaState, event);
				if(unlikely(!keyCode)) // ignore "unknown" key codes
				{
					return 0;
				}
				//handleKeyEvent(keyCode, AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? 0 : 1, dev->enumId(), metaState & AMETA_SHIFT_ON, AKeyEvent_getEventTime(event), *dev);
				uint shiftState = metaState & AMETA_SHIFT_ON;
				auto time = AKeyEvent_getEventTime(event);
				assert((uint)keyCode < Keycode::COUNT);
				uint action = AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP ? RELEASED : PUSHED;
				#ifdef CONFIG_INPUT_ICADE
				if(!dev->iCadeMode() || (dev->iCadeMode() && !processICadeKey(keyCode, action, *dev, Base::mainWindow())))
				#endif
				{
					cancelKeyRepeatTimer();
					Key key = keyCode & 0x1ff;
					Base::mainWindow().dispatchInputEvent({dev->enumId(), Event::MAP_SYSTEM, key, key, action, shiftState, time, dev});
				}
			}
			return 1;
		}
	}
	logWarn("unhandled input event type %d", type);
	return 0;
}

static void processInputCommon(AInputQueue *inputQueue, AInputEvent* event)
{
	//logMsg("input event start");
	if(unlikely(!Base::deviceWindow()))
	{
		logMsg("ignoring input with uninitialized window");
		AInputQueue_finishEvent(inputQueue, event, 0);
		return;
	}
	if(unlikely(eventsUseOSInputMethod() && AInputQueue_preDispatchEvent(inputQueue, event)))
	{
		//logMsg("input event used by pre-dispatch");
		return;
	}
	auto handled = processInputEvent(event, *Base::deviceWindow());
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
		processInputCommon(inputQueue, event);
		events++;
	}
	if(events > 1)
	{
		//logMsg("processed %d input events", events);
	}
}

void processInputWithHasEvents(AInputQueue *inputQueue)
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

// dlsym extra functions from supplied libandroid.so
bool dlLoadAndroidFuncs(void *libandroid)
{
	#if CONFIG_ENV_ANDROID_MINSDK < 12
	if(Base::androidSDK() < 12)
	{
		return false;
	}
	// load AMotionEvent_getAxisValue dynamically
	if((AMotionEvent_getAxisValue = (AMotionEvent_getAxisValueProto)dlsym(libandroid, "AMotionEvent_getAxisValue")) == nullptr)
	{
		bug_exit("AMotionEvent_getAxisValue not found even though using SDK %d", Base::androidSDK());
		return false;
	}
	#endif
	return true;
}

static const char* aInputSourceToStr(uint source)
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

DragPointer *dragState(int p)
{
	return &m[p].dragState;
}

}
