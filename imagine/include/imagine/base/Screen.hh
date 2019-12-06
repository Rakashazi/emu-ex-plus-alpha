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

#include <vector>
#include <imagine/config/defs.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFuncSet.hh>

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

	using ChangeDelegate = DelegateFunc<void (Screen &screen, Change change)>;
	using OnFrameDelegate = DelegateFunc<bool (FrameParams params)>;

	struct FrameParams
	{
		Screen &screen_;
		FrameTimeBase timestamp_;

		Screen &screen() const { return screen_; }
		FrameTimeBase timestamp() const { return timestamp_; }
		FrameTimeBaseDiff timestampDiff() const
		{
			auto lastTimestamp = screen_.lastFrameTimestamp();
			return lastTimestamp ? timestamp_ - lastTimestamp : 0;
		}
		uint elapsedFrames() const { return screen_.elapsedFrames(timestamp_); }
	};

  static constexpr double DISPLAY_RATE_DEFAULT = 0;

	constexpr Screen() {}
	static uint screens();
	static Screen *screen(uint idx);
	// Called when a screen addition/removal/change occurs
	static void setOnChange(ChangeDelegate del);
	int width();
	int height();
	bool isPosted();
	static bool screensArePosted();
	bool addOnFrame(OnFrameDelegate del, int priority = 0);
	bool removeOnFrame(OnFrameDelegate del);
	bool containsOnFrame(OnFrameDelegate del);
	uint onFrameDelegates();
	bool runningOnFrameDelegates();
	FrameTimeBase lastFrameTimestamp() const { return prevFrameTimestamp; }
	uint elapsedFrames(FrameTimeBase frameTime);
	bool frameRateIsReliable() const;
	double frameRate() const;
	double frameTime() const;
	void setFrameRate(double rate);
	std::vector<double> supportedFrameRates();
	void setFrameInterval(uint interval);
	static bool supportsFrameInterval();
	static bool supportsTimestamps();

	// for internal use
	FrameTimeBase prevFrameTimestamp{};
	static ChangeDelegate onChange;

	static void addScreen(Screen *s);
	void frameUpdate(FrameTimeBase timestamp);
	void startDebugFrameStats(FrameTimeBase timestamp);
	void endDebugFrameStats();
	void setActive(bool active);
	static void setActiveAll(bool active);
	void deinit();

private:
  FrameTimeBase timePerFrame{};
	bool framePosted = false;
	bool inFrameHandler = false;
	bool isActive = true;
	#ifndef NDEBUG
	// for debug frame stats
	uint continuousFrames{};
	#endif
	DelegateFuncSet<OnFrameDelegate> onFrameDelegate{};

	void runOnFrameDelegates(FrameTimeBase timestamp);
	void postFrame();
	void unpostFrame();
};

}
