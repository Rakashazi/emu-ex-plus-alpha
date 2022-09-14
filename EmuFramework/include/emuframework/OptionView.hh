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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/container/ArrayList.hh>
#include <memory>

namespace IG
{
class TextTableView;
}

namespace EmuEx
{

using namespace IG;

class SystemOptionView : public TableView, public EmuAppHelper<SystemOptionView>
{
public:
	SystemOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem autoSaveStateItem[4];
	MultiChoiceMenuItem autoSaveState;
	BoolMenuItem confirmAutoLoadState;
	BoolMenuItem confirmOverwriteState;
	TextMenuItem fastSlowModeSpeedItem[8];
	MultiChoiceMenuItem fastSlowModeSpeed;
	IG_UseMemberIf(Config::envIsAndroid, BoolMenuItem, performanceMode);
	StaticArrayList<MenuItem*, 24> item;

	TextMenuItem::SelectDelegate setAutoSaveStateDel();
	TextMenuItem::SelectDelegate setFastSlowModeSpeedDel();
};

class FilePathOptionView : public TableView, public EmuAppHelper<FilePathOptionView>
{
public:
	FilePathOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem savePath;
	StaticArrayList<MenuItem*, 6> item;

	void onSavePathChange(std::string_view path);
	virtual bool onFirmwarePathChange(IG::CStringView path, bool isDir);
	std::unique_ptr<TextTableView> makeFirmwarePathMenu(UTF16String name, bool allowFiles = false, int extraItemsHint = 0);

	void pushAndShowFirmwarePathMenu(UTF16Convertible auto &&name, const Input::Event &e, bool allowFiles = false)
	{
		pushAndShow(makeFirmwarePathMenu(IG_forward(name), allowFiles), e);
	}

	void pushAndShowFirmwareFilePathMenu(UTF16Convertible auto &&name, const Input::Event &e)
	{
		pushAndShowFirmwarePathMenu(IG_forward(name), e, true);
	}
};

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
	TextHeadingMenuItem orientationHeading;
	TextMenuItem menuOrientationItem[5];
	MultiChoiceMenuItem menuOrientation;
	TextMenuItem emuOrientationItem[5];
	MultiChoiceMenuItem emuOrientation;
	IG_UseMemberIf(Config::TRANSLUCENT_SYSTEM_UI, BoolMenuItem, layoutBehindSystemUI);
	StaticArrayList<MenuItem*, 22> item;

	TextMenuItem::SelectDelegate setFontSizeDel();
	TextMenuItem::SelectDelegate setMenuOrientationDel();
	TextMenuItem::SelectDelegate setEmuOrientationDel();
	TextMenuItem::SelectDelegate setStatusBarDel();
	TextMenuItem::SelectDelegate setLowProfileOSNavDel();
	TextMenuItem::SelectDelegate setHideOSNavDel();
};

class BiosSelectMenu : public TableView, public EmuAppHelper<BiosSelectMenu>
{
public:
	using BiosChangeDelegate = DelegateFunc<void (std::string_view displayName)>;

	BiosSelectMenu(UTF16String name, ViewAttachParams attach, FS::PathString *biosPathStr, BiosChangeDelegate onBiosChange,
		EmuSystem::NameFilterFunc fsFilter);

protected:
	TextMenuItem selectFile{};
	TextMenuItem unset{};
	BiosChangeDelegate onBiosChangeD{};
	FS::PathString *biosPathStr{};
	EmuSystem::NameFilterFunc fsFilter{};
};

}
