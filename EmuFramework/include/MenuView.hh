/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <util/gui/BaseMenuView.hh>
#include "EmuSystem.hh"
#include <StateSlotView.hh>
#include <meta.h>
#ifdef CONFIG_BLUETOOTH
	#include <bluetooth/sys.hh>
	#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
#include <config/version.h>
#include <MultiChoiceView.hh>

void startGameFromMenu();

class OptionCategoryView : public BaseMenuView
{
	TextMenuItem subConfig[5];

	MenuItem *item[5] = {nullptr};
public:
	constexpr OptionCategoryView(): BaseMenuView("Options") { }

	void init(bool highlightFirst);
	void onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i);
};

class RecentGameView : public BaseMenuView
{
private:
	TextMenuItem recentGame[10];
	TextMenuItem clear;

	static void clearRecentMenuHandler(TextMenuItem &, const InputEvent &e);

	MenuItem *item[1 + 10 + 1] = {nullptr};
public:
	constexpr RecentGameView(): BaseMenuView("Recent Games") { }

	void init(bool highlightFirst);
};

class InputPlayerMapView : public BaseMenuView
{
private:
	static const uint numDevs = 5 // key
	+ 1 // pointer
	+ 1 // iCade
	#ifdef CONFIG_BASE_PS3
	+ 5
	#endif
	#ifdef CONFIG_BLUETOOTH
	+ (Bluetooth::maxGamepadsPerTypeStorage*3)
	#endif
	;

	struct InputPlayerMapMenuItem : public MultiChoiceSelectMenuItem
	{
		constexpr InputPlayerMapMenuItem(): player(0) { }
		uint *player;
		void init(const char *name, uint *player)
		{
			this->player = player;
			static const char *str[] = { "1", "2", "3", "4", "5" };
			MultiChoiceSelectMenuItem::init(name, str, *player, EmuSystem::maxPlayers);
		}

		void doSet(int val)
		{
			*player = val;
		}
	} inputMap[numDevs];

	MenuItem *item[numDevs + 1] = {nullptr};
public:
	constexpr InputPlayerMapView(): BaseMenuView("Input/Player Mapping") { }

	void init(bool highlightFirst);
};

// TODO: refactor contents of ifdef block
#ifndef PROTOTYPES_ONLY

void OptionCategoryView::onSelectElement(const GuiTable1D *table, const InputEvent &e, uint i)
{
	oCategoryMenu.init(i, !e.isPointer());
	viewStack.pushAndShow(&oCategoryMenu);
}

OptionCategoryView oMenu;

StateSlotView ssMenu;

RecentGameView rMenu;

InputPlayerMapView ipmMenu;

#endif

class MenuView : public BaseMenuView
{
protected:
	TextMenuItem loadGame;

	TextMenuItem reset;

	TextMenuItem loadState;

	TextMenuItem recentGames;

	TextMenuItem saveState;

	TextMenuItem stateSlot;

	char stateSlotText[sizeof("State Slot (0)")] = {0};

	TextMenuItem options;

	TextMenuItem inputPlayerMap;

	TextMenuItem benchmark;

	#ifdef CONFIG_BLUETOOTH
	TextMenuItem scanWiimotes;

	TextMenuItem bluetoothDisconnect;
	#endif

	TextMenuItem about;

	TextMenuItem exitApp;

	TextMenuItem screenshot;

public:
	constexpr MenuView(): BaseMenuView(CONFIG_APP_NAME " " IMAGINE_VERSION) { }

	static const uint STANDARD_ITEMS = 14;

	void onShow();
	void loadFileBrowserItems(MenuItem *item[], uint &items);
	void loadStandardItems(MenuItem *item[], uint &items);
};
