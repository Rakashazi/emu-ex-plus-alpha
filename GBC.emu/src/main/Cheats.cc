#include <imagine/gui/TextEntry.hh>
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

	std::error_code ec{};
	int version = 0;
	file.writeVal((uint8)version, &ec);
	file.writeVal((uint16)cheatList.size(), &ec);
	for(auto &e : cheatList)
	{
		file.writeVal((uint8)e.flags, &ec);
		file.writeVal((uint16)strlen(e.name), &ec);
		file.write(e.name, strlen(e.name), &ec);
		file.writeVal((uint8)strlen(e.code), &ec);
		file.write(e.code, strlen(e.code), &ec);
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

	auto version = file.readVal<uint8>();
	if(version != 0)
	{
		logMsg("skipping due to version code %d", version);
		return;
	}
	auto size = file.readVal<uint16>();
	iterateTimes(size, i)
	{
		if(cheatList.isFull())
		{
			logMsg("cheat list full while reading from file");
			break;
		}
		GbcCheat cheat;
		auto flags = file.readVal<uint8>();
		cheat.flags = flags;
		auto nameLen = file.readVal<uint16>();
		file.read(cheat.name, std::min(uint16(sizeof(cheat.name)-1), nameLen));
		auto codeLen = file.readVal<uint8>();
		file.read(cheat.code, std::min(uint8(sizeof(cheat.code)-1), codeLen));
		cheatList.push_back(cheat);
	}
}

void EmuEditCheatView::renamed(const char *str)
{
	string_copy(cheat->name, str);
	cheatsModified = 1;
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheat_.name,
		[this](const TableView &)
		{
			return 3;
		},
		[this](const TableView &, uint idx) -> MenuItem&
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
			cheatList.remove(*cheat);
			cheatsModified = 1;
			EmuApp::refreshCheatViews();
			applyCheats();
			dismiss();
			return true;
		}
	},
	ggCode
	{
		"Code",
		cheat_.code,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e,
				"Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", cheat->code,
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!strIsGGCode(str) && !strIsGSCode(str))
						{
							EmuApp::postMessage(true, "Invalid format");
							window().postDraw();
							return 1;
						}
						string_copy(cheat->code, str);
						string_toUpper(cheat->code);
						cheatsModified = 1;
						applyCheats();
						ggCode.compile(renderer(), projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				});
		}
	},
	cheat{&cheat_}
{}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
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
		"Add Game Genie / GameShark Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e,
				"Input xxxxxxxx (GS) or xxx-xxx-xxx (GG) code", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(cheatList.isFull())
						{
							EmuApp::postMessage(true, "Cheat list is full");
							view.dismiss();
							return 0;
						}
						if(!strIsGGCode(str) && !strIsGSCode(str))
						{
							EmuApp::postMessage(true, "Invalid format");
							return 1;
						}
						GbcCheat c;
						string_copy(c.code, str);
						string_toUpper(c.code);
						string_copy(c.name, "Unnamed Cheat");
						cheatList.push_back(c);
						logMsg("added new cheat, %d total", cheatList.size());
						cheatsModified = 1;
						applyCheats();
						view.dismiss();
						EmuApp::refreshCheatViews();
						EmuApp::pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									string_copy(cheatList.back().name, str);
									view.dismiss();
									EmuApp::refreshCheatViews();
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
	uint cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name,
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(cheatList[c]), e);
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
	uint cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, cIdx)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, thisCheat.isOn(),
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
