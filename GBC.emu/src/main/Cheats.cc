/*  This file is part of GBC.emu.

	GBC.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBC.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBC.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/TextEntry.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <main/Cheats.hh>
#include <gambatte.h>
extern gambatte::GB gbEmu;
StaticArrayList<GbcCheat, EmuCheats::MAX> cheatList;
bool cheatsModified = 0;

static bool strIsGGCode(const char *str)
{
	int hex;
	return strlen(str) == 11 &&
		sscanf(str, "%1x%1x%1x-%1x%1x%1x-%1x%1x%1x",
			&hex, &hex, &hex, &hex, &hex, &hex, &hex, &hex, &hex) == 9;
}

static bool strIsGSCode(const char *str)
{
	int hex;
	return strlen(str) == 8 &&
		sscanf(str, "%1x%1x%1x%1x%1x%1x%1x%1x",
			&hex, &hex, &hex, &hex, &hex, &hex, &hex, &hex) == 8;
}

void applyCheats()
{
	if(EmuSystem::gameIsRunning())
	{
		std::string ggCodeStr, gsCodeStr;
		for(auto &e : cheatList)
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

	auto filename = FS::makePathStringPrintf("%s/%s.gbcht", EmuSystem::savePath(), EmuSystem::gameName().data());

	if(!cheatList.size())
	{
		logMsg("deleting cheats file %s", filename.data());
		FS::remove(filename.data());
		cheatsModified = 0;
		return;
	}

	FileIO file;
	file.create(filename.data());
	if(!file)
	{
		logMsg("error creating cheats file %s", filename.data());
		return;
	}
	logMsg("writing cheats file %s", filename.data());

	int version = 0;
	file.write((uint8_t)version);
	file.write((uint16_t)cheatList.size());
	for(auto &e : cheatList)
	{
		file.write((uint8_t)e.flags);
		file.write((uint16_t)strlen(e.name));
		file.write(e.name, strlen(e.name));
		file.write((uint8_t)strlen(e.code));
		file.write(e.code, strlen(e.code));
	}
	cheatsModified = 0;
}

void readCheatFile()
{
	auto filename = FS::makePathStringPrintf("%s/%s.gbcht", EmuSystem::savePath(), EmuSystem::gameName().data());
	FileIO file;
	file.open(filename.data(), IO::AccessHint::ALL);
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file %s", filename.data());

	auto version = file.get<uint8_t>();
	if(version != 0)
	{
		logMsg("skipping due to version code %d", version);
		return;
	}
	auto size = file.get<uint16_t>();
	iterateTimes(size, i)
	{
		if(cheatList.isFull())
		{
			logMsg("cheat list full while reading from file");
			break;
		}
		GbcCheat cheat;
		auto flags = file.get<uint8_t>();
		cheat.flags = flags;
		auto nameLen = file.get<uint16_t>();
		file.read(cheat.name, std::min(uint16_t(sizeof(cheat.name)-1), nameLen));
		auto codeLen = file.get<uint8_t>();
		file.read(cheat.code, std::min(uint8_t(sizeof(cheat.code)-1), codeLen));
		cheatList.push_back(cheat);
	}
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat_, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheat_.name,
		[this](const TableView &)
		{
			return 3;
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return name;
				case 1: return ggCode;
				default: return remove;
			}
		},
		[this](TextMenuItem &, View &, Input::Event)
		{
			IG::eraseFirst(cheatList, *cheat);
			cheatsModified = 1;
			onCheatListChanged();
			applyCheats();
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	ggCode
	{
		"Code",
		cheat_.code,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
				"Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", cheat->code,
				[this](EmuApp &app, auto str)
				{
					if(!strIsGGCode(str) && !strIsGSCode(str))
					{
						app.postMessage(true, "Invalid format");
						postDraw();
						return false;
					}
					string_copy(cheat->code, str);
					string_toUpper(cheat->code);
					cheatsModified = 1;
					applyCheats();
					ggCode.set2ndName(str);
					ggCode.compile(renderer(), projP);
					postDraw();
					return true;
				});
		}
	},
	cheat{&cheat_}
{}

const char *EmuEditCheatView::cheatNameString() const
{
	return cheat->name;
}

void EmuEditCheatView::renamed(const char *str)
{
	string_copy(cheat->name, str);
	cheatsModified = 1;
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addGGGS;
				default: return cheat[idx - 1];
			}
		}
	},
	addGGGS
	{
		"Add Game Genie / GameShark Code", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e,
				"Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(cheatList.isFull())
						{
							app().postMessage(true, "Cheat list is full");
							view.dismiss();
							return 0;
						}
						if(!strIsGGCode(str) && !strIsGSCode(str))
						{
							app().postMessage(true, "Invalid format");
							return 1;
						}
						GbcCheat c;
						string_copy(c.code, str);
						string_toUpper(c.code);
						string_copy(c.name, "Unnamed Cheat");
						cheatList.push_back(c);
						logMsg("added new cheat, %zu total", cheatList.size());
						cheatsModified = 1;
						applyCheats();
						onCheatListChanged();
						view.dismiss();
						app().pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[this](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									string_copy(cheatList.back().name, str);
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
	}
{
	loadCheatItems();
}

void EmuEditCheatListView::loadCheatItems()
{
	unsigned cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, &defaultFace(),
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(cheatList[c], [this](){ onCheatListChanged(); }), e);
			});
		++it;
	}
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void EmuCheatsView::loadCheatItems()
{
	unsigned cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, cIdx)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, &defaultFace(), thisCheat.isOn(),
			[this, cIdx](BoolMenuItem &item, View &, Input::Event e)
			{
				item.flipBoolValue(*this);
				auto &c = cheatList[cIdx];
				c.toggleOn();
				cheatsModified = 1;
				applyCheats();
			});
		logMsg("added cheat %s : %s", thisCheat.name, thisCheat.code);
		++it;
	}
}
