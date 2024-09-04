#pragma once

/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>
#include <vector>
#include "system.h"

namespace EmuEx
{

unsigned decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

class EditCheatsView : public BaseEditCheatsView
{
public:
	EditCheatsView(ViewAttachParams, CheatsView& cheatsView);

private:
	TextMenuItem addCode;
};

class EditCheatView : public BaseEditCheatView
{
public:
	EditCheatView(ViewAttachParams, Cheat& cheat, BaseEditCheatsView& editCheatsView);
	void loadItems();

private:
	TextMenuItem addCode;
};

}
