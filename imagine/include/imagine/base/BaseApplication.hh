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
#include <imagine/input/Device.hh>
#include <imagine/util/DelegateFuncSet.hh>
#include <memory>
#include <optional>
#include <cstdint>
#include <string_view>
#include <algorithm>

namespace IG
{

enum class BluetoothSocketState: uint8_t;

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

class BaseApplication
{
public:
	OnApplicationEvent onEvent{delegateFuncDefaultInit};

	BaseApplication(ApplicationContext);
	virtual ~BaseApplication() = default;
	BaseApplication &operator=(BaseApplication &&) = delete;
	ActivityState activityState() const;
	void setPausedActivityState();
	void setRunningActivityState();
	void setExitingActivityState();
	bool isRunning() const;
	bool isPaused() const;
	bool isExiting() const;

	void addWindow(std::unique_ptr<Window>);
	std::unique_ptr<Window> moveOutWindow(Window &win);
	const WindowContainer &windows() const;
	Window &mainWindow() const;

	bool addOnResume(ResumeDelegate, int priority);
	bool removeOnResume(ResumeDelegate);
	bool containsOnResume(ResumeDelegate) const;
	void dispatchOnResume(ApplicationContext, bool focused);

	bool addOnExit(ExitDelegate, int priority = APP_ON_EXIT_PRIORITY);
	bool removeOnExit(ExitDelegate);
	bool containsOnExit(ExitDelegate) const;
	void dispatchOnExit(ApplicationContext, bool backgrounded);

	void dispatchOnFreeCaches(ApplicationContext, bool running);

	void dispatchOnScreenChange(ApplicationContext ctx, Screen &, ScreenChange);
	Screen &addScreen(ApplicationContext, std::unique_ptr<Screen>, bool notify);
	Screen *findScreen(ScreenId) const;
	std::unique_ptr<Screen> removeScreen(ApplicationContext, ScreenId, bool notify);
	const ScreenContainer &screens() const;
	Screen &mainScreen() const;
	bool screensArePosted() const;
	void setActiveForAllScreens(bool active);

	// Input functions
	void startKeyRepeatTimer(Input::KeyEvent);
	void cancelKeyRepeatTimer();
	void deinitKeyRepeatTimer();
	void setAllowKeyRepeatTimer(bool on);
	const InputDeviceContainer &inputDevices() const;
	Input::Device &addInputDevice(ApplicationContext, std::unique_ptr<Input::Device>, bool notify = false);
	void removeInputDevice(ApplicationContext, Input::Device &, bool notify = false);
	void removeInputDevice(ApplicationContext, Input::Map map, int id, bool notify = false);

	void removeInputDeviceIf(ApplicationContext ctx, auto unaryPredicate, bool notify)
	{
		removeInputDevice(ctx, std::ranges::find_if(inputDev, unaryPredicate), notify);
	}

	void removeInputDevices(ApplicationContext, Input::Map matchingMap, bool notify = false);
	bool dispatchRepeatableKeyInputEvent(Input::KeyEvent, Window &);
	bool dispatchRepeatableKeyInputEvent(Input::KeyEvent);
	bool dispatchKeyInputEvent(Input::KeyEvent, Window &);
	bool dispatchKeyInputEvent(Input::KeyEvent);
	void dispatchInputDeviceChange(ApplicationContext, const Input::Device &, Input::DeviceChange);
	std::optional<bool> swappedConfirmKeysOption() const;
	bool swappedConfirmKeys() const;
	void setSwappedConfirmKeys(std::optional<bool>);
	uint8_t keyEventFlags() const;
	bool processICadeKey(const Input::KeyEvent &, Window &);
	void bluetoothInputDeviceStatus(ApplicationContext, Input::Device&, BluetoothSocketState);

protected:
	struct CommandMessage
	{
		MainThreadMessageDelegate del;
	};

	DelegateFuncSet<ResumeDelegate> onResume_;
	DelegateFuncSet<ExitDelegate> onExit_;
	WindowContainer window_{};
	ScreenContainer screen_{};
	MessagePort<CommandMessage> commandPort{"Main thread messages"};
	InputDeviceContainer inputDev{};
	std::optional<Timer> keyRepeatTimer{};
	Input::KeyEvent keyRepeatEvent{};
	bool allowKeyRepeatTimer_{true};
	bool swappedConfirmKeys_{Input::SWAPPED_CONFIRM_KEYS_DEFAULT};
	ActivityState appState = ActivityState::PAUSED;

	void deinitWindows();
	void removeSecondaryScreens();
	uint8_t nextInputDeviceEnumId(std::string_view name) const;
	void removeInputDevice(ApplicationContext, InputDeviceContainer::iterator, bool notify = false);
};

}
