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
#include <VController.hh>

void startGameFromMenu();

class OptionCategoryView : public BaseMenuView
{
	TextMenuItem subConfig[5]
	{
		"Video",
		"Audio",
		"Input",
		"System",
		"GUI"
	};

	MenuItem *item[5] = {nullptr};
public:
	constexpr OptionCategoryView(): BaseMenuView("Options") { }

	void init(bool highlightFirst);
	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i);
};

class RecentGameView : public BaseMenuView
{
private:
	TextMenuItem recentGame[10];
	TextMenuItem clear {"Clear List", TextMenuItem::SelectDelegate::create<&clearRecentMenuHandler>()};

	static void clearRecentMenuHandler(TextMenuItem &, const Input::Event &e);

	MenuItem *item[1 + 10 + 1] = {nullptr};
public:
	constexpr RecentGameView(): BaseMenuView("Recent Games") { }

	void init(bool highlightFirst);
};

struct InputPlayerMapMenuItem : public MultiChoiceSelectMenuItem
{
	constexpr InputPlayerMapMenuItem() { }
	uint *player = nullptr;
	void init(const char *name, uint *player)
	{
		this->player = player;
		static const char *str[] = { "1", "2", "3", "4", "5" };
		MultiChoiceSelectMenuItem::init(name, str, *player, EmuSystem::maxPlayers);
	}

	void doSet(int val)
	{
		*player = val;
		vController.updateMapping(*player);
	}
};

class MenuView : public BaseMenuView
{
protected:
	TextMenuItem loadGame {"Load Game", TextMenuItem::SelectDelegate::create<&loadGameHandler>()};
	static void loadGameHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem reset {"Reset", TextMenuItem::SelectDelegate::create<&resetHandler>()};
	static void resetHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem loadState {"Load State", TextMenuItem::SelectDelegate::create<&loadStateHandler>()};
	static void loadStateHandler(TextMenuItem &item, const Input::Event &e);

	TextMenuItem recentGames {"Recent Games", TextMenuItem::SelectDelegate::create<&recentGamesHandler>()};
	static void recentGamesHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem saveState {"Save State", TextMenuItem::SelectDelegate::create<&saveStateHandler>()};
	static void saveStateHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem stateSlot {TextMenuItem::SelectDelegate::create<&stateSlotHandler>()};
	static void stateSlotHandler(TextMenuItem &, const Input::Event &e);
	char stateSlotText[sizeof("State Slot (0)")] = // Can't init with string literal due to GCC bug #43453
			{'S', 't', 'a', 't', 'e', ' ', 'S', 'l', 'o', 't', ' ', '(', '0', ')' };

	TextMenuItem options {"Options", TextMenuItem::SelectDelegate::create<&optionsHandler>()};
	static void optionsHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem inputManager {"Input Device Setup", TextMenuItem::SelectDelegate::create<&inputManagerHandler>()};
	static void inputManagerHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem benchmark {"Benchmark Game", TextMenuItem::SelectDelegate::create<&benchmarkHandler>()};
	static void benchmarkHandler(TextMenuItem &, const Input::Event &e);

	#ifdef CONFIG_BLUETOOTH
	TextMenuItem scanWiimotes {"Scan for Wiimotes/iCP/JS1", TextMenuItem::SelectDelegate::create<&bluetoothScanHandler>()};
	static void bluetoothScanHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem bluetoothDisconnect {"Disconnect Bluetooth", TextMenuItem::SelectDelegate::create<&bluetoothDisconnectHandler>()};
	static void bluetoothDisconnectHandler(TextMenuItem &item, const Input::Event &e);
	#endif

	TextMenuItem about {"About", TextMenuItem::SelectDelegate::create<&aboutHandler>()};
	static void aboutHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem exitApp {"Exit", TextMenuItem::SelectDelegate::create<&exitAppHandler>()};
	static void exitAppHandler(TextMenuItem &, const Input::Event &e);

	TextMenuItem screenshot {"Game Screenshot", TextMenuItem::SelectDelegate::create<&screenshotHandler>()};
	static void screenshotHandler(TextMenuItem &item, const Input::Event &e);

public:
	constexpr MenuView(): BaseMenuView(CONFIG_APP_NAME " " IMAGINE_VERSION) { }

	static const uint STANDARD_ITEMS = 15;
	static const uint MAX_SYSTEM_ITEMS = 2;

	void onShow();
	void loadFileBrowserItems(MenuItem *item[], uint &items);
	void loadStandardItems(MenuItem *item[], uint &items);

protected:
	MenuItem *item[STANDARD_ITEMS + MAX_SYSTEM_ITEMS] {nullptr};

public:
	virtual void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
