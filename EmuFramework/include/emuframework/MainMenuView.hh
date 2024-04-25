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
#include <imagine/bluetooth/defs.hh>
#include <array>

namespace EmuEx
{

using namespace IG;
class EmuVideoLayer;
class EmuAudio;

class MainMenuView : public TableView, public EmuAppHelper
{
public:
	MainMenuView(ViewAttachParams attach, bool customMenu = false);
	void onShow() final;
	void loadFileBrowserItems();
	void loadStandardItems();
	virtual void reloadItems();

	static constexpr int STANDARD_ITEMS = 15;
	static constexpr int MAX_SYSTEM_ITEMS = 5;

protected:
	TextMenuItem loadGame;
	TextMenuItem systemActions;
	TextMenuItem recentGames;
	TextMenuItem bundledGames;
	TextMenuItem options;
	TextMenuItem onScreenInputManager;
	TextMenuItem inputManager;
	TextMenuItem benchmark;
	ConditionalMember<Config::Input::BLUETOOTH, TextMenuItem> scanWiimotes;
	ConditionalMember<Config::Input::BLUETOOTH, TextMenuItem> bluetoothDisconnect;
	ConditionalMember<Config::Bluetooth::server, TextMenuItem> acceptPS3ControllerConnection;
	TextMenuItem about;
	TextMenuItem exitApp;
	StaticArrayList<MenuItem*, STANDARD_ITEMS + MAX_SYSTEM_ITEMS> item;
};

}
