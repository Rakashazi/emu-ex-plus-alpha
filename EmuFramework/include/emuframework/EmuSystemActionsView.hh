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

#if defined __ANDROID__
#define CONFIG_EMUFRAMEWORK_ADD_LAUNCHER_ICON
#endif

class EmuSystemActionsView : public TableView, public EmuAppHelper<EmuSystemActionsView>
{
public:
	EmuSystemActionsView(ViewAttachParams attach, bool customMenu = false);
	void onShow() override;
	void loadStandardItems();

	static constexpr unsigned STANDARD_ITEMS = 9;
	static constexpr unsigned MAX_SYSTEM_ITEMS = 6;

protected:
	TextMenuItem cheats;
	TextMenuItem reset;
	TextMenuItem loadState;
	TextMenuItem saveState;
	TextMenuItem stateSlot;
	#ifdef CONFIG_EMUFRAMEWORK_ADD_LAUNCHER_ICON
	TextMenuItem addLauncherIcon;
	#endif
	TextMenuItem screenshot;
	TextMenuItem resetSessionOptions;
	TextMenuItem close;
	StaticArrayList<MenuItem*, STANDARD_ITEMS + MAX_SYSTEM_ITEMS> item{};
};
