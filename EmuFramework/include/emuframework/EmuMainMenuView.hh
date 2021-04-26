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
#include <array>
#ifdef CONFIG_BLUETOOTH
#include <imagine/bluetooth/sys.hh>
#endif

class EmuVideoLayer;
class EmuAudio;

class EmuMainMenuView : public TableView, public EmuAppHelper<EmuMainMenuView>
{
public:
	EmuMainMenuView(ViewAttachParams attach, bool customMenu = false);
	void onShow() final;
	void loadFileBrowserItems();
	void loadStandardItems();
	void setAudioVideo(EmuAudio &audio, EmuVideoLayer &videoLayer);

	static constexpr unsigned STANDARD_ITEMS = 14;
	static constexpr unsigned MAX_SYSTEM_ITEMS = 5;

protected:
	EmuAudio *audio{};
	EmuVideoLayer *videoLayer{};
	TextMenuItem loadGame;
	TextMenuItem systemActions;
	TextMenuItem recentGames;
	TextMenuItem bundledGames;
	TextMenuItem options;
	TextMenuItem onScreenInputManager;
	TextMenuItem inputManager;
	TextMenuItem benchmark;
	#ifdef CONFIG_BLUETOOTH
	TextMenuItem scanWiimotes;
	TextMenuItem bluetoothDisconnect;
	#endif
	#ifdef CONFIG_BLUETOOTH_SERVER
	TextMenuItem acceptPS3ControllerConnection;
	#endif
	TextMenuItem about;
	TextMenuItem exitApp;
	StaticArrayList<MenuItem*, STANDARD_ITEMS + MAX_SYSTEM_ITEMS> item{};
};
