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

#define LOGTAG "AppContext"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/DelegateFuncSet.hh>
#include <imagine/util/ScopeGuard.hh>
#include "basePrivate.hh"
#include <imagine/logger/logger.h>
#include <cstring>

namespace Base
{

static InterProcessMessageDelegate onInterProcessMessage_;
static DelegateFuncSet<ResumeDelegate> onResume_;
static FreeCachesDelegate onFreeCaches_;
static DelegateFuncSet<ExitDelegate> onExit_;
static ScreenChangeDelegate onScreenChange_;
static ActivityState appState = ActivityState::PAUSED;

#ifdef CONFIG_BASE_MULTI_WINDOW
static std::vector<std::unique_ptr<Window>> window_;
#else
static std::array<std::unique_ptr<Window>, 1> window_;
#endif

#ifdef CONFIG_BASE_MULTI_SCREEN
std::vector<std::unique_ptr<Screen>> screen_;
#else
static std::array<std::unique_ptr<Screen>, 1> screen_;
#endif

static void addWindow(std::unique_ptr<Window> winPtr)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.emplace_back(std::move(winPtr));
	#else
	assert(!window_[0]);
	window_[0] = std::move(winPtr);
	#endif
}

std::unique_ptr<Window> moveOutWindow(Window &win)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	return IG::moveOutIf(window_, [&](auto &w){ return *w == win; });
	#else
	return std::move(window_[0]);
	#endif
}

void deinitWindows()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.clear();
	#else
	window_[0].reset();
	#endif
}

Window *ApplicationContext::makeWindow(WindowConfig config, WindowInitDelegate onInit)
{
	if(!Config::BASE_MULTI_WINDOW && windows())
	{
		bug_unreachable("no multi-window support");
	}
	auto winPtr = std::make_unique<Window>(*this, config, onInit);
	if(!*winPtr)
	{
		return nullptr;
	}
	auto ptr = winPtr.get();
	addWindow(std::move(winPtr));
	if(Window::shouldRunOnInitAfterAddingWindow && onInit)
		onInit(*this, *ptr);
	return ptr;
}

unsigned ApplicationContext::windows() const
{
	if constexpr(Config::BASE_MULTI_WINDOW)
	{
		return window_.size();
	}
	else
	{
		return (bool)window_[0];
	}
}

Window *ApplicationContext::window(unsigned idx) const
{
	if(unlikely(idx >= window_.size()))
		return nullptr;
	return window_[idx].get();
}

Window &ApplicationContext::mainWindow()
{
	assert(windows());
	return *window(0);
}

Screen &ApplicationContext::addScreen(std::unique_ptr<Screen> ptr, bool notify)
{
	auto &screen = *ptr.get();
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_.emplace_back(std::move(ptr));
	if(notify && onScreenChange_)
		onScreenChange_(*this, screen, {ScreenChange::ADDED});
	#else
	assert(!screen_[0]);
	screen_[0] = std::move(ptr);
	#endif
	return screen;
}

Screen *ApplicationContext::findScreen(ScreenId id) const
{
	auto it = IG::find_if(screen_, [&](const auto &s) { return *s == id; });
	if(it == screen_.end())
	{
		return nullptr;
	}
	return it->get();
}

std::unique_ptr<Screen> ApplicationContext::removeScreen(ScreenId id, bool notify)
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	auto removedScreen = IG::moveOutIf(screen_, [&](const auto &s){ return *s == id; });
	if(notify && removedScreen && onScreenChange_)
		onScreenChange_(*this, *removedScreen, {ScreenChange::REMOVED});
	return removedScreen;
	#else
	return {};
	#endif
}

void ApplicationContext::removeSecondaryDisplays()
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	while(screen_.size() > 1)
	{
		screen_.pop_back();
	}
	#endif
}

unsigned ApplicationContext::screens() const
{
	return screen_.size();
}

Screen *ApplicationContext::screen(unsigned idx) const
{
	if(idx >= screen_.size())
		return nullptr;
	return screen_[idx].get();
}

Screen &ApplicationContext::mainScreen()
{
	return *screen(0);
}

bool ApplicationContext::screensArePosted() const
{
	for(auto &screen : screen_)
	{
		if(screen->isPosted())
			return true;
	}
	return false;
}

void ApplicationContext::setActiveForAllScreens(bool active)
{
	for(auto &screen : screen_)
	{
		screen->setActive(active);
	}
}

ActivityState activityState()
{
	return appState;
}

void setPausedActivityState()
{
	if(appState == ActivityState::EXITING)
	{
		return; // ignore setting paused state while exiting
	}
	appState = ActivityState::PAUSED;
}

void setRunningActivityState()
{
	assert(appState != ActivityState::EXITING); // should never set running state after exit state
	appState = ActivityState::RUNNING;
}

void setExitingActivityState()
{
	appState = ActivityState::EXITING;
}

bool ApplicationContext::isRunning() const
{
	return activityState() == ActivityState::RUNNING;
}

bool appIsPaused()
{
	return activityState() == ActivityState::PAUSED;
}

bool appIsExiting()
{
	return activityState() == ActivityState::EXITING;
}

void ApplicationContext::setOnInterProcessMessage(InterProcessMessageDelegate del)
{
	onInterProcessMessage_ = del;
}

bool ApplicationContext::addOnResume(ResumeDelegate del, int priority)
{
	return onResume_.add(del, priority);
}

bool ApplicationContext::removeOnResume(ResumeDelegate del)
{
	if(appIsExiting())
		return false;
	return onResume_.remove(del);
}

bool ApplicationContext::containsOnResume(ResumeDelegate del) const
{
	return onResume_.contains(del);
}

void ApplicationContext::setOnFreeCaches(FreeCachesDelegate del)
{
	onFreeCaches_ = del;
}

bool ApplicationContext::addOnExit(ExitDelegate del, int priority)
{
	return onExit_.add(del, priority);
}

bool ApplicationContext::removeOnExit(ExitDelegate del)
{
	if(appIsExiting())
		return false;
	return onExit_.remove(del);
}

bool ApplicationContext::containsOnExit(ExitDelegate del) const
{
	return onExit_.contains(del);
}

void ApplicationContext::dispatchOnInterProcessMessage(const char *filename)
{
	onInterProcessMessage_.callCopySafe(*this, filename);
}

bool ApplicationContext::hasOnInterProcessMessage() const
{
	return (bool)onInterProcessMessage_;
}

void ApplicationContext::setOnScreenChange(ScreenChangeDelegate del)
{
	onScreenChange_ = del;
}

void ApplicationContext::dispatchOnResume(bool focused)
{
	onResume_.runAll([&](ResumeDelegate del){ return del(*this, focused); });
}

void ApplicationContext::dispatchOnFreeCaches(bool running)
{
	onFreeCaches_.callCopySafe(*this, running);
}

void ApplicationContext::dispatchOnExit(bool backgrounded)
{
	onExit_.runAll([&](ExitDelegate del){ return del(*this, backgrounded); }, true);
}

FS::RootPathInfo ApplicationContext::nearestRootPath(const char *path)
{
	if(!path)
		return {};
	auto location = rootFileLocations();
	const FS::PathLocation *nearestPtr{};
	size_t lastMatchOffset = 0;
	for(const auto &l : location)
	{
		auto subStr = strstr(path, l.path.data());
		if(subStr != path)
			continue;
		auto matchOffset = (size_t)(&path[l.root.length] - path);
		if(matchOffset > lastMatchOffset)
		{
			nearestPtr = &l;
			lastMatchOffset = matchOffset;
		}
	}
	if(!lastMatchOffset)
		return {};
	logMsg("found root location:%s with length:%d", nearestPtr->root.name.data(), (int)nearestPtr->root.length);
	return {nearestPtr->root.name, nearestPtr->root.length};
}

AssetIO ApplicationContext::openAsset(const char *name, IO::AccessHint access, const char *appName)
{
	AssetIO io;
	#ifdef __ANDROID__
	io.open(*this, name, access);
	#else
	io.open(FS::makePathStringPrintf("%s/%s", assetPath(appName).data(), name).data(), access);
	#endif
	return io;
}

Orientation ApplicationContext::validateOrientationMask(Orientation oMask) const
{
	if(!(oMask & VIEW_ROTATE_ALL))
	{
		// use default when none of the orientation bits are set
		oMask = defaultSystemOrientations();
	}
	return oMask;
}

void ApplicationContext::exitWithErrorMessagePrintf(int exitVal, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	auto vaEnd = IG::scopeGuard([&](){ va_end(args); });
	exitWithErrorMessageVPrintf(exitVal, format, args);
}

OnExit::OnExit(ResumeDelegate del, ApplicationContext app, int priority): del{del}, app{app}
{
	app.addOnExit(del, priority);
}

OnExit::OnExit(OnExit &&o)
{
	*this = std::move(o);
}

OnExit &OnExit::operator=(OnExit &&o)
{
	if(del)
		app.removeOnExit(del);
	del = std::exchange(o.del, {});
	app = o.app;
	return *this;
}

OnExit::~OnExit()
{
	if(del)
		app.removeOnExit(del);
}

ApplicationContext OnExit::appContext() const
{
	return app;
}

}
