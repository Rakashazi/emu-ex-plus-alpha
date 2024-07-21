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

#include <imagine/base/Application.hh>
#include <imagine/logger/logger.h>

namespace IG
{

const char *copyright = "Imagine is Copyright 2010-2023 Robert Broglia";
constexpr SystemLogger log{"App"};

BaseApplication::BaseApplication(ApplicationContext ctx)
{
	log.debug("{}", copyright);
	log.debug("compiled on {} {}", __DATE__, __TIME__);

	commandPort.attach({},
		[ctx](auto msgs)
		{
			for(auto msg : msgs)
			{
				msg.del(ctx);
			}
			return true;
		});
}

void BaseApplication::addWindow(std::unique_ptr<Window> winPtr)
{
	window_.emplace_back(std::move(winPtr));
}

std::unique_ptr<Window> BaseApplication::moveOutWindow(Window &win)
{
	return moveOut(window_, [&](const auto& w){ return *w == win; });
}

void BaseApplication::deinitWindows()
{
	window_.clear();
}

const WindowContainer &BaseApplication::windows() const
{
	return window_;
}

Window &BaseApplication::mainWindow() const
{
	assert(windows().size());
	return *windows()[0];
}

Screen &BaseApplication::addScreen(ApplicationContext ctx, std::unique_ptr<Screen> ptr, bool notify)
{
	auto &newScreen = screen_.emplace_back(std::move(ptr));
	if(notify)
		onEvent(ctx, ScreenChangeEvent{*newScreen, ScreenChange::added});
	return *newScreen;
}

Screen* BaseApplication::findScreen(ScreenId id) const
{
	return findPtr(screen_, [&](const auto &s) { return *s == id; });
}

std::unique_ptr<Screen> BaseApplication::removeScreen(ApplicationContext ctx, ScreenId id, bool notify)
{
	auto removedScreen = moveOut(screen_, [&](const auto &s){ return *s == id; });
	if(notify && removedScreen)
		onEvent(ctx, ScreenChangeEvent{*removedScreen, ScreenChange::removed});
	return removedScreen;
}

void BaseApplication::removeSecondaryScreens()
{
	if constexpr(Config::BASE_MULTI_SCREEN)
	{
		while(screen_.size() > 1)
		{
			screen_.pop_back();
		}
	}
}

const ScreenContainer &BaseApplication::screens() const
{
	return screen_;
}

Screen &BaseApplication::mainScreen() const
{
	return *screens()[0];
}

bool BaseApplication::screensArePosted() const
{
	for(auto &screen : screen_)
	{
		if(screen->isPosted())
			return true;
	}
	return false;
}

void BaseApplication::setActiveForAllScreens(bool active)
{
	for(auto &screen : screen_)
	{
		screen->setActive(active);
	}
}

ActivityState BaseApplication::activityState() const
{
	return appState;
}

void BaseApplication::setPausedActivityState()
{
	if(appState == ActivityState::EXITING)
	{
		return; // ignore setting paused state while exiting
	}
	appState = ActivityState::PAUSED;
}

void BaseApplication::setRunningActivityState()
{
	assert(appState != ActivityState::EXITING); // should never set running state after exit state
	appState = ActivityState::RUNNING;
}

void BaseApplication::setExitingActivityState()
{
	appState = ActivityState::EXITING;
}

bool BaseApplication::isRunning() const
{
	return activityState() == ActivityState::RUNNING;
}

bool BaseApplication::isPaused() const
{
	return activityState() == ActivityState::PAUSED;
}

bool BaseApplication::isExiting() const
{
	return activityState() == ActivityState::EXITING;
}

bool BaseApplication::addOnResume(ResumeDelegate del, int priority)
{
	return onResume_.add(del, priority);
}

bool BaseApplication::removeOnResume(ResumeDelegate del)
{
	return onResume_.remove(del);
}

bool BaseApplication::containsOnResume(ResumeDelegate del) const
{
	return onResume_.contains(del);
}

bool BaseApplication::addOnExit(ExitDelegate del, int priority)
{
	return onExit_.add(del, priority);
}

bool BaseApplication::removeOnExit(ExitDelegate del)
{
	return onExit_.remove(del);
}

bool BaseApplication::containsOnExit(ExitDelegate del) const
{
	return onExit_.contains(del);
}

void BaseApplication::dispatchOnScreenChange(ApplicationContext ctx, Screen &s, ScreenChange change)
{
	onEvent(ctx, ScreenChangeEvent{s, change});
}

void BaseApplication::dispatchOnResume(ApplicationContext ctx, bool focused)
{
	onResume_.runAll([&](ResumeDelegate del){ return del(ctx, focused); });
}

void BaseApplication::dispatchOnFreeCaches(ApplicationContext ctx, bool running)
{
	onEvent.callCopy(ctx, FreeCachesEvent{running});
}

void BaseApplication::dispatchOnExit(ApplicationContext ctx, bool backgrounded)
{
	onExit_.runAll([&](ExitDelegate del){ return del(ctx, backgrounded); }, true);
	if(!backgrounded)
	{
		// destroy app window data since it was allocated by the app subclass
		// for logical destructor ordering
		for(auto &win : window_)
		{
			win->resetAppData();
			win->resetRendererData();
			// surface should be destroyed by reseting renderer data so skip surface change event
			win->onEvent = delegateFuncDefaultInit;
		}
	}
}

[[gnu::weak]] void ApplicationContext::setIdleDisplayPowerSave(bool) {}

[[gnu::weak]] void ApplicationContext::endIdleByUserActivity() {}

[[gnu::weak]] bool ApplicationContext::registerInstance(ApplicationInitParams, const char*) { return false; }

[[gnu::weak]] void ApplicationContext::setAcceptIPC(bool, const char*) {}

void Application::runOnMainThread(MainThreadMessageDelegate del)
{
	if(!del) [[unlikely]]
		return;
	commandPort.send({del});
}

void Application::flushMainThreadMessages()
{
	commandPort.dispatchMessages();
}

}
