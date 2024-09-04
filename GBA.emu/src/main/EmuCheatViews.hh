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

namespace EmuEx
{

class EditCheatsView : public BaseEditCheatsView
{
public:
	EditCheatsView(ViewAttachParams, CheatsView&);

private:
	TextMenuItem addGS12CBCode, addGS3Code;
};

class EditCheatView : public BaseEditCheatView
{
public:
	EditCheatView(ViewAttachParams, Cheat&, BaseEditCheatsView&);
	void loadItems();

private:
	TextMenuItem addGS12CBCode, addGS3Code;
};

}
