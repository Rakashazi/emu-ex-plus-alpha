/*  This file is part of NEO.emu.

	NEO.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NEO.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NEO.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/ScopeGuard.hh>
#include "internal.hh"

extern "C"
{
	#include <gngeo/resfile.h>
	#include <gngeo/conf.h>
	#include <gngeo/emu.h>
	#include <gngeo/fileio.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
}

class ConsoleOptionView : public TableView
{
	TextMenuItem timerItem[3]
	{
		{"Off", &defaultFace(), [](){ setTimerInt(0); }},
		{"On", &defaultFace(), [](){ setTimerInt(1); }},
		{"Auto", &defaultFace(), [](){ setTimerInt(2); }},
	};

	static void setTimerInt(int val)
	{
		EmuSystem::sessionOptionSet();
		optionTimerInt = val;
		setTimerIntOption();
	}

	MultiChoiceMenuItem timer
	{
		"Emulate Timer", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 2)
			{
				t.setString(conf.raster ? "On" : "Off");
				return true;
			}
			else
				return false;
		},
		std::min((int)optionTimerInt, 2),
		timerItem
	};

	std::array<MenuItem*, 1> menuItem
	{
		&timer
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemOptionView : public SystemOptionView
{
private:
	TextMenuItem regionItem[4]
	{
		{"Japan", &defaultFace(), [](){ conf.country = CTY_JAPAN; optionMVSCountry = conf.country; }},
		{"Europe", &defaultFace(), [](){ conf.country = CTY_EUROPE; optionMVSCountry = conf.country; }},
		{"USA", &defaultFace(), [](){ conf.country = CTY_USA; optionMVSCountry = conf.country; }},
		{"Asia", &defaultFace(), [](){ conf.country = CTY_ASIA; optionMVSCountry = conf.country; }},
	};

	MultiChoiceMenuItem region
	{
		"MVS Region", &defaultFace(),
		std::min((int)conf.country, 3),
		regionItem
	};

	TextMenuItem biosItem[7]
	{
		{"Unibios 2.3", &defaultFace(), [](){ conf.system = SYS_UNIBIOS; optionBIOSType = conf.system; }},
		{"Unibios 3.0", &defaultFace(), [](){ conf.system = SYS_UNIBIOS_3_0; optionBIOSType = conf.system; }},
		{"Unibios 3.1", &defaultFace(), [](){ conf.system = SYS_UNIBIOS_3_1; optionBIOSType = conf.system; }},
		{"Unibios 3.2", &defaultFace(), [](){ conf.system = SYS_UNIBIOS_3_2; optionBIOSType = conf.system; }},
		{"Unibios 3.3", &defaultFace(), [](){ conf.system = SYS_UNIBIOS_3_3; optionBIOSType = conf.system; }},
		{"Unibios 4.0", &defaultFace(), [](){ conf.system = SYS_UNIBIOS_4_0; optionBIOSType = conf.system; }},
		{"MVS", &defaultFace(), [](){ conf.system = SYS_ARCADE; optionBIOSType = conf.system; }},
	};

	MultiChoiceMenuItem bios
	{
		"BIOS Type", &defaultFace(),
		[]()
		{
			switch(conf.system)
			{
				default: return 0;
				case SYS_UNIBIOS_3_0: return 1;
				case SYS_UNIBIOS_3_1: return 2;
				case SYS_UNIBIOS_3_2: return 3;
				case SYS_UNIBIOS_3_3: return 4;
				case SYS_UNIBIOS_4_0: return 5;
				case SYS_ARCADE: return 6;
			}
		}(),
		biosItem
	};

	BoolMenuItem createAndUseCache
	{
		"Make/Use Cache Files", &defaultFace(),
		(bool)optionCreateAndUseCache,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionCreateAndUseCache = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem strictROMChecking
	{
		"Strict ROM Checking", &defaultFace(),
		(bool)optionStrictROMChecking,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionStrictROMChecking = item.flipBoolValue(*this);
		}
	};

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&bios);
		item.emplace_back(&region);
		item.emplace_back(&createAndUseCache);
		item.emplace_back(&strictROMChecking);
	}
};

class EmuGUIOptionView : public GUIOptionView
{
	BoolMenuItem listAll
	{
		"List All Games", &defaultFace(),
		(bool)optionListAllGames,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionListAllGames = item.flipBoolValue(*this);
		}
	};

public:
	EmuGUIOptionView(ViewAttachParams attach): GUIOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&listAll);
	}
};

struct RomListEntry
{
	const char* name;
	unsigned bugs;
};

static const RomListEntry romlist[]
{
	{ "2020bb", 0 },
	{ "2020bba", 0 },
	{ "2020bbh", 0 },
	{ "3countb", 0 },
	{ "alpham2", 0 },
	{ "androdun", 0 },
	{ "aodk", 0 },
	{ "aof", 0 },
	{ "aof2", 0 },
	{ "aof2a", 0 },
	{ "aof3", 0 },
	{ "aof3k", 0 },
	{ "bakatono", 0 },
	{ "bangbead", 0 },
	{ "bbbuster", 0 },
	{ "bjourney", 0 },
	{ "blazstar", 0 },
	{ "breakers", 0 },
	{ "breakrev", 0 },
	{ "bstars", 0 },
	{ "bstars2", 0 },
	{ "burningf", 0 },
	{ "burningfh", 0 },
	{ "crsword", 0 },
	{ "ct2k3sa", 1 },
	{ "ct2k3sp", 1 },
	{ "cthd2003", 1 },
	{ "ctomaday", 0 },
	{ "cyberlip", 0 },
	{ "diggerma", 0 },
	{ "doubledr", 0 },
	{ "eightman", 0 },
	{ "fatfursa", 0 },
	{ "fatfursp", 0 },
	{ "fatfury1", 0 },
	{ "fatfury2", 1 },
	{ "fatfury3", 0 },
	{ "fbfrenzy", 0 },
	{ "fightfev", 0 },
	{ "fightfeva", 0 },
	{ "flipshot", 0 },
	{ "fswords", 0 },
	{ "galaxyfg", 0 },
	{ "ganryu", 0 },
	{ "garou", 0 },
	{ "garoubl", 0 },
	{ "garouo", 0 },
	{ "garoup", 0 },
	{ "ghostlop", 0 },
	{ "goalx3", 0 },
	{ "gowcaizr", 0 },
	{ "gpilots", 0 },
	{ "gpilotsh", 0 },
	{ "gururin", 0 },
	{ "ironclad", 0 },
	{ "ironclado", 0 },
	{ "irrmaze", 1 },
	{ "janshin", 0 },
	{ "jockeygp", 1 },
	{ "joyjoy", 0 },
	{ "kabukikl", 0 },
	{ "karnovr", 0 },
	{ "kf10thep", 1 },
	{ "kf2k2mp", 1 },
	{ "kf2k2mp2", 1 },
	{ "kf2k2pla", 1 },
	{ "kf2k2pls", 1 },
	{ "kf2k3bl", 1 },
	{ "kf2k3bla", 1 },
	{ "kf2k3pcb", 1 },
	{ "kf2k3pl", 1 },
	{ "kf2k3upl", 1 },
	{ "kf2k5uni", 1 },
	{ "kizuna", 0 },
	{ "kof10th", 1 },
	{ "kof2000", 1 },
	{ "kof2000n", 1 },
	{ "kof2001", 1 },
	{ "kof2001h", 1 },
	{ "kof2002", 1 },
	{ "kof2002b", 1 },
	{ "kof2003", 1 },
	{ "kof2003h", 1 },
	{ "kof2k4se", 1 },
	{ "kof94", 0 },
	{ "kof95", 0 },
	{ "kof95h", 0 },
	{ "kof96", 0 },
	{ "kof96h", 0 },
	{ "kof97", 0 },
	{ "kof97a", 0 },
	{ "kof97pls", 0 },
	{ "kof98", 0 },
	{ "kof98k", 0 },
	{ "kof98n", 0 },
	{ "kof99", 0 },
	{ "kof99a", 0 },
	{ "kof99e", 0 },
	{ "kof99n", 0 },
	{ "kof99p", 0 },
	{ "kog", 1 },
	{ "kotm", 0 },
	{ "kotm2", 0 },
	{ "kotmh", 0 },
	{ "lans2004", 1 },
	{ "lastblad", 0 },
	{ "lastbladh", 0 },
	{ "lastbld2", 0 },
	{ "lastsold", 0 },
	{ "lbowling", 0 },
	{ "legendos", 0 },
	{ "lresort", 0 },
	{ "magdrop2", 0 },
	{ "magdrop3", 0 },
	{ "maglord", 0 },
	{ "maglordh", 0 },
	{ "mahretsu", 0 },
	{ "marukodq", 0 },
	{ "matrim", 0 },
	{ "matrimbl", 1 },
	{ "miexchng", 0 },
	{ "minasan", 0 },
	{ "mosyougi", 0 },
	{ "ms4plus", 1 },
	{ "ms5pcb", 1 },
	{ "ms5plus", 1 },
	{ "mslug", 0 },
	{ "mslug2", 0 },
	{ "mslug3", 0 },
	{ "mslug3b6", 0 },
	{ "mslug3h", 0 },
	{ "mslug3n", 0 },
	{ "mslug4", 0 },
	{ "mslug5", 1 },
	{ "mslug5h", 1 },
	{ "mslugx", 0 },
	{ "mutnat", 0 },
	{ "nam1975", 0 },
	{ "ncombat", 0 },
	{ "ncombath", 0 },
	{ "ncommand", 0 },
	{ "neobombe", 0 },
	{ "neocup98", 0 },
	{ "neodrift", 0 },
	{ "neomrdo", 0 },
	{ "ninjamas", 0 },
	{ "nitd", 0 },
	{ "nitdbl", 0 },
	{ "overtop", 0 },
	{ "panicbom", 0 },
	{ "pbobbl2n", 0 },
	{ "pbobblen", 0 },
	{ "pbobblena", 0 },
	{ "pgoal", 0 },
	{ "pnyaa", 1 },
	{ "popbounc", 0 },
	{ "preisle2", 1 },
	{ "pspikes2", 0 },
	{ "pulstar", 0 },
	{ "puzzldpr", 0 },
	{ "puzzledp", 0 },
	{ "quizdai2", 0 },
	{ "quizdais", 0 },
	{ "quizkof", 0 },
	{ "ragnagrd", 0 },
	{ "rbff1", 0 },
	{ "rbff1a", 0 },
	{ "rbff2", 0 },
	{ "rbff2h", 0 },
	{ "rbff2k", 0 },
	{ "rbffspec", 0 },
	{ "ridhero", 1 },
	{ "ridheroh", 1 },
	{ "roboarmy", 0 },
	{ "rotd", 1 },
	{ "s1945p", 1 },
	{ "samsh5sp", 0 },
	{ "samsh5sph", 0 },
	{ "samsh5spn", 0 },
	{ "samsho", 0 },
	{ "samsho2", 0 },
	{ "samsho3", 0 },
	{ "samsho3h", 0 },
	{ "samsho4", 0 },
	{ "samsho5", 1 },
	{ "samsho5b", 1 },
	{ "samsho5h", 1 },
	{ "samshoh", 0 },
	{ "savagere", 0 },
	{ "sdodgeb", 0 },
	{ "sengokh", 0 },
	{ "sengoku", 0 },
	{ "sengoku2", 0 },
	{ "sengoku3", 1 },
	{ "shocktr2", 0 },
	{ "shocktra", 0 },
	{ "shocktro", 0 },
	{ "socbrawl", 0 },
	{ "socbrawla", 0 },
	{ "sonicwi2", 0 },
	{ "sonicwi3", 0 },
	{ "spinmast", 0 },
	{ "ssideki", 0 },
	{ "ssideki2", 0 },
	{ "ssideki3", 0 },
	{ "ssideki4", 0 },
	{ "stakwin", 0 },
	{ "stakwin2", 0 },
	{ "strhoop", 0 },
	{ "superspy", 0 },
	{ "svc", 1 },
	{ "svcboot", 1 },
	{ "svcpcb", 1 },
	{ "svcpcba", 1 },
	{ "svcplus", 1 },
	{ "svcplusa", 1 },
	{ "svcsplus", 1 },
	{ "tophuntr", 0 },
	{ "tophuntra", 0 },
	{ "tpgolf", 0 },
	{ "trally", 0 },
	{ "totcarib", 0 },
	{ "turfmast", 0 },
	{ "twinspri", 0 },
	{ "tws96", 0 },
	{ "viewpoin", 0	},
	{ "vliner", 1 },
	{ "vlinero", 1 },
	{ "wakuwak7", 0 },
	{ "wh1", 0 },
	{ "wh1h", 0 },
	{ "wh1ha", 0 },
	{ "wh2", 0 },
	{ "wh2j", 0 },
	{ "wh2jh", 0 },
	{ "whp", 0 },
	{ "wjammers", 0 },
	{ "zedblade", 0 },
	{ "zintrckb", 0 },
	{ "zupapa", 0 },
};

static FS::PathString gameFilePath(EmuApp &app, const char *name)
{
	auto path = app.mediaSearchPath();
	auto zipPath = FS::makePathStringPrintf("%s/%s.zip", path.data(), name);
	if(FS::exists(zipPath))
		return zipPath;
	auto sZipPath = FS::makePathStringPrintf("%s/%s.7z", path.data(), name);
	if(FS::exists(sZipPath))
		return sZipPath;
	auto rarPath = FS::makePathStringPrintf("%s/%s.rar", path.data(), name);
	if(FS::exists(rarPath))
		return rarPath;
	return {};
}

static bool gameFileExists(EmuApp &app, const char *name)
{
	return strlen(gameFilePath(app, name).data());
}

class GameListView : public TableView, public EmuAppHelper<GameListView>
{
private:
	std::vector<TextMenuItem> item{};

	void loadGame(const RomListEntry &entry, Input::Event e)
	{
		app().createSystemWithMedia({}, gameFilePath(app(), entry.name).data(), "", e, {}, attachParams(),
			[this](Input::Event e)
			{
				app().launchSystemWithResumePrompt(e, true);
			});
	}

public:
	GameListView(ViewAttachParams attach):
		TableView
		{
			"Game List",
			attach,
			item
		}
	{
		for(const auto &entry : romlist)
		{
			auto ctx = appContext();
			ROM_DEF *drv = res_load_drv(&ctx, entry.name);
			if(!drv)
				continue;
			auto freeDrv = IG::scopeGuard([&](){ free(drv); });
			bool fileExists = gameFileExists(app(), drv->name);
			if(!optionListAllGames && !fileExists)
			{
				continue;
			}
			item.emplace_back(drv->longname, &defaultFace(),
				[this, &entry](TextMenuItem &item, View &, Input::Event e)
				{
					if(item.active())
					{
						if(entry.bugs)
						{
							auto ynAlertView = makeView<YesNoAlertView>(
								"This game doesn't yet work properly, load anyway?");
							ynAlertView->setOnYes(
								[this, &entry](Input::Event e)
								{
									loadGame(entry, e);
								});
							app().pushAndShowModalView(std::move(ynAlertView), e);
						}
						else
						{
							loadGame(entry, e);
						}
					}
					else
					{
						app().printfMessage(3, 1, "%s not present", entry.name);
					}
					return true;
				});
			item.back().setActive(fileExists);
		}
	}

	int games()
	{
		return item.size();
	}
};

class UnibiosSwitchesView : public TableView
{
	TextMenuItem regionItem[3]
	{
		{"Japan", &defaultFace(), [](){ setRegion(0); }},
		{"USA", &defaultFace(), [](){ setRegion(1); }},
		{"Europe", &defaultFace(), [](){ setRegion(2); }},
	};

	MultiChoiceMenuItem region
	{
		"Region", &defaultFace(),
		(int)memory.memcard[3] & 0x3,
		regionItem
	};

	BoolMenuItem system
	{
		"Mode", &defaultFace(),
		bool(memory.memcard[2] & 0x80),
		"Console (AES)", "Arcade (MVS)",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			memory.memcard[2] = on ? IG::bit(7) : 0;
		}
	};

	static void setRegion(Uint8 val)
	{
		memory.memcard[3] = val;
	}

public:
	UnibiosSwitchesView(ViewAttachParams attach):
		TableView
		{
			"Unibios Switches",
			attach,
			[this](const TableView &)
			{
				return 2;
			},
			[this](const TableView &, unsigned idx) -> MenuItem&
			{
				switch(idx)
				{
					case 0: return region;
					default: return system;
				}
			}
		}
	{}

	/*void onShow()
	{
		// TODO
		region.refreshActive();
		system.refreshActive();
	}*/
};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	TextMenuItem unibiosSwitches
	{
		"Unibios Switches", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(item.active())
				{
					pushAndShow(makeView<UnibiosSwitchesView>(), e);
				}
				else
				{
					app().postMessage("Only used with Unibios");
				}
			}
		}
	};

	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&unibiosSwitches);
		item.emplace_back(&options);
		loadStandardItems();
	}

	void onShow()
	{
		EmuSystemActionsView::onShow();
		bool isUnibios = conf.system >= SYS_UNIBIOS && conf.system <= SYS_UNIBIOS_LAST;
		unibiosSwitches.setActive(EmuSystem::gameIsRunning() && isUnibios);
	}
};

class CustomMainMenuView : public EmuMainMenuView
{
private:
	TextMenuItem gameList
	{
		"Load Game From List", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto gameListMenu = makeView<GameListView>();
			if(!gameListMenu->games())
			{
				app().postMessage(6, true, "No games found, use \"Load Game\" command to browse to a directory with valid games.");
				return;
			}
			pushAndShow(std::move(gameListMenu), e);
		}
	};

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&gameList);
		loadStandardItems();
	}

public:
	CustomMainMenuView(ViewAttachParams attach): EmuMainMenuView{attach, true}
	{
		reloadItems();
		app().setOnMainMenuItemOptionChanged([this](){ reloadItems(); });
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return std::make_unique<CustomMainMenuView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		default: return nullptr;
	}
}
