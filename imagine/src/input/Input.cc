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

#include <imagine/input/Input.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Timer.hh>
#include <imagine/logger/logger.h>

namespace Input
{

static Base::Timer keyRepeatTimer;
static Event keyRepeatEvent;
static bool allowKeyRepeats_ = true;

void setAllowKeyRepeats(bool on)
{
	allowKeyRepeats_ = on;
	if(!on)
	{
		deinitKeyRepeatTimer();
	}
}

bool allowKeyRepeats()
{
	return allowKeyRepeats_;
}

void startKeyRepeatTimer(const Event &event)
{
	if(!allowKeyRepeats_)
		return;
	if(!event.pushed())
	{
		// only repeat PUSHED action, otherwise cancel the timer
		//logMsg("repeat event is not for pushed action");
		cancelKeyRepeatTimer();
		return;
	}
	//logMsg("starting key repeat");
	keyRepeatEvent = event;
	keyRepeatTimer.callbackAfterMSec(
		[]()
		{
			logMsg("repeating key event");
			if(likely(keyRepeatEvent.pushed()))
				Base::onInputEvent(Base::mainWindow(), keyRepeatEvent);
		}, 400, 50, Base::Timer::HINT_REUSE);
}

void cancelKeyRepeatTimer()
{
	//logMsg("cancelled key repeat");
	keyRepeatTimer.cancel();
	keyRepeatEvent = {};
}

void deinitKeyRepeatTimer()
{
	//logMsg("deinit key repeat");
	keyRepeatTimer.deinit();
	keyRepeatEvent = {};
}

StaticArrayList<Device*, MAX_DEVS> devList;

void addDevice(Device &d)
{
	d.idx = devList.size();
	devList.push_back(&d);
}

void removeDevice(Device &d)
{
	logMsg("removing device: %s,%d", d.name(), d.enumId());
	cancelKeyRepeatTimer();
	devList.remove(&d);
	indexDevices();
}

void indexDevices()
{
	// re-number device indices
	uint i = 0;
	for(auto &e : Input::devList)
	{
		e->idx = i;
		i++;
	}
}

bool swappedGamepadConfirm = SWAPPED_GAMEPAD_CONFIRM_DEFAULT;

static uint xPointerTransform_ = POINTER_NORMAL;
void xPointerTransform(uint mode)
{
	xPointerTransform_ = mode;
}

static uint yPointerTransform_ = POINTER_NORMAL;
void yPointerTransform(uint mode)
{
	yPointerTransform_ = mode;
}

static uint pointerAxis_ = POINTER_NORMAL;
void pointerAxis(uint mode)
{
	pointerAxis_ = mode;
}

IG::Point2D<int> transformInputPos(const Base::Window &win, IG::Point2D<int> srcPos)
{
	IG::Point2D<int> pos;
	// x,y axis is swapped first
	pos.x = pointerAxis_ == POINTER_INVERT ? srcPos.y : srcPos.x;
	pos.y = pointerAxis_ == POINTER_INVERT ? srcPos.x : srcPos.y;

	// then coordinates are inverted
	if(xPointerTransform_ == POINTER_INVERT)
		pos.x = win.width() - pos.x;
	if(yPointerTransform_ == POINTER_INVERT)
		pos.y = win.height() - pos.y;
	return pos;
}

// For soft-orientation
void configureInputForOrientation(const Base::Window &win)
{
	using namespace Input;
	using namespace Base;
	xPointerTransform(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_90 ? POINTER_NORMAL : POINTER_INVERT);
	yPointerTransform(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_270 ? POINTER_NORMAL : POINTER_INVERT);
	pointerAxis(win.rotateView == VIEW_ROTATE_0 || win.rotateView == VIEW_ROTATE_180 ? POINTER_NORMAL : POINTER_INVERT);
}

bool keyInputIsPresent()
{
	return Device::anyTypeBitsPresent(Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_GAMEPAD);
}

void dispatchInputEvent(const Input::Event &event)
{
	onInputEvent(Base::mainWindow(), event);
}

bool processICadeKey(char c, uint action, const Device &dev, Base::Window &win)
{
	static const char *ON_STATES  = "wdxayhujikol";
	static const char *OFF_STATES = "eczqtrfnmpgv";

	#ifndef CONFIG_BASE_IOS
	using namespace ICade;
	static const Key keycodeMap[14] =
	{
		UP, RIGHT, DOWN, LEFT,
		A, B, C, D, E, F, G, H
	};
	#endif

	if(!c)
		return false; // ignore null character

	const char *p = strchr(ON_STATES, c);
	if(p)
	{
		//logMsg("handling iCade on-state key %c", *p);
		int index = p-ON_STATES;
		if(action == PUSHED)
		{
			#ifdef CONFIG_BASE_IOS
			Event event{0, Event::MAP_ICADE, (Key)(index+1), PUSHED, 0, 0, &dev};
			#else
			Event event{0, Event::MAP_ICADE, (Key)keycodeMap[index], PUSHED, 0, 0, &dev};
			#endif
			startKeyRepeatTimer(event);
			Base::onInputEvent(win, event);
		}
		return true;
	}
	else
	{
		p = strchr(OFF_STATES, c);
		if(p)
		{
			//logMsg("handling iCade off-state key %c", *p);
			int index = p-OFF_STATES;
			if(action == PUSHED)
			{
				cancelKeyRepeatTimer();
				#ifdef CONFIG_BASE_IOS
				Base::onInputEvent(win, Input::Event{0, Event::MAP_ICADE, (Key)(index+1), RELEASED, 0, 0, &dev});
				#else
				Base::onInputEvent(win, Input::Event{0, Event::MAP_ICADE, keycodeMap[index], RELEASED, 0, 0, &dev});
				#endif
			}
			return true;
		}
	}
	return false; // not an iCade key
}

}
