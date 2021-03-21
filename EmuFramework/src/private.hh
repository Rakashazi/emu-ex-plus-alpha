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

#include <imagine/gfx/AnimatedViewport.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include "Recent.hh"
#include <memory>

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

namespace Base
{
class Window;
}

namespace Input
{
class Event;
}

class EmuSystemTask;
class EmuViewController;
class EmuVideo;
class EmuAudio;

struct WindowData
{
	Gfx::Viewport viewport() const { return projection.plane().viewport(); }
	Gfx::Projection projection{};
	Gfx::AnimatedViewport animatedViewport{};
	bool hasEmuView = false;
	bool hasPopup = false;
	bool focused = true;
};

extern DelegateFunc<void ()> onUpdateInputDevices;
extern FS::PathString lastLoadPath;
extern EmuVideo emuVideo;
extern EmuAudio emuAudio;
extern RecentGameList recentGameList;
static constexpr const char *strftimeFormat = "%x  %r";

EmuViewController &emuViewController();
void loadConfigFile(Base::ApplicationContext);
void saveConfigFile(Base::ApplicationContext);
void addRecentGame(const char *fullPath, const char *name);
bool isMenuDismissKey(Input::Event e, EmuViewController &emuViewController);
void applyOSNavStyle(Base::ApplicationContext, bool inGame);
const char *appViewTitle();
bool hasGooglePlayStoreFeatures();
void setCPUNeedsLowLatency(Base::ApplicationContext, bool needed);
void onMainMenuItemOptionChanged();
void runBenchmarkOneShot();
void onSelectFileFromPicker(const char* name, Input::Event e, EmuSystemCreateParams params, ViewAttachParams attachParams);
void launchSystem(bool tryAutoState, bool addToRecent);
Gfx::PixmapTexture &getAsset(Gfx::Renderer &, AssetID);
std::unique_ptr<View> makeEmuView(ViewAttachParams attach, EmuApp::ViewID id);
Gfx::Viewport makeViewport(const Base::Window &win);
Gfx::Projection updateProjection(Gfx::Viewport viewport);
WindowData &windowData(const Base::Window &win);
uint8_t currentFrameInterval();
IG::PixelFormatID optionImageEffectPixelFormatValue();

static void addRecentGame()
{
	addRecentGame(EmuSystem::fullGamePath(), EmuSystem::fullGameName().data());
}
