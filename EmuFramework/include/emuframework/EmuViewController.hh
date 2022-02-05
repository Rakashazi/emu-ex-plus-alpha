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
#include <imagine/gui/ViewStack.hh>
#include <imagine/gui/ToastView.hh>

namespace IG
{
class AppContext;
class Screen;
class Window;
class IO;
}

namespace IG::Input
{
class Event;
class KeyEvent;
}

namespace EmuEx
{

using namespace IG;
class EmuSystemTask;
class EmuViewController;
class EmuAudio;
struct WindowData;

class EmuMenuViewStack : public ViewStack
{
public:
	EmuMenuViewStack(EmuViewController &);
	bool inputEvent(const Input::Event &) final;
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
	void pushAndShow(std::unique_ptr<View>, const Input::Event &, bool needsNavView, bool isModal = false) final;
	using ViewController::pushAndShow;
	void pushAndShowModal(std::unique_ptr<View>, const Input::Event &, bool needsNavView);
	void pushAndShowModal(std::unique_ptr<View>, bool needsNavView);
	void pop() final;
	void popTo(View &v) final;
	void dismissView(View &v, bool refreshLayout) final;
	void dismissView(int idx, bool refreshLayout) final;
	bool inputEvent(const Input::Event &) final;
	void showEmulation();
	void showUI(bool updateTopView = true);
	bool showAutoStateConfirm(const Input::Event &);
	void placeEmuViews();
	void placeElements();
	void setEmuViewOnExtraWindow(bool on, IG::Screen &screen);
	void startMainViewportAnimation();
	void updateEmuAudioStats(unsigned underruns, unsigned overruns, unsigned callbacks, double avgCallbackFrames, unsigned frames);
	void clearEmuAudioStats();
	void closeSystem(bool allowAutosaveState = true);
	void popToSystemActionsMenu();
	void postDrawToEmuWindows();
	IG::Screen *emuWindowScreen() const;
	IG::Window &emuWindow() const;
	WindowData &emuWindowData();
	Gfx::RendererTask &rendererTask() const;
	bool hasModalView() const;
	void popModalViews();
	void prepareDraw();
	void popToRoot();
	void showNavView(bool show);
	void setShowNavViewBackButton(bool show);
	void showSystemActionsView(ViewAttachParams, const Input::Event &);
	void onInputDevicesChanged();
	void onSystemCreated();
	EmuInputView &inputView();
	ToastView &popupMessageView();
	EmuVideoLayer &videoLayer() const;
	EmuAudio &emuAudio() const;
	void onScreenChange(IG::ApplicationContext, IG::Screen &, IG::ScreenChange);
	void handleOpenFileCommand(IG::CStringView path);
	void setFastForwardSpeed(int speed);
	bool isMenuDismissKey(const Input::KeyEvent &);
	void setUsePresentationTime(bool on) { usePresentationTime_ = on; }
	bool usePresentationTime() const { return usePresentationTime_; }
	void writeConfig(IO &io);
	IG::ApplicationContext appContext() const;
	EmuSystemTask &emuTask() { return *systemTaskPtr; }

protected:
	static constexpr bool HAS_USE_RENDER_TIME = Config::envIsLinux
		|| (Config::envIsAndroid && Config::ENV_ANDROID_MIN_SDK < 16);
	EmuView emuView{};
	EmuInputView emuInputView{};
	ToastView popup{};
	EmuMenuViewStack viewStack{*this};
	IG::OnFrameDelegate onFrameUpdate{};
	Gfx::RendererTask *rendererTask_{};
	EmuSystemTask *systemTaskPtr{};
	EmuAudio *emuAudioPtr{};
	EmuApp *appPtr{};
	IG::OnExit onExit{};
	bool showingEmulation{};
	IG::WindowFrameTimeSource winFrameTimeSrc{};
	uint8_t targetFastForwardSpeed{};
	IG_UseMemberIf(Config::envIsAndroid, bool, usePresentationTime_){true};

	void initViews(ViewAttachParams attach);
	void onFocusChange(bool in);
	IG::OnFrameDelegate makeOnFrameDelayed(uint8_t delay);
	void addOnFrameDelegate(IG::OnFrameDelegate onFrame);
	void addOnFrameDelayed();
	void addOnFrame();
	void removeOnFrame();
	void moveOnFrame(IG::Window &from, IG::Window &to);
	void startEmulation();
	void pauseEmulation();
	void configureAppForEmulation(bool running);
	void configureWindowForEmulation(IG::Window &win, bool running);
	void startViewportAnimation(IG::Window &win);
	void updateWindowViewport(IG::Window &win, IG::WindowSurfaceChange change);
	void drawMainWindow(IG::Window &win, Gfx::RendererCommands &cmds, bool hasEmuView, bool hasPopup);
	void movePopupToWindow(IG::Window &win);
	void moveEmuViewToWindow(IG::Window &win);
	void applyFrameRates(bool updateFrameTime = true);
	bool allWindowsAreFocused() const;
	WindowData &mainWindowData() const;
	IG::Window &mainWindow() const;
	void setWindowFrameClockSource(IG::WindowFrameTimeSource);
	bool useRendererTime() const;
	void configureSecondaryScreens();
};

}
