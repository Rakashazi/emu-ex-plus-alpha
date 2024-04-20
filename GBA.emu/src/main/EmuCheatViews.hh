#pragma once

/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <vector>

namespace EmuEx
{

class EmuCheatsView : public BaseCheatsView
{
public:
	EmuCheatsView(ViewAttachParams attach);

private:
	void loadCheatItems() final;
};

class EmuEditCheatListView : public BaseEditCheatListView
{
public:
	EmuEditCheatListView(ViewAttachParams attach);

private:
	TextMenuItem addGS12CBCode{}, addGS3Code{};

	void loadCheatItems() final;
	void addNewCheat(int isGSv3);
};

class EmuEditCheatView : public BaseEditCheatView<EmuEditCheatView>
{
public:
	EmuEditCheatView(ViewAttachParams attach, int cheatIdx, RefreshCheatsDelegate onCheatListChanged_);
	std::string_view cheatNameString() const;
	void renamed(std::string_view);

private:
	std::array<MenuItem*, 3> items;
	DualTextMenuItem code{};
	int idx = 0;
};

}
