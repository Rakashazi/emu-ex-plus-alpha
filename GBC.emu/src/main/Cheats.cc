#include <Cheats.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <util/gui/ViewStack.hh>
#include "EmuCheatViews.hh"
#include <main/Cheats.hh>
#include <gambatte.h>
extern MsgPopup popup;
extern CollectTextInputView textInputView;
extern ViewStack viewStack;
extern gambatte::GB gbEmu;
StaticDLList<GbcCheat, EmuCheats::MAX> cheatList;
bool cheatsModified = 0;

void applyCheats()
{
	if(EmuSystem::gameIsRunning())
	{
		std::string ggCodeStr, gsCodeStr;
		forEachInDLList(&cheatList, e)
		{
			if(!e.isOn())
				continue;
			std::string &codeStr = strstr(e.code, "-") ? ggCodeStr : gsCodeStr;
			if(codeStr.size())
				codeStr += ";";
			codeStr += e.code;
		}
		gbEmu.setGameGenie(ggCodeStr);
		gbEmu.setGameShark(gsCodeStr);
		if(ggCodeStr.size())
			logMsg("set GG codes: %s", ggCodeStr.c_str());
		if(gsCodeStr.size())
			logMsg("set GS codes: %s", gsCodeStr.c_str());
	}
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	FsSys::cPath filename;
	sprintf(filename, "%s/%s.gbcht", EmuSystem::savePath(), EmuSystem::gameName);

	if(!cheatList.size)
	{
		logMsg("deleting cheats file %s", filename);
		FsSys::remove(filename);
		cheatsModified = 0;
		return;
	}

	auto file = IoSys::create(filename);
	if(!file)
	{
		logMsg("error creating cheats file %s", filename);
		return;
	}
	logMsg("writing cheats file %s", filename);

	int version = 0;
	file->writeVar((uint8)version);
	file->writeVar((uint16)cheatList.size);
	forEachInDLList(&cheatList, e)
	{
		file->writeVar((uint8)e.flags);
		file->writeVar((uint16)strlen(e.name));
		file->fwrite(e.name, strlen(e.name), 1);
		file->writeVar((uint8)strlen(e.code));
		file->fwrite(e.code, strlen(e.code), 1);
	}
	file->close();
	cheatsModified = 0;
}

void readCheatFile()
{
	FsSys::cPath filename;
	sprintf(filename, "%s/%s.gbcht", EmuSystem::savePath(), EmuSystem::gameName);
	auto file = IoSys::open(filename);
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file %s", filename);

	uint8 version = 0;
	file->readVar(version);
	if(version != 0)
	{
		logMsg("skipping due to version code %d", version);
		file->close();
		return;
	}
	uint16 size = 0;
	file->readVar(size);
	iterateTimes(size, i)
	{
		GbcCheat cheat;
		uint8 flags = 0;
		file->readVar(flags);
		cheat.flags = flags;
		uint16 nameLen = 0;
		file->readVar(nameLen);
		file->read(cheat.name, IG::min(uint16(sizeof(cheat.name)-1), nameLen));
		uint8 codeLen = 0;
		file->readVar(codeLen);
		file->read(cheat.code, IG::min(uint8(sizeof(cheat.code)-1), codeLen));
		if(!cheatList.addToEnd(cheat))
		{
			logMsg("cheat list full while reading from file");
			break;
		}
	}
	file->close();
}

uint SystemEditCheatView::handleGgCodeFromTextInput(const char *str)
{
	if(str)
	{
		if(!strIsGGCode(str) && !strIsGSCode(str))
		{
			popup.postError("Invalid format");
			Base::displayNeedsUpdate();
			return 1;
		}
		string_copy(cheat->code, str);
		string_toUpper(cheat->code);
		cheatsModified = 1;
		applyCheats();
		ggCode.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::ggCodeHandler(DualTextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", cheat->code);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleGgCodeFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void SystemEditCheatView::renamed(const char *str)
{
	string_copy(cheat->name, str);
	cheatsModified = 1;
}

void SystemEditCheatView::removed()
{
	cheatList.remove(*cheat);
	cheatsModified = 1;
	refreshCheatViews();
	applyCheats();
}

void SystemEditCheatView::init(bool highlightFirst, GbcCheat &cheat)
{
	this->cheat = &cheat;

	uint i = 0;
	loadNameItem(cheat.name, item, i);
	ggCode.init(cheat.code); item[i++] = &ggCode;
	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

uint EditCheatListView::handleNameFromTextInput(const char *str)
{
	if(str)
	{
		GbcCheat c;
		string_copy(c.name, str);
		if(!cheatList.addToEnd(c))
		{
			logErr("error adding new cheat");
			removeModalView();
			return 0;
		}
		logMsg("added new cheat, %d total", cheatList.size);
		cheatsModified = 1;
		removeModalView();
		refreshCheatViews();
		editCheatView.init(0, *cheatList.last());
		viewStack.pushAndShow(&editCheatView);
	}
	else
	{
		removeModalView();
	}
	return 0;
}

void EditCheatListView::addGGGSHandler(TextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input description");
	textInputView.onTextDelegate().bind<template_mfunc(EditCheatListView, handleNameFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}


void EditCheatListView::cheatSelected(uint idx, const Input::Event &e)
{
	editCheatView.init(!e.isPointer(), *cheatList.index(idx));
	viewStack.pushAndShow(&editCheatView);
}

void EditCheatListView::loadAddCheatItems(MenuItem *item[], uint &items)
{
	addGGGS.init(); item[items++] = &addGGGS;
}

void EditCheatListView::loadCheatItems(MenuItem *item[], uint &items)
{
	int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
	auto it = cheatList.iterator();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = it.obj();
		cheat[c].init(thisCheat.name); item[items++] = &cheat[c];
		it.advance();
	}
}

void CheatsView::cheatSelected(uint idx, const Input::Event &e)
{
	cheat[idx].toggle();
	auto c = cheatList.index(idx);
	c->toggleOn();
	cheatsModified = 1;
	applyCheats();
}

void CheatsView::loadCheatItems(MenuItem *item[], uint &i)
{
	int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
	auto it = cheatList.iterator();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = it.obj();
		cheat[c].init(thisCheat.name, thisCheat.isOn()); item[i++] = &cheat[c];
		logMsg("added cheat %s : %s", thisCheat.name, thisCheat.code);
		it.advance();
	}
}
