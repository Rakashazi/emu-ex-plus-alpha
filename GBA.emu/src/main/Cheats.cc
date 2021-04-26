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

#include <imagine/gui/TextEntry.hh>
#include <imagine/util/string.h>
#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <gba/Cheats.h>
static bool cheatsModified = false;

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, unsigned cheatIdx, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheatsList[cheatIdx].desc,
		[this](const TableView &)
		{
			return 3;
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return name;
				case 1: return code;
				default: return remove;
			}
		},
		[this](TextMenuItem &item, View &parent, Input::Event e)
		{
			cheatsModified = true;
			cheatsDelete(gGba.cpu, idx, true);
			onCheatListChanged();
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	code
	{
		"Code",
		cheatsList[cheatIdx].codestring,
		&defaultFace(),
		[this](DualTextMenuItem &, View &, Input::Event)
		{
			app().postMessage("To change this cheat, please delete and re-add it");
		}
	},
	idx{cheatIdx}
{}

const char *EmuEditCheatView::cheatNameString() const
{
	return cheatsList[idx].desc;
}

void EmuEditCheatView::renamed(const char *str)
{
	cheatsModified = true;
	auto &cheat = cheatsList[idx];
	string_copy(cheat.desc, str);
}

void EmuEditCheatListView::loadCheatItems()
{
	unsigned cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back(cheatsList[c].desc, &defaultFace(),
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(c, [this](){ onCheatListChanged(); }), e);
			});
	}
}

void EmuEditCheatListView::addNewCheat(int isGSv3)
{
	if(cheatsNumber == EmuCheats::MAX)
	{
		app().postMessage(true, "Too many cheats, delete some first");
		return;
	}
	app().pushAndShowNewCollectTextInputView(attachParams(), {},
		isGSv3 ? "Input xxxxxxxx yyyyyyyy" : "Input xxxxxxxx yyyyyyyy (GS) or xxxxxxxx yyyy (AR)", "",
		[this, isGSv3](CollectTextInputView &view, const char *str)
		{
			if(str)
			{
				char tempStr[20];
				string_copy(tempStr, str);
				string_toUpper(tempStr);
				if(strlen(tempStr) == 17 && tempStr[8] == ' ')
				{
					logMsg("removing middle space in text");
					memmove(&tempStr[8], &tempStr[9], 9); // 8 chars + null byte
				}
				if(isGSv3 ?
					cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", true) :
					((strlen(tempStr) == 16 && cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", false))
					|| cheatsAddCBACode(gGba.cpu, tempStr, "Unnamed Cheat")))
				{
					logMsg("added new cheat, %d total", cheatsNumber);
				}
				else
				{
					app().postMessage(true, "Invalid format");
					return 1;
				}
				cheatsModified = true;
				cheatsDisable(gGba.cpu, cheatsNumber-1);
				onCheatListChanged();
				view.dismiss();
				app().pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
					[this](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							string_copy(cheatsList[cheatsNumber-1].desc, str);
							onCheatListChanged();
							view.dismiss();
						}
						else
						{
							view.dismiss();
						}
						return 0;
					});
			}
			else
			{
				view.dismiss();
			}
			return 0;
		});
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 2 + cheat.size();
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addGS12CBCode;
				case 1: return addGS3Code;
				default: return cheat[idx - 2];
			}
		}
	},
	addGS12CBCode
	{
		"Add Game Shark v1-2/Code Breaker Code", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(false);
		}
	},
	addGS3Code
	{
		"Add Game Shark v3 Code", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(true);
		}
	}
{
	loadCheatItems();
}

void EmuCheatsView::loadCheatItems()
{
	unsigned cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		auto &cheatEntry = cheatsList[c];
		cheat.emplace_back(cheatEntry.desc, &defaultFace(), cheatEntry.enabled,
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				cheatsModified = true;
				bool on = item.flipBoolValue(*this);
				if(on)
					cheatsEnable(c);
				else
					cheatsDisable(gGba.cpu, c);
			});
	}
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());

	if(!cheatsNumber)
	{
		logMsg("deleting cheats file %s", filename.data());
		FS::remove(filename.data());
		cheatsModified = false;
		return;
	}
	cheatsSaveCheatList(filename.data());
	cheatsModified = false;
}

void readCheatFile()
{
	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());
	if(cheatsLoadCheatList(filename.data()))
	{
		logMsg("loaded cheat file: %s", filename.data());
	}
}
