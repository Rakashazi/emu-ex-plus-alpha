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
#include <EmuSystem.hh>
#include <EmuView.hh>
#include <MsgPopup.hh>
#include <InputManagerView.hh>
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
#include <VController.hh>
#include <TouchConfigView.hh>
#endif

enum AssetID { ASSET_ARROW, ASSET_CLOSE, ASSET_ACCEPT, ASSET_GAME_ICON, ASSET_MENU, ASSET_FAST_FORWARD };

class EmuNavView : public BasicNavView
{
public:
	constexpr EmuNavView() {}
	void onLeftNavBtn(const Input::Event &e) override;
	void onRightNavBtn(const Input::Event &e) override;
	void draw(const Base::Window &win) override;
};

extern EmuNavView viewNav;
extern EmuView emuView;
extern ViewStack viewStack;
extern BasicViewController modalViewController;
extern Base::Window mainWin, secondWin;
extern MsgPopup popup;
extern const char *launchGame;
extern InputManagerView *imMenu;
extern StackAllocator menuAllocator;
extern bool menuViewIsActive;
#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
extern SysVController vController;
#endif

Gfx::BufferImage &getAsset(AssetID assetID);
Gfx::BufferImage *getCollectTextCloseAsset();
void handleInputEvent(Base::Window &win, const Input::Event &e);
void handleOpenFileCommand(const char *filename);
bool isMenuDismissKey(const Input::Event &e);
void startGameFromMenu();
void restoreMenuFromGame();
void applyOSNavStyle(bool inGame);
void mainInitCommon(int argc, char** argv);
void mainInitWindowCommon(Base::Window &win, const Gfx::LGradientStopDesc *navViewGrad, uint navViewGradSize);
void initMainMenu(Base::Window &win);
View &mainMenu();
View &allocAndGetOptionCategoryMenu(Base::Window &win, const Input::Event &e, StackAllocator &allocator, uint idx);

template <size_t NAV_GRAD_SIZE>
void mainInitWindowCommon(Base::Window &win, const Gfx::LGradientStopDesc (&navViewGrad)[NAV_GRAD_SIZE])
{
	mainInitWindowCommon(win, navViewGrad, NAV_GRAD_SIZE);
}
