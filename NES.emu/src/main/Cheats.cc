#include <Cheats.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <util/gui/ViewStack.hh>
#include "EmuCheatViews.hh"
#include <fceu/driver.h>
extern MsgPopup popup;
extern CollectTextInputView textInputView;
extern ViewStack viewStack;
extern uint fceuCheats;
void EncodeGG(char *str, int a, int v, int c);

void SystemEditCheatView::syncCheat(const char *newName)
{
	if(type)
	{
		int a, v, c;
		if(!FCEUI_DecodeGG(ggCodeStr, &a, &v, &c))
		{
			logWarn("error decoding GG code %s", ggCodeStr);
			a = 0; v = 0; c = -1;
		}
		if(!FCEUI_SetCheat(idx, newName, a, v, c, -1, 1))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
	else
	{
		logMsg("setting comp %d", strlen(compStr) ? (int)strtoul(compStr, nullptr, 16) : -1);
		if(!FCEUI_SetCheat(idx,
				newName, strtoul(addrStr, nullptr, 16), strtoul(valueStr, nullptr, 16),
				strlen(compStr) ? strtoul(compStr, nullptr, 16) : -1, -1, 0))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
}

uint SystemEditCheatView::handleGgCodeFromTextInput(const char *str)
{
	if(str)
	{
		if(strlen(str) != 6 && strlen(str) != 8)
		{
			popup.postError("Invalid, must be 6 or 8 digits");
			Base::displayNeedsUpdate();
			return 1;
		}
		string_copy(ggCodeStr, str, sizeof(ggCodeStr));
		syncCheat();
		ggCode.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::ggCodeHandler(DualTextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input Game Genie code", ggCodeStr);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleGgCodeFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

uint SystemEditCheatView::handleAddrFromTextInput(const char *str)
{
	if(str)
	{
		uint a = strtoul(str, nullptr, 16);
		if(a > 0xFFFF)
		{
			logMsg("addr 0x%X too large", a);
			popup.postError("Invalid input");
			Base::displayNeedsUpdate();
			return 1;
		}
		string_copy(addrStr, a ? str : "0", sizeof(addrStr));
		syncCheat();
		addr.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::addrHandler(DualTextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input 4-digit hex", addrStr);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleAddrFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

uint SystemEditCheatView::handleValFromTextInput(const char *str)
{
	if(str)
	{
		uint a = strtoul(str, nullptr, 16);
		if(a > 0xFF)
		{
			logMsg("val 0x%X too large", a);
			popup.postError("Invalid input");
			Base::displayNeedsUpdate();
			return 1;
		}
		string_copy(valueStr, a ? str : "0", sizeof(valueStr));
		syncCheat();
		value.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::valHandler(DualTextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input 2-digit hex", valueStr);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleValFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

uint SystemEditCheatView::handleCompFromTextInput(const char *str)
{
	if(str)
	{
		if(strlen(str))
		{
			uint a = strtoul(str, nullptr, 16);
			if(a > 0xFF)
			{
				logMsg("val 0x%X too large", a);
				popup.postError("Invalid input");
				Base::displayNeedsUpdate();
				return 1;
			}
			string_copy(compStr, str, sizeof(compStr));
		}
		else
		{
			compStr[0] = 0;
		}
		syncCheat();
		comp.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::compHandler(DualTextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input 2-digit hex or blank", compStr);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleCompFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void SystemEditCheatView::renamed(const char *str)
{
	syncCheat(str);
	FCEUI_GetCheat(idx, &nameStr, nullptr, nullptr, nullptr, nullptr, nullptr);
	name.t.setString(nameStr);
	name.compile();
}

void SystemEditCheatView::removed()
{
	assert(fceuCheats != 0);
	FCEUI_DelCheat(idx);
	fceuCheats--;
	refreshCheatViews();
}

void SystemEditCheatView::init(bool highlightFirst, int cheatIdx)
{
	idx = cheatIdx;
	uint32 a;
	uint8 v;
	int compare;
	int gotCheat = FCEUI_GetCheat(cheatIdx, &nameStr, &a, &v, &compare, 0, &type);
	logMsg("got cheat with addr 0x%.4x val 0x%.2x comp %d", a, v, compare);

	uint i = 0;
	loadNameItem(nameStr, item, i);
	if(type)
	{
		name_ = "Edit Code";
		if(a == 0 && v == 0 && compare == -1)
			ggCodeStr[0] = 0;
		else
			EncodeGG(ggCodeStr, a, v, compare);
		ggCode.init(ggCodeStr); item[i++] = &ggCode;
	}
	else
	{
		name_ = "Edit RAM Patch";
		snprintf(addrStr, sizeof(addrStr), "%x", a);
		addr.init(addrStr); item[i++] = &addr;
		snprintf(valueStr, sizeof(valueStr), "%x", v);
		value.init(valueStr); item[i++] = &value;
		if(compare == -1)
			compStr[0] = 0;
		else
			snprintf(compStr, sizeof(compStr), "%x", compare);
		comp.init(compStr); item[i++] = &comp;
	}
	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

uint EditCheatListView::handleNameFromTextInput(const char *str)
{
	if(str)
	{
		if(!FCEUI_AddCheat(str, 0, 0, -1, addCheatType))
		{
			logErr("error adding new cheat");
			removeModalView();
			return 0;
		}
		fceuCheats++;
		FCEUI_ToggleCheat(fceuCheats-1);
		logMsg("added new cheat, %d total", fceuCheats);
		removeModalView();
		refreshCheatViews();
		editCheatView.init(0, fceuCheats-1);
		viewStack.pushAndShow(&editCheatView);
	}
	else
	{
		removeModalView();
	}
	return 0;
}

void EditCheatListView::addGGHandler(TextMenuItem &item, const Input::Event &e)
{
	addCheatType = 1;
	textInputView.init("Input description");
	textInputView.onTextDelegate().bind<template_mfunc(EditCheatListView, handleNameFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void EditCheatListView::addRAMHandler(TextMenuItem &item, const Input::Event &e)
{
	addCheatType = 0;
	textInputView.init("Input description");
	textInputView.onTextDelegate().bind<template_mfunc(EditCheatListView, handleNameFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void EditCheatListView::cheatSelected(uint idx, const Input::Event &e)
{
	editCheatView.init(!e.isPointer(), idx);
	viewStack.pushAndShow(&editCheatView);
}

void EditCheatListView::loadAddCheatItems(MenuItem *item[], uint &items)
{
	addGG.init(); item[items++] = &addGG;
	addRAM.init(); item[items++] = &addRAM;
}

void EditCheatListView::loadCheatItems(MenuItem *item[], uint &items)
{
	int cheats = IG::min(fceuCheats, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		char *name;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, 0, 0);
		assert(gotCheat);
		if(!gotCheat) continue;
		cheat[c].init(name); item[items++] = &cheat[c];
	}
}

void CheatsView::cheatSelected(uint idx, const Input::Event &e)
{
	uint32 a;
	uint8 v;
	int compare, type;
	FCEUI_GetCheat(idx, nullptr, &a, &v, &compare, 0, &type);
	if(!cheat[idx].on && type && a == 0 && v == 0 && compare == -1)
	{
		// Don't turn on null Game Genie codes
		popup.postError("Game Genie code isn't set", 2);
		return;
	}
	cheat[idx].toggle();
	FCEUI_ToggleCheat(idx);
}

void CheatsView::loadCheatItems(MenuItem *item[], uint &i)
{
	int cheats = IG::min(fceuCheats, (uint)sizeofArray(cheat));
	iterateTimes(cheats, c)
	{
		char *name;
		int status;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, &status, 0);
		assert(gotCheat);
		if(!gotCheat) continue;
		cheat[c].init(name, status); item[i++] = &cheat[c];
	}
}
