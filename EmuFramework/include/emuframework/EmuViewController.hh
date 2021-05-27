#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/base/Window.hh>
#include <imagine/gui/ViewStack.hh>
#include <imagine/gui/ToastView.hh>

namespace Base
{
class AppContext;
class Screen;
}

namespace Input
{
class Event;
}

class EmuSystemTask;
class EmuViewController;
class EmuAudio;
struct WindowData;

class EmuMenuViewStack : public ViewStack
{
public:
	EmuMenuViewStack(EmuViewController &);
	bool inputEvent(Input::Event e) final;
	constexpr EmuViewController &viewController() { return *emuViewControllerPtr; }

protected:
	EmuViewController *emuViewControllerPtr;
};

class EmuViewController final: public ViewController, public EmuAppHelper<EmuViewController>
{
public:
	EmuViewController();
	EmuViewController(ViewAttachParams,
		VController &, EmuVideoLayer &, EmuSystemTask &, EmuAudio &);
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal = false) final;
	using ViewController::pushAndShow;
	void pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView);
	void pushAndShowModal(std::unique_ptr<View> v, bool needsNavView);
	void pop() final;
	void popTo(View &v) final;
	void dismissView(View &v, bool refreshLayout) final;
	void dismissView(int idx, bool refreshLayout) final;
	bool inputEvent(Input::Event e) final;
	void showEmulation();
	void showUI(bool updateTopView = true);
	bool showAutoStateConfirm(Input::Event e, bool addToRecent);
	void placeEmuViews();
	void placeElements();
	void setEmuViewOnExtraWindow(bool on, Base::Screen &screen);
	void startMainViewportAnimation();
	void updateEmuAudioStats(unsigned underruns, unsigned overruns, unsigned callbacks, double avgCallbackFrames, unsigned frames);
	void clearEmuAudioStats();
	void closeSystem(bool allowAutosaveState = true);
	void popToSystemActionsMenu();
	void postDrawToEmuWindows();
	Base::Screen *emuWindowScreen() const;
	Base::Window &emuWindow() const;
	WindowData &emuWindowData();
	Gfx::RendererTask &rendererTask() const;
	bool hasModalView() const;
	void popModalViews();
	void prepareDraw();
	void popToRoot();
	void showNavView(bool show);
	void setShowNavViewBackButton(bool show);
	void showSystemActionsView(ViewAttachParams attach, Input::Event e);
	void onInputDevicesChanged();
	void onSystemCreated();
	EmuInputView &inputView();
	ToastView &popupMessageView();
	EmuVideoLayer &videoLayer() const;
	EmuAudio &emuAudio() const;
	void onScreenChange(Base::ApplicationContext, Base::Screen &, Base::ScreenChange);
	void handleOpenFileCommand(const char *path);
	void setOnScreenControls(bool on);
	void updateAutoOnScreenControlVisible();
	void setPhysicalControlsPresent(bool present);
	void setFastForwardActive(bool active);
	bool isMenuDismissKey(Input::Event);
	Base::ApplicationContext appContext() const;

protected:
	static constexpr bool HAS_USE_RENDER_TIME = Config::envIsLinux
		|| (Config::envIsAndroid && Config::ENV_ANDROID_MINSDK < 16);
	EmuView emuView{};
	EmuInputView emuInputView{};
	ToastView popup{};
	EmuMenuViewStack viewStack{*this};
	Base::OnFrameDelegate onFrameUpdate{};
	Gfx::RendererTask *rendererTask_{};
	EmuSystemTask *systemTask{};
	EmuAudio *emuAudioPtr{};
	EmuApp *appPtr{};
	Base::OnExit onExit{};
	bool showingEmulation{};
	bool physicalControlsPresent{};
	Base::Window::FrameTimeSource winFrameTimeSrc{};
	uint8_t targetFastForwardSpeed{};

	void initViews(ViewAttachParams attach);
	void onFocusChange(bool in);
	Base::OnFrameDelegate makeOnFrameDelayed(uint8_t delay);
	void addOnFrameDelegate(Base::OnFrameDelegate onFrame);
	void addOnFrameDelayed();
	void addOnFrame();
	void removeOnFrame();
	void moveOnFrame(Base::Window &from, Base::Window &to);
	void startEmulation();
	void pauseEmulation();
	void configureAppForEmulation(bool running);
	void configureWindowForEmulation(Base::Window &win, bool running);
	void startViewportAnimation(Base::Window &win);
	void updateWindowViewport(Base::Window &win, Base::Window::SurfaceChange change);
	void drawMainWindow(Base::Window &win, Gfx::RendererCommands &cmds, bool hasEmuView, bool hasPopup);
	void movePopupToWindow(Base::Window &win);
	void moveEmuViewToWindow(Base::Window &win);
	void applyFrameRates(bool updateFrameTime = true);
	bool allWindowsAreFocused() const;
	WindowData &mainWindowData() const;
	Base::Window &mainWindow() const;
	void setWindowFrameClockSource(Base::Window::FrameTimeSource);
	bool useRendererTime() const;
	void configureSecondaryScreens();
};
