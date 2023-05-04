#pragma once

/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>

namespace EmuEx
{

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() final;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG{}, addRAM{};

	void loadCheatItems() final;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView<EmuEditCheatView>
{
public:
	EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged);
	std::string cheatNameString() const;
	void renamed(std::string_view);

private:
	DualTextMenuItem addr{}, value{}, comp{}, ggCode{};
	unsigned idx = 0;
	int type = 0;
	IG::StaticString<4> addrStr{};
	IG::StaticString<2> valueStr{};
	IG::StaticString<2> compStr{};
	IG::StaticString<8> ggCodeStr{};

	void syncCheat(std::string_view newName = {});
};

}
