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
#include <emuframework/viewUtils.hh>
#include <emuframework/Option.hh>
#include "EmuCheatViews.hh"
#include "MainSystem.hh"
#include <main/Cheats.hh>
#include <gambatte.h>

namespace EmuEx
{

StaticArrayList<GbcCheat, maxCheats> cheatList;

static void writeCheatFile(EmuSystem &);

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

void GbcSystem::applyCheats()
{
	if(!hasContent())
		return;
	std::string ggCodeStr, gsCodeStr;
	for(auto &e : cheatList)
	{
		if(!e.isOn())
			continue;
		std::string &codeStr = std::string_view{e.code}.contains('-') ? ggCodeStr : gsCodeStr;
		if(codeStr.size())
			codeStr += ';';
		codeStr += e.code;
	}
	gbEmu.setGameGenie(ggCodeStr);
	gbEmu.setGameShark(gsCodeStr);
	if(ggCodeStr.size())
		logMsg("set GG codes: %s", ggCodeStr.c_str());
	if(gsCodeStr.size())
		logMsg("set GS codes: %s", gsCodeStr.c_str());
}

void writeCheatFile(EmuSystem &sys_)
{
	auto &sys = static_cast<GbcSystem&>(sys_);
	auto ctx = sys.appContext();
	auto path = sys.userFilePath(sys.cheatsDir, ".gbcht");

	if(!cheatList.size())
	{
		logMsg("deleting cheats file %s", path.data());
		ctx.removeFileUri(path);
		return;
	}

	auto file = ctx.openFileUri(path, OpenFlags::testNewFile());
	if(!file)
	{
		logMsg("error creating cheats file %s", path.data());
		return;
	}
	logMsg("writing cheats file %s", path.data());

	int version = 0;
	file.put(uint8_t(version));
	file.put(int16_t(cheatList.size()));
	for(auto &e : cheatList)
	{
		file.put(uint8_t(e.flags));
		writeSizedData<uint16_t>(file, e.name);
		writeSizedData<uint8_t>(file, e.code);
	}
}

void readCheatFile(EmuSystem &sys_)
{
	auto &sys = static_cast<GbcSystem&>(sys_);
	auto path = sys.userFilePath(sys.cheatsDir, ".gbcht");
	auto file = sys.appContext().openFileUri(path, {.test = true, .accessHint = IOAccessHint::All});
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
	for(auto i : iotaCount(size))
	{
		if(cheatList.isFull())
		{
			logMsg("cheat list full while reading from file");
			break;
		}
		GbcCheat cheat{};
		auto flags = file.get<uint8_t>();
		cheat.flags = flags;
		readSizedData<uint16_t>(file, cheat.name);
		readSizedData<uint8_t>(file, cheat.code);
		cheatList.push_back(cheat);
	}
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat_, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheat_.name,
		items,
		[this](TextMenuItem &, View &, Input::Event)
		{
			IG::eraseFirst(cheatList, *cheat);
			onCheatListChanged();
			writeCheatFile(system());
			static_cast<GbcSystem&>(system()).applyCheats();
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	items{&name, &ggCode, &remove},
	ggCode
	{
		"Code",
		cheat_.code,
		attach,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
				"Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", cheat->code,
				[this](CollectTextInputView&, auto str)
				{
					if(!strIsGGCode(str) && !strIsGSCode(str))
					{
						app().postMessage(true, "Invalid format");
						postDraw();
						return false;
					}
					cheat->code = IG::toUpperCase<decltype(cheat->code)>(str);
					writeCheatFile(system());
					static_cast<GbcSystem&>(app().system()).applyCheats();
					ggCode.set2ndName(str);
					ggCode.place();
					postDraw();
					return true;
				});
		}
	},
	cheat{&cheat_}
{}

std::string_view EmuEditCheatView::cheatNameString() const
{
	return std::string_view{cheat->name};
}

void EmuEditCheatView::renamed(std::string_view str)
{
	cheat->name = str;
	writeCheatFile(system());
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](ItemMessage msg) -> ItemReply
		{
			return msg.visit(overloaded
			{
				[&](const ItemsMessage &m) -> ItemReply { return 1 + cheat.size(); },
				[&](const GetItemMessage &m) -> ItemReply
				{
					switch(m.idx)
					{
						case 0: return &addGGGS;
						default: return &cheat[m.idx - 1];
					}
				},
			});
		}
	},
	addGGGS
	{
		"Add Game Genie / GameShark Code", attach,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectTextInputView(attachParams(), e,
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
						c.code = IG::toUpperCase<decltype(c.code)>(str);
						c.name = "Unnamed Cheat";
						cheatList.push_back(c);
						logMsg("added new cheat, %zu total", cheatList.size());
						static_cast<GbcSystem&>(system()).applyCheats();
						onCheatListChanged();
						writeCheatFile(system());
						view.dismiss();
						pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
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
	auto cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	for(auto c : iotaCount(cheats))
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, attachParams(),
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
	for(auto cIdx : iotaCount(cheats))
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, attachParams(), thisCheat.isOn(),
			[this, cIdx](BoolMenuItem &item, View &, Input::Event e)
			{
				item.flipBoolValue(*this);
				auto &c = cheatList[cIdx];
				c.toggleOn();
				writeCheatFile(system());
				static_cast<GbcSystem&>(system()).applyCheats();
			});
		logMsg("added cheat %s : %s", thisCheat.name.data(), thisCheat.code.data());
		++it;
	}
}

}
