#pragma once

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

#include <imagine/engine-globals.h>
#include <imagine/base/baseDefs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/DelegateFunc.hh>

#if (defined __ANDROID__ && !(defined __arm__ && __ARM_ARCH < 7)) || defined CONFIG_BASE_IOS
#define CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_SCREEN_HOTPLUG
#endif

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XScreen.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidScreen.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSScreen.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaScreen.hh>
#endif

namespace Base
{
using namespace IG;

class Screen : public ScreenImpl
{
public:
	struct Change
	{
		uint state;
		enum { ADDED, REMOVED };

		constexpr Change(uint state): state(state) {}
		bool added() const { return state == ADDED; }
		bool removed() const { return state == REMOVED; }
	};
	using ChangeDelegate = DelegateFunc<void (const Screen &screen, const Change &change)>;

  static const uint REFRESH_RATE_DEFAULT = 0;
  FrameTimeBase prevFrameTime = 0;
	using OnFrameDelegate = DelegateFunc<void (Screen &screen, FrameTimeBase frameTime)>;
	StaticArrayList<OnFrameDelegate, 4> onFrameDelegate;
	bool framePosted = false;
	bool inFrameHandler = false;
	static ChangeDelegate onChange;

	constexpr Screen() {}
	void postFrame();
	void unpostFrame();
	static void unpostAll();
	bool frameIsPosted();
	static bool screensArePosted();
	void addOnFrameDelegate(OnFrameDelegate del);
	bool removeOnFrameDelegate(OnFrameDelegate del);
	bool containsOnFrameDelegate(OnFrameDelegate del);
	void clearOnFrameDelegates();
	void runOnFrameDelegates(FrameTimeBase frameTime);
	FrameTimeBase lastPostedFrameTime() { return prevFrameTime; }
  uint refreshRate();
  #ifdef CONFIG_BASE_X11
  void setRefreshRate(uint rate);
  #else
  void setRefreshRate(uint rate) {}
  #endif
  bool frameUpdate(FrameTimeBase frameTime, bool forceDraw);
  bool frameUpdate(FrameTimeBase frameTime)
  {
  	return frameUpdate(frameTime, false);
  }
	static uint screens();
	static Screen *screen(uint idx);

	static void addScreen(Screen *s);
	void swapsComplete();
	void deinit();

	// Called when a screen addition/removal/change occurs
	static void setOnChange(ChangeDelegate del);
};

}
