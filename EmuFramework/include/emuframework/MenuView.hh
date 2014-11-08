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

#include <imagine/gui/TableView.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/StateSlotView.hh>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#include <imagine/bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <imagine/config/version.h>
#include <emuframework/MultiChoiceView.hh>
#include <emuframework/VController.hh>

class OptionCategoryView : public TableView
{
	TextMenuItem subConfig[5]
	{
		"Video",
		"Audio",
		"Input",
		"System",
		"GUI"
	};
	MenuItem *item[5] {nullptr};

public:
	OptionCategoryView(Base::Window &win): TableView{"Options", win} {}
	void init(bool highlightFirst);
};

class RecentGameView : public TableView
{
private:
	TextMenuItem recentGame[10];
	TextMenuItem clear;
	MenuItem *item[1 + 10 + 1]{};

public:
	RecentGameView(Base::Window &win);
	void init(bool highlightFirst);
};

class MenuView : public TableView
{
public:
	MenuView(Base::Window &win);
	void onShow() override;
	void loadFileBrowserItems(MenuItem *item[], uint &items);
	void loadStandardItems(MenuItem *item[], uint &items);
	virtual void init(bool highlightFirst);

	static const uint STANDARD_ITEMS = 19;
	static const uint MAX_SYSTEM_ITEMS = 3;

protected:
	TextMenuItem loadGame;
	TextMenuItem reset;
	TextMenuItem loadState;
	TextMenuItem recentGames;
	TextMenuItem bundledGames;
	TextMenuItem saveState;
	TextMenuItem stateSlot;
	char stateSlotText[sizeof("State Slot (0)")];
	TextMenuItem options;
	TextMenuItem onScreenInputManager;
	TextMenuItem inputManager;
	TextMenuItem benchmark;
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	TextMenuItem addLauncherIcon;
	#endif
	#ifdef CONFIG_BLUETOOTH
	TextMenuItem scanWiimotes;
	TextMenuItem bluetoothDisconnect;
	#endif
	#ifdef CONFIG_BLUETOOTH_SERVER
	TextMenuItem acceptPS3ControllerConnection;
	#endif
	TextMenuItem about;
	TextMenuItem exitApp;
	TextMenuItem screenshot;
	MenuItem *item[STANDARD_ITEMS + MAX_SYSTEM_ITEMS]{};
};
