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
#include <imagine/gfx/Gfx.hh>
#include <imagine/util/container/ArrayList.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include "EmuSystemTask.hh"
#include <emuframework/Recent.hh>
#include <memory>
#include <atomic>

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

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

class EmuModalViewStack : public ViewStack
{
public:
	bool inputEvent(Input::Event e) final;
};

class EmuViewController : public ViewController
{
public:
	EmuViewController(AppWindowData &winData, Gfx::Renderer &renderer, Gfx::RendererTask &rTask, VController &vCtrl, EmuVideoLayer &videoLayer);
	void initViews(ViewAttachParams attach);
	Base::WindowConfig addWindowConfig(Base::WindowConfig conf, AppWindowData &winData);
	void pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView) final;
	using ViewController::pushAndShow;
	void pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView);
	void pop() final;
	void dismissView(View &v) final;
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
	void postDrawToEmuWindows();
	Base::Screen *emuWindowScreen() const;
	Base::Window &emuWindow() const;
	Gfx::RendererTask &rendererTask() const;
	bool hasModalView();
	void popModalViews();
	void prepareDraw();
	void popTo(View &v);
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
	EmuView emuView;
	EmuInputView emuInputView;
	ToastView popup;
	EmuMenuViewStack viewStack{};
	EmuModalViewStack modalViewController{};
	Base::Screen::OnFrameDelegate onFrameUpdate{};
	Gfx::RendererTask &rendererTask_;
	Base::FrameTime initialTotalFrameTime{};
	bool showingEmulation = false;
	bool physicalControlsPresent = false;
	bool fastForwardActive = false;
	std::atomic_bool emuVideoInProgress{};

	void onFocusChange(uint in);
	void addInitialOnFrame(Base::Screen &screen, uint delay);
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
};

extern EmuVideoLayer emuVideoLayer;
extern EmuViewController emuViewController;
extern EmuSystemTask emuSystemTask;
extern DelegateFunc<void ()> onUpdateInputDevices;
extern FS::PathString lastLoadPath;
extern EmuVideo emuVideo;
extern EmuAudio emuAudio;
extern StaticArrayList<RecentGameInfo, RecentGameInfo::MAX_RECENT> recentGameList;
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

static void addRecentGame()
{
	addRecentGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName().data());
}
