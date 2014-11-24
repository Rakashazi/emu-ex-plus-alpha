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

namespace Config
{
#if (defined __ANDROID__ && !(defined __arm__ && __ARM_ARCH < 7)) || defined CONFIG_BASE_IOS
#define CONFIG_BASE_MULTI_SCREEN
#define CONFIG_BASE_SCREEN_HOTPLUG
static constexpr bool BASE_MULTI_SCREEN = true;
#else
static constexpr bool BASE_MULTI_SCREEN = false;
#endif
}

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
	struct FrameParams;

	using ChangeDelegate = DelegateFunc<void (const Screen &screen, Change change)>;
	using OnFrameDelegate = DelegateFunc<void (Screen &screen, FrameParams params)>;

	struct FrameParams
	{
		FrameTimeBase frameTime_;
		OnFrameDelegate onFrame_;

		FrameTimeBase frameTime() const { return frameTime_; }
		OnFrameDelegate thisOnFrame() const { return onFrame_; }
	};

  static const uint REFRESH_RATE_DEFAULT = 0;

	constexpr Screen() {}
	static uint screens();
	static Screen *screen(uint idx);
	// Called when a screen addition/removal/change occurs
	static void setOnChange(ChangeDelegate del);
	int width();
	int height();
	void postFrame();
	void unpostFrame();
	static void unpostAll();
	bool isPosted();
	static bool screensArePosted();
	void addOnFrame(OnFrameDelegate del);
	bool addOnFrameOnce(OnFrameDelegate del);
	void postOnFrame(OnFrameDelegate del);
	bool postOnFrameOnce(OnFrameDelegate del);
	bool removeOnFrame(OnFrameDelegate del);
	bool containsOnFrame(OnFrameDelegate del);
	uint onFrameDelegates();
	FrameTimeBase lastPostedFrameTime() const { return prevFrameTime; }
	uint elapsedFrames(FrameTimeBase frameTime);
  uint refreshRate();
  void setRefreshRate(uint rate);
	void setFrameInterval(uint interval);
	static bool supportsFrameInterval();

	// for internal use
	FrameTimeBase prevFrameTime{};
	static ChangeDelegate onChange;

	static void addScreen(Screen *s);
	void frameUpdate(FrameTimeBase frameTime);
	void startDebugFrameStats(FrameTimeBase frameTime);
	void endDebugFrameStats();
	void deinit();

private:
  FrameTimeBase timePerFrame{};
	bool framePosted = false;
	bool inFrameHandler = false;
	#ifndef NDEBUG
	// for debug frame stats
	uint continuousFrames{};
	#endif
	StaticArrayList<OnFrameDelegate, 8> onFrameDelegate;

	void runOnFrameDelegates(FrameTimeBase frameTime);
};

}
