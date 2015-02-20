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

#include <imagine/base/Base.hh>
#include <imagine/input/Input.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/gui/ViewStack.hh>
#include <imagine/fs/WorkDirStack.hh>
#include <imagine/gfx/AnimatedViewport.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuView.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/InputManagerView.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <emuframework/VController.hh>
#include <emuframework/TouchConfigView.hh>
#endif

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

class EmuNavView : public BasicNavView
{
public:
	constexpr EmuNavView() {}
	void onLeftNavBtn(const Input::Event &e) override;
	void onRightNavBtn(const Input::Event &e) override;
	void draw(const Base::Window &win, const Gfx::ProjectionPlane &projP) override;
};

struct AppWindowData
{
	Base::Window win;
	Gfx::Viewport viewport() { return projectionPlane.viewport; }
	Gfx::Mat4 projectionMat;
	Gfx::ProjectionPlane projectionPlane;
	Gfx::AnimatedViewport animatedViewport;
	bool focused = true;

	constexpr AppWindowData() {};
};

extern AppWindowData mainWin, extraWin;
extern AppWindowData *emuWin;
extern EmuNavView viewNav;
extern EmuVideo emuVideo;
extern EmuVideoLayer emuVideoLayer;
extern ViewStack viewStack;
extern BasicViewController modalViewController;
extern MsgPopup popup;
extern const char *launchGame;
extern DelegateFunc<void ()> onUpdateInputDevices;
extern bool menuViewIsActive;
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern SysVController vController;
#endif
extern WorkDirStack<1> workDirStack;

Gfx::PixmapTexture &getAsset(AssetID assetID);
Gfx::PixmapTexture *getCollectTextCloseAsset();
void handleInputEvent(Base::Window &win, const Input::Event &e);
void handleOpenFileCommand(const char *filename);
bool isMenuDismissKey(const Input::Event &e);
void startGameFromMenu();
void restoreMenuFromGame();
void closeGame(bool allowAutosaveState = true);
void applyOSNavStyle(bool inGame);
using MenuShownDelegate = DelegateFunc<void (Base::Window &win)>;
void mainInitCommon(int argc, char** argv, const Gfx::LGradientStopDesc *navViewGrad, uint navViewGradSize, MenuShownDelegate menuShownDel);
void initMainMenu(Base::Window &win);
View &mainMenu();
View *makeOptionCategoryMenu(Base::Window &win, const Input::Event &e, uint idx);
View *makeEditCheatListView(Base::Window &win, const Input::Event &e);
const char *appViewTitle();
const char *appName();
const char *appID();
void setEmuViewOnExtraWindow(bool on);
void placeEmuViews();
void placeElements();
void startViewportAnimation(AppWindowData &winData);
void updateAndDrawEmuVideo();

template <size_t NAV_GRAD_SIZE>
void mainInitCommon(int argc, char** argv, const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE], MenuShownDelegate menuShownDel)
{
	mainInitCommon(argc, argv, navViewGrad, NAV_GRAD_SIZE, menuShownDel);
}

template <size_t NAV_GRAD_SIZE>
void mainInitCommon(int argc, char** argv, const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE])
{
	mainInitCommon(argc, argv, navViewGrad, NAV_GRAD_SIZE, {});
}
