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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/time/Time.hh>
#include <imagine/util/utility.h>
#include <imagine/util/algorithm.h>

namespace IG
{

constexpr SystemLogger log{"Screen"};

Screen::Screen(ApplicationContext ctx, InitParams params):
	ScreenImpl{ctx, params},
	windowsPtr{&ctx.application().windows()},
	appCtx{ctx} {}

void Screen::addOnFrame(OnFrameDelegate del, int priority)
{
	postFrame();
	onFrameDelegate.insert(del, priority);
}

bool Screen::removeOnFrame(OnFrameDelegate del)
{
	bool removed = onFrameDelegate.remove(del);
	if(!onFrameDelegate.size())
	{
		unpostFrame();
	}
	return removed;
}

bool Screen::containsOnFrame(OnFrameDelegate del) const
{
	return onFrameDelegate.contains(del);
}

void Screen::runOnFrameDelegates(SteadyClockTimePoint time)
{
	postFrame();
	auto params = makeFrameParams(time, std::exchange(lastFrameTime_, time));
	onFrameDelegate.runAll([&](OnFrameDelegate del)
	{
		return del(params);
	});
}

size_t Screen::onFrameDelegates() const
{
	return onFrameDelegate.size();
}

bool Screen::isPosted() const
{
	return framePosted;
}

bool Screen::frameUpdate(SteadyClockTimePoint timestamp)
{
	assert(hasTime(timestamp));
	assert(isActive);
	framePosted = false;
	if(!onFrameDelegate.size())
	{
		lastFrameTime_ = {};
		return false;
	}
	runOnFrameDelegates(timestamp);
	for(auto &w : *windowsPtr)
	{
		if(w->screen() == this)
		{
			w->dispatchOnDraw();
		}
	}
	if(!framePosted)
	{
		lastFrameTime_ = {};
	}
	return true;
}

void Screen::setActive(bool active)
{
	if(active && !isActive)
	{
		log.info("screen:{} activated", (void*)this);
		isActive = true;
		if(onFrameDelegate.size())
			postFrame();
	}
	else if(!active && isActive)
	{
		log.info("screen:{} deactivated", (void*)this);
		isActive = false;
		unpostFrame();
	}
}

FrameParams Screen::makeFrameParams(SteadyClockTimePoint time, SteadyClockTimePoint lastTime) const
{
	return {.time = time, .lastTime = lastTime, .duration = frameRate().duration(), .timeSource = FrameClockSource::Screen};
}

void Screen::postFrame()
{
	if(!isActive) [[unlikely]]
	{
		log.info("skipped posting inactive screen:{}", (void*)this);
		return;
	}
	if(framePosted)
		return;
	//log.info("posting frame");
	framePosted = true;
	postFrameTimer();
}

void Screen::unpostFrame()
{
	if(!framePosted)
		return;
	framePosted = false;
	lastFrameTime_ = {};
	unpostFrameTimer();
}

bool Screen::shouldUpdateFrameTimer(const FrameTimer& frameTimer, bool newVariableFrameTimeValue)
{
	return (newVariableFrameTimeValue && !std::holds_alternative<SimpleFrameTimer>(frameTimer)) ||
		(!newVariableFrameTimeValue && std::holds_alternative<SimpleFrameTimer>(frameTimer));
}

[[gnu::weak]] SteadyClockDuration Screen::presentationDeadline() const { return {}; }


}
