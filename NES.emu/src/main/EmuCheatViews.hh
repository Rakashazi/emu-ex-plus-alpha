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

class EditCheatsView : public BaseEditCheatsView
{
public:
	EditCheatsView(ViewAttachParams, CheatsView&);

private:
	TextMenuItem addGG, addRAM;
};

class EditCheatView : public BaseEditCheatView
{
public:
	EditCheatView(ViewAttachParams, Cheat&, BaseEditCheatsView&);
	void loadItems();

private:
	TextMenuItem addGG, addRAM;
};

class EditRamCheatView: public TableView, public EmuAppHelper
{
public:
	EditRamCheatView(ViewAttachParams, Cheat&, CheatCode&, EditCheatView&);

private:
	Cheat& cheat;
	CheatCode& code;
	EditCheatView& editCheatView;
	DualTextMenuItem addr, value, comp;
	TextMenuItem remove;
};

}
