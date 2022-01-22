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
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <imagine/fs/FS.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <gba/Cheats.h>
#include <gba/GBA.h>
static bool cheatsModified = false;

namespace EmuEx
{

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
	strncpy(cheat.desc, str, sizeof(cheat.desc));
}

void EmuEditCheatListView::loadCheatItems()
{
	cheat.clear();
	cheat.reserve(cheatsList.size());
	for(auto &c : cheatsList)
	{
		cheat.emplace_back(c.desc, &defaultFace(),
			[this, idx = std::distance(cheatsList.data(), &c)](Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(idx, [this](){ onCheatListChanged(); }), e);
			});
	}
}

void EmuEditCheatListView::addNewCheat(int isGSv3)
{
	if(cheatsList.size() == cheatsList.capacity())
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
				auto tempStr{IG::stringToUpper<std::string>(str)};
				if(tempStr.size() == 17 && tempStr[8] == ' ')
				{
					logMsg("removing middle space in text");
					tempStr.erase(tempStr.begin() + 8);
				}
				if(isGSv3 ?
					cheatsAddGSACode(gGba.cpu, tempStr.data(), "Unnamed Cheat", true) :
					((tempStr.size() == 16 && cheatsAddGSACode(gGba.cpu, tempStr.data(), "Unnamed Cheat", false))
					|| cheatsAddCBACode(gGba.cpu, tempStr.data(), "Unnamed Cheat")))
				{
					logMsg("added new cheat, %d total", (int)cheatsList.size());
				}
				else
				{
					app().postMessage(true, "Invalid format");
					return true;
				}
				cheatsModified = true;
				cheatsDisable(gGba.cpu, cheatsList.size()-1);
				onCheatListChanged();
				view.dismiss();
				app().pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
					[this](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							auto &cheat = cheatsList[cheatsList.size()-1];
							strncpy(cheat.desc, str, sizeof(cheat.desc));
							onCheatListChanged();
							view.dismiss();
						}
						else
						{
							view.dismiss();
						}
						return false;
					});
			}
			else
			{
				view.dismiss();
			}
			return false;
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
		[this](const TableView &, size_t idx) -> MenuItem&
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
	cheat.clear();
	cheat.reserve(cheatsList.size());
	for(auto &c : cheatsList)
	{
		cheat.emplace_back(c.desc, &defaultFace(), c.enabled,
			[this, idx = std::distance(cheatsList.data(), &c)](BoolMenuItem &item, Input::Event e)
			{
				cheatsModified = true;
				bool on = item.flipBoolValue(*this);
				if(on)
					cheatsEnable(idx);
				else
					cheatsDisable(gGba.cpu, idx);
			});
	}
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void writeCheatFile(IG::ApplicationContext ctx)
{
	if(!cheatsModified)
		return;

	auto filename = EmuSystem::contentSaveFilePath(ctx, ".clt");

	if(!cheatsList.size())
	{
		logMsg("deleting cheats file %s", filename.data());
		ctx.removeFileUri(filename);
		cheatsModified = false;
		return;
	}
	cheatsSaveCheatList(ctx, filename.data());
	cheatsModified = false;
}

void readCheatFile(IG::ApplicationContext ctx)
{
	auto filename = EmuSystem::contentSaveFilePath(ctx, ".clt");
	if(cheatsLoadCheatList(ctx, filename.data()))
	{
		logMsg("loaded cheat file: %s", filename.data());
	}
}

}
