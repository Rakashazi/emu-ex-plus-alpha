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
#include <imagine/base/baseDefs.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/MessagePort.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/DelegateFuncSet.hh>
#include <imagine/util/NonCopyable.hh>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace Base
{

enum class ActivityState : uint8_t
{
	PAUSED,
	RUNNING,
	EXITING
};

struct CommandArgs
{
	int c{};
	char **v{};
};

class BaseApplication : private NonCopyable
{
public:
	BaseApplication(ApplicationContext);
	virtual ~BaseApplication();
	ActivityState activityState() const;
	void setPausedActivityState();
	void setRunningActivityState();
	void setExitingActivityState();
	bool isRunning() const;
	bool isPaused() const;
	bool isExiting() const;

	void addWindow(std::unique_ptr<Window>);
	std::unique_ptr<Window> moveOutWindow(Window &win);
	const WindowContainter &windows() const;
	Window &mainWindow() const;

	bool addOnResume(ResumeDelegate, int priority);
	bool removeOnResume(ResumeDelegate);
	bool containsOnResume(ResumeDelegate) const;
	void dispatchOnResume(ApplicationContext, bool focused);

	bool addOnExit(ExitDelegate, int priority = APP_ON_EXIT_PRIORITY);
	bool removeOnExit(ExitDelegate);
	bool containsOnExit(ExitDelegate) const;
	void dispatchOnExit(ApplicationContext, bool backgrounded);

	void setOnFreeCaches(FreeCachesDelegate del);
	void dispatchOnFreeCaches(ApplicationContext, bool running);

	void setOnScreenChange(ScreenChangeDelegate del);
	Screen &addScreen(ApplicationContext, std::unique_ptr<Screen>, bool notify);
	Screen *findScreen(ScreenId) const;
	std::unique_ptr<Screen> removeScreen(ApplicationContext, ScreenId, bool notify);
	const ScreenContainter &screens() const;
	Screen &mainScreen() const;
	bool screensArePosted() const;
	void setActiveForAllScreens(bool active);

	void setOnInterProcessMessage(InterProcessMessageDelegate);
	bool hasOnInterProcessMessage() const;
	void dispatchOnInterProcessMessage(ApplicationContext, const char *filename);

	// Input functions
	void startKeyRepeatTimer(Input::Event);
	void cancelKeyRepeatTimer();
	void deinitKeyRepeatTimer();
	void setAllowKeyRepeatTimer(bool on);
	const InputDeviceContainer &systemInputDevices() const;
	void addSystemInputDevice(Input::Device &d, bool notify = false);
	void removeSystemInputDevice(Input::Device &d, bool notify = false);
	bool dispatchRepeatableKeyInputEvent(Input::Event, Window &);
	bool dispatchRepeatableKeyInputEvent(Input::Event);
	bool dispatchKeyInputEvent(Input::Event, Window &);
	bool dispatchKeyInputEvent(Input::Event);
	void setOnInputDeviceChange(InputDeviceChangeDelegate);
	void dispatchInputDeviceChange(const Input::Device &, Input::DeviceChange);
	void setOnInputDevicesEnumerated(InputDevicesEnumeratedDelegate);
	std::optional<bool> swappedConfirmKeysOption() const;
	bool swappedConfirmKeys() const;
	void setSwappedConfirmKeys(std::optional<bool>);
	uint8_t keyEventFlags() const;
	bool processICadeKey(Input::Key, Input::Action, Input::Time, const Input::Device &, Base::Window &);

protected:
	struct CommandMessage
	{
		MainThreadMessageDelegate del{};
		constexpr explicit operator bool() const { return (bool)del; }
	};

	InterProcessMessageDelegate onInterProcessMessage_;
	DelegateFuncSet<ResumeDelegate> onResume_;
	FreeCachesDelegate onFreeCaches_;
	DelegateFuncSet<ExitDelegate> onExit_;
	ScreenChangeDelegate onScreenChange_;
	InputDeviceChangeDelegate onInputDeviceChange{};
	InputDevicesEnumeratedDelegate onInputDevicesEnumerated{};
	WindowContainter window_{};
	ScreenContainter screen_{};
	MessagePort<CommandMessage> commandPort{"Main thread messages"};
	InputDeviceContainer inputDev{};
	std::optional<Base::Timer> keyRepeatTimer{};
	Input::Event keyRepeatEvent{};
	bool allowKeyRepeatTimer_{true};
	bool swappedConfirmKeys_{Input::SWAPPED_CONFIRM_KEYS_DEFAULT};
	ActivityState appState = ActivityState::PAUSED;

	void deinitWindows();
	void removeSecondaryScreens();
	void indexSystemInputDevices();
};

}
