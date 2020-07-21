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

#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/ViewStack.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/gfx/AnimatedViewport.hh>
#include <imagine/gfx/DrawableHolder.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "Recent.hh"
#include <memory>
#include <atomic>

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

class EmuSystemTask;

struct AppWindowData
{
	Base::Window win{};
	Gfx::DrawableHolder drawableHolder{};
	Gfx::Viewport viewport() const { return projectionPlane.viewport(); }
	Gfx::Mat4 projectionMat{};
	Gfx::ProjectionPlane projectionPlane{};
	Gfx::AnimatedViewport animatedViewport{};
	bool hasEmuView = false;
	bool hasPopup = false;
	bool focused = true;

	constexpr AppWindowData() {};
};

class EmuMenuViewStack : public ViewStack
{
public:
	bool inputEvent(Input::Event e) final;
};

class EmuViewController : public ViewController
{
public:
	EmuViewController(AppWindowData &winData, Gfx::Renderer &renderer, Gfx::RendererTask &rTask,
		VController &vCtrl, EmuVideoLayer &videoLayer, EmuSystemTask &systemTask);
	void initViews(ViewAttachParams attach);
	Base::WindowConfig addWindowConfig(Base::WindowConfig conf, AppWindowData &winData);
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal = false) final;
	using ViewController::pushAndShow;
	void pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView);
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
	void updateEmuAudioStats(uint underruns, uint overruns, uint callbacks, double avgCallbackFrames, uint frames);
	void clearEmuAudioStats();
	void closeSystem(bool allowAutosaveState = true);
	void popToSystemActionsMenu();
	void postDrawToEmuWindows();
	Base::Screen *emuWindowScreen() const;
	Base::Window &emuWindow() const;
	AppWindowData &emuWindowData();
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
	void onScreenChange(Base::Screen &screen, Base::Screen::Change change);
	void handleOpenFileCommand(const char *path);
	void setOnScreenControls(bool on);
	void updateAutoOnScreenControlVisible();
	void setPhysicalControlsPresent(bool present);
	void setFastForwardActive(bool active);

protected:
	static constexpr bool HAS_USE_RENDER_TIME = Config::envIsLinux
		|| (Config::envIsAndroid && Config::ENV_ANDROID_MINSDK < 16);
	EmuView emuView;
	EmuInputView emuInputView;
	ToastView popup;
	EmuMenuViewStack viewStack{};
	Base::OnFrameDelegate onFrameUpdate{};
	Gfx::RendererTask *rendererTask_{};
	EmuSystemTask *systemTask{};
	bool showingEmulation = false;
	bool physicalControlsPresent = false;
	[[no_unique_address]] IG::UseTypeIf<HAS_USE_RENDER_TIME, bool> useRendererTime_ = false;
	uint8_t targetFastForwardSpeed = 0;
	std::atomic_bool emuVideoInProgress{};

	void onFocusChange(uint in);
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
	void startViewportAnimation(AppWindowData &winData);
	void updateWindowViewport(AppWindowData &winData, Base::Window::SurfaceChange change);
	void drawMainWindow(Base::Window &win, Gfx::RendererCommands &cmds, bool hasEmuView, bool hasPopup);
	void movePopupToWindow(Base::Window &win);
	void moveEmuViewToWindow(Base::Window &win);
	void applyFrameRates();
	bool allWindowsAreFocused() const;
	AppWindowData &mainWindowData() const;
	void setUseRendererTime(bool on);
	bool useRendererTime() const;
};

extern EmuVideoLayer emuVideoLayer;
extern EmuViewController emuViewController;
extern DelegateFunc<void ()> onUpdateInputDevices;
extern FS::PathString lastLoadPath;
extern EmuVideo emuVideo;
extern EmuAudio emuAudio;
extern RecentGameList recentGameList;
static constexpr const char *strftimeFormat = "%x  %r";

void loadConfigFile();
void saveConfigFile();
void addRecentGame(const char *fullPath, const char *name);
bool isMenuDismissKey(Input::Event e);
void applyOSNavStyle(bool inGame);
const char *appViewTitle();
const char *appName();
const char *appID();
bool hasGooglePlayStoreFeatures();
void setCPUNeedsLowLatency(bool needed);
void onMainMenuItemOptionChanged();
void runBenchmarkOneShot();
void onSelectFileFromPicker(const char* name, Input::Event e);
void launchSystem(bool tryAutoState, bool addToRecent);
Gfx::PixmapTexture &getAsset(Gfx::Renderer &r, AssetID assetID);
ViewAttachParams emuViewAttachParams();
std::unique_ptr<View> makeEmuView(ViewAttachParams attach, EmuApp::ViewID id);
Gfx::Viewport makeViewport(const Base::Window &win);
void updateProjection(AppWindowData &appWin, const Gfx::Viewport &viewport);
AppWindowData &appWindowData(const Base::Window &win);
uint8_t currentFrameInterval();
IG::PixelFormatID optionImageEffectPixelFormatValue();

static void addRecentGame()
{
	addRecentGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName().data());
}
