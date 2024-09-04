#pragma once
#include <emuframework/Cheats.hh>
#include <cheats.h>

namespace EmuEx
{

class EditCheatsView : public BaseEditCheatsView
{
public:
	EditCheatsView(ViewAttachParams, CheatsView&);

private:
	TextMenuItem addCode;
};

class EditCheatView : public BaseEditCheatView
{
public:
	EditCheatView(ViewAttachParams, Cheat&, BaseEditCheatsView&);
	void loadItems();

private:
	#ifndef SNES9X_VERSION_1_4
	TextMenuItem addCode;
	#endif
};

class EditRamCheatView: public TableView, public EmuAppHelper
{
public:
	EditRamCheatView(ViewAttachParams, Cheat&, CheatCode&, EditCheatView&);

private:
	Cheat& cheat;
	CheatCode& code;
	EditCheatView& editCheatView;
	DualTextMenuItem addr, value, conditional;
	TextMenuItem remove;
};

}
