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

#include <memory>
#include <imagine/base/Base.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/gui/ViewStack.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gfx/AnimatedViewport.hh>
#include <emuframework/EmuInputView.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/Recent.hh>

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

struct AppWindowData
{
	Base::Window win{};
	Gfx::Drawable drawable{};
	Gfx::Viewport viewport() { return projectionPlane.viewport; }
	Gfx::Mat4 projectionMat{};
	Gfx::ProjectionPlane projectionPlane{};
	Gfx::AnimatedViewport animatedViewport{};
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

extern AppWindowData mainWin, extraWin;
extern AppWindowData *emuWin;
extern EmuVideoLayer emuVideoLayer;
extern EmuMenuViewStack viewStack;
extern EmuModalViewStack modalViewController;
extern bool menuViewIsActive;
extern DelegateFunc<void ()> onUpdateInputDevices;
extern FS::PathString lastLoadPath;
extern MsgPopup popup;
extern EmuVideo emuVideo;
extern EmuInputView emuInputView;
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
void setEmuViewOnExtraWindow(bool on);
void startViewportAnimation(AppWindowData &winData);
bool showAutoStateConfirm(Gfx::Renderer &r, Input::Event e, bool addToRecent);
void onMainMenuItemOptionChanged();
void placeEmuViews();
void placeElements();
void runBenchmarkOneShot();
void onSelectFileFromPicker(Gfx::Renderer &r, const char* name, Input::Event e);
void startGameFromMenu();
void closeGame(bool allowAutosaveState = true);
bool handleInputEvent(Base::Window &win, Input::Event e);
void launchSystem(bool tryAutoState, bool addToRecent);
Gfx::PixmapTexture &getAsset(Gfx::Renderer &r, AssetID assetID);
ViewAttachParams emuViewAttachParams();
View *makeView(ViewAttachParams attach, EmuApp::ViewID id);
void updateAndDrawEmuVideo();
void updateEmuAudioStats(uint underruns, uint overruns, uint callbacks, double avgCallbackFrames, uint frames);
void clearEmuAudioStats();

static void addRecentGame()
{
	addRecentGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName().data());
}
