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
#include <imagine/time/Time.hh>
#include <imagine/base/baseDefs.hh>
#include <imagine/base/SimpleFrameTimer.hh>
#include <imagine/base/linux/DRMFrameTimer.hh>
#include <imagine/base/linux/FBDevFrameTimer.hh>
#include <imagine/base/FrameTimerInterface.hh>
#include <utility>
#include <variant>

namespace IG
{

class ApplicationContext;

using FrameTimerVariant = std::variant<
	SimpleFrameTimer,
	#if CONFIG_PACKAGE_LIBDRM
	DRMFrameTimer,
	#endif
	FBDevFrameTimer>;

class FrameTimer : public FrameTimerInterface<FrameTimerVariant>
{
public:
	using FrameTimerInterface::FrameTimerInterface;
};

class XScreen
{
public:
	struct InitParams
	{
		void *xScreen;
	};

	XScreen(ApplicationContext, InitParams);
	std::pair<float, float> mmSize() const;
	void *nativeObject() const;
	bool operator ==(XScreen const &rhs) const;
	explicit operator bool() const;

	constexpr bool operator ==(ScreenId screen) const
	{
		return xScreen == screen;
	}

protected:
	void *xScreen{};
	FrameTimer frameTimer;
	SteadyClockTime frameTime_{};
	float xMM{}, yMM{};
	float frameRate_{};
	bool reliableFrameTime = true;
};

using ScreenImpl = XScreen;

}
