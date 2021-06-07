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

#include <imagine/config/defs.hh>

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XScreen.hh>
#elif defined CONFIG_BASE_ANDROID
#include <imagine/base/android/AndroidScreen.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSScreen.hh>
#elif defined CONFIG_BASE_MACOSX
#include <imagine/base/osx/CocoaScreen.hh>
#endif

#include <imagine/base/baseDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/DelegateFuncSet.hh>
#include <imagine/util/typeTraits.hh>
#include <vector>

namespace Base
{
using namespace IG;

class Screen : public ScreenImpl
{
public:
  static constexpr double DISPLAY_RATE_DEFAULT = 0;

	Screen(ApplicationContext, InitParams);
	int width() const;
	int height() const;
	bool isPosted() const;
	bool addOnFrame(OnFrameDelegate, int priority = 0);
	bool removeOnFrame(OnFrameDelegate);
	bool containsOnFrame(OnFrameDelegate) const;
	uint32_t onFrameDelegates() const;
	FrameParams makeFrameParams(FrameTime timestamp) const;
	bool frameRateIsReliable() const;
	double frameRate() const;
	FloatSeconds frameTime() const;
	void setFrameRate(double rate);
	std::vector<double> supportedFrameRates(ApplicationContext) const;
	void setFrameInterval(int interval);
	static bool supportsFrameInterval();
	bool supportsTimestamps() const;
	bool frameUpdate(FrameTime timestamp);
	void setActive(bool active);

private:
	DelegateFuncSet<OnFrameDelegate> onFrameDelegate{};
	ApplicationContext ctx;
	bool framePosted{};
	bool isActive{true};

	void runOnFrameDelegates(FrameTime timestamp);
	void postFrame();
	void unpostFrame();
	void postFrameTimer();
	void unpostFrameTimer();
};

}
