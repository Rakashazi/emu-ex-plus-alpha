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
#include <imagine/fs/FS.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
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
			std::string &codeStr = IG::stringContains(e.code, "-") ? ggCodeStr : gsCodeStr;
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

void writeCheatFile(Base::ApplicationContext ctx)
{
	if(!cheatsModified)
		return;

	auto path = EmuSystem::contentSaveFilePath(ctx, ".gbcht");

	if(!cheatList.size())
	{
		logMsg("deleting cheats file %s", path.data());
		ctx.removeFileUri(path);
		cheatsModified = 0;
		return;
	}

	auto file = ctx.openFileUri(path, IO::OPEN_CREATE | IO::OPEN_TEST);
	if(!file)
	{
		logMsg("error creating cheats file %s", path.data());
		return;
	}
	logMsg("writing cheats file %s", path.data());

	int version = 0;
	file.write((uint8_t)version);
	file.write((uint16_t)cheatList.size());
	for(auto &e : cheatList)
	{
		file.write((uint8_t)e.flags);
		file.write((uint16_t)e.name.size());
		file.write(e.name.data(), e.name.size());
		file.write((uint8_t)e.code.size());
		file.write(e.code.data(), e.code.size());
	}
	cheatsModified = 0;
}

void readCheatFile(Base::ApplicationContext ctx)
{
	auto path = EmuSystem::contentSaveFilePath(ctx, ".gbcht");
	auto file = ctx.openFileUri(path, IO::AccessHint::ALL, IO::OPEN_TEST);
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file:%s", path.data());

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
		GbcCheat cheat{};
		auto flags = file.get<uint8_t>();
		cheat.flags = flags;
		auto nameLen = file.get<uint16_t>();
		file.readSized(cheat.name, nameLen);
		auto codeLen = file.get<uint8_t>();
		file.readSized(cheat.code, codeLen);
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
					cheat->code = IG::stringToUpper<decltype(cheat->code)>(str);
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
	return cheat->name.data();
}

void EmuEditCheatView::renamed(const char *str)
{
	cheat->name = str;
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
		[this](const TableView &, size_t idx) -> MenuItem&
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
							return false;
						}
						if(!strIsGGCode(str) && !strIsGSCode(str))
						{
							app().postMessage(true, "Invalid format");
							return true;
						}
						GbcCheat c;
						c.code = IG::stringToUpper<decltype(c.code)>(str);
						c.name = "Unnamed Cheat";
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
									cheatList.back().name = str;
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
		logMsg("added cheat %s : %s", thisCheat.name.data(), thisCheat.code.data());
		++it;
	}
}
