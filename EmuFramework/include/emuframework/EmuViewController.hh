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
class Viewport;
}

namespace IG::Input
{
class Event;
class KeyEvent;
}

namespace EmuEx
{

using namespace IG;
class EmuApp;
class EmuAudio;
class EmuSystem;
struct WindowData;
struct FrameTimeConfig;
class MainMenuView;

class EmuMenuViewStack : public ViewStack
{
public:
	EmuMenuViewStack(ViewAttachParams, EmuApp &);
	bool inputEvent(const Input::Event&) final;
	constexpr EmuApp &app() { return *emuAppPtr; }

protected:
	EmuApp *emuAppPtr;
};

class EmuViewController final: public ViewController, public EmuAppHelper
{
public:
	EmuViewController(ViewAttachParams, VController &, EmuVideoLayer &, EmuSystem &);
	void pushAndShow(std::unique_ptr<View>, const Input::Event &, bool needsNavView, bool isModal = false) final;
	using ViewController::pushAndShow;
	void pushAndShowModal(std::unique_ptr<View>, const Input::Event &, bool needsNavView);
	void pushAndShowModal(std::unique_ptr<View>, bool needsNavView);
	void pop() final;
	void popTo(View &v) final;
	void dismissView(View &v, bool refreshLayout) final;
	void dismissView(int idx, bool refreshLayout) final;
	bool inputEvent(const Input::Event&) final;
	bool extraWindowInputEvent(const Input::Event &e);
	void showEmulationView(FrameTimeConfig);
	void showMenuView(bool updateTopView);
	void placeEmuViews();
	void placeElements();
	void updateMainWindowViewport(IG::Window &, IG::Viewport, Gfx::RendererTask &);
	void updateExtraWindowViewport(IG::Window &, IG::Viewport, Gfx::RendererTask &);
	bool drawMainWindow(IG::Window &win, IG::WindowDrawParams, Gfx::RendererTask &);
	bool drawExtraWindow(IG::Window &win, IG::WindowDrawParams, Gfx::RendererTask &);
	void popToSystemActionsMenu();
	void postDrawToEmuWindows();
	IG::Screen *emuWindowScreen() const;
	IG::Window &emuWindow() const;
	WindowData &emuWindowData();
	bool hasModalView() const;
	void popModalViews();
	void prepareDraw();
	void popToRoot();
	void showNavView(bool show);
	void setShowNavViewBackButton(bool show);
	void showSystemActionsView(ViewAttachParams, const Input::Event &);
	void onInputDevicesChanged();
	void onSystemCreated();
	void onSystemClosed();
	MainMenuView &mainMenu();
	bool isMenuDismissKey(const Input::KeyEvent &) const;
	IG::ApplicationContext appContext() const;
	bool isShowingEmulation() const { return showingEmulation; }
	void onHide();
	void movePopupToWindow(IG::Window &win);
	void moveEmuViewToWindow(IG::Window &win);
	View &top() const { return viewStack.top(); }

public:
	EmuView emuView;
	EmuInputView inputView;
	ToastView popup;
	ConditionalMember<Gfx::supportsPresentationTime, SteadyClockTimePoint> presentTime{};
protected:
	EmuMenuViewStack viewStack;
	bool showingEmulation{};
public:
	bool drawBlankFrame{};

	static constexpr bool HAS_USE_RENDER_TIME = Config::envIsLinux
		|| (Config::envIsAndroid && Config::ENV_ANDROID_MIN_SDK < 16);

	void configureWindowForEmulation(Window &, FrameTimeConfig, bool running);
	EmuVideoLayer &videoLayer() const;
};

}
