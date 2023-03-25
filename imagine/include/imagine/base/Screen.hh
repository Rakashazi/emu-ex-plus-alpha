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
#elif defined __ANDROID__
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
#include <imagine/util/Point2D.hh>
#include <vector>

namespace IG
{

class Screen : public ScreenImpl
{
public:
	Screen(ApplicationContext, InitParams);
	int width() const;
	int height() const;
	IP sizePx() const { return {width(), height()}; }
	bool isPosted() const;
	bool addOnFrame(OnFrameDelegate, int priority = 0);
	bool removeOnFrame(OnFrameDelegate);
	bool containsOnFrame(OnFrameDelegate) const;
	size_t onFrameDelegates() const;
	FrameParams makeFrameParams(FrameTime timestamp) const;
	bool frameRateIsReliable() const;
	FrameRate frameRate() const;
	FloatSeconds frameTime() const;
	void setFrameRate(FrameRate);
	void setFrameTime(FloatSeconds);
	std::vector<FrameRate> supportedFrameRates() const;
	void setFrameInterval(int interval);
	static bool supportsFrameInterval();
	bool supportsTimestamps() const;
	bool frameUpdate(FrameTime timestamp);
	void setActive(bool active);
	ApplicationContext appContext() const { return appCtx; }
	Application &application() const { return appContext().application(); }

private:
	DelegateFuncSet<OnFrameDelegate> onFrameDelegate{};
	const WindowContainer *windowsPtr{};
	ApplicationContext appCtx;
	bool framePosted{};
	bool isActive{true};

	void runOnFrameDelegates(FrameTime timestamp);
	void postFrame();
	void unpostFrame();
	void postFrameTimer();
	void unpostFrameTimer();
};

}
