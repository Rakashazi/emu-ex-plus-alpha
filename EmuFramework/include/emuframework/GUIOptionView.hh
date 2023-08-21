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

#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

using namespace IG;

class GUIOptionView : public TableView, public EmuAppHelper<GUIOptionView>
{
public:
	GUIOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	BoolMenuItem pauseUnfocused;
	TextMenuItem fontSizeItem[10];
	MultiChoiceMenuItem fontSize;
	BoolMenuItem notificationIcon;
	IG_UseMemberIf(Config::STATUS_BAR, TextMenuItem, statusBarItem[3]);
	IG_UseMemberIf(Config::STATUS_BAR, MultiChoiceMenuItem, statusBar);
	IG_UseMemberIf(Config::NAVIGATION_BAR, TextMenuItem, lowProfileOSNavItem[3]);
	IG_UseMemberIf(Config::NAVIGATION_BAR, MultiChoiceMenuItem, lowProfileOSNav);
	IG_UseMemberIf(Config::NAVIGATION_BAR, TextMenuItem, hideOSNavItem[3]);
	IG_UseMemberIf(Config::NAVIGATION_BAR, MultiChoiceMenuItem, hideOSNav);
	BoolMenuItem idleDisplayPowerSave;
	IG_UseMemberIf(CAN_HIDE_TITLE_BAR, BoolMenuItem, navView);
	BoolMenuItem backNav;
	BoolMenuItem systemActionsIsDefaultMenu;
	BoolMenuItem showBundledGames;
	BoolMenuItem showBluetoothScan;
	BoolMenuItem showHiddenFiles;
	DualTextMenuItem maxRecentContent;
	TextHeadingMenuItem orientationHeading;
	TextMenuItem menuOrientationItem[5];
	MultiChoiceMenuItem menuOrientation;
	TextMenuItem emuOrientationItem[5];
	MultiChoiceMenuItem emuOrientation;
	IG_UseMemberIf(Config::TRANSLUCENT_SYSTEM_UI, BoolMenuItem, layoutBehindSystemUI);
	IG_UseMemberIf(Config::freeformWindows, TextMenuItem, setWindowSize);
	IG_UseMemberIf(Config::freeformWindows, TextMenuItem, toggleFullScreen);
	StaticArrayList<MenuItem*, 23> item;
};

}
