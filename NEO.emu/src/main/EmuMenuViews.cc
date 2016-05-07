#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include <imagine/util/bits.h>
#include "internal.hh"

extern "C"
{
	#include <gngeo/roms.h>
	#include <gngeo/conf.h>
	#include <gngeo/emu.h>
	#include <gngeo/fileio.h>
	#include <gngeo/timer.h>
	#include <gngeo/memory.h>
}

class EmuSystemOptionView : public SystemOptionView
{
private:
	TextMenuItem timerItem[3]
	{
		{"Off", [](){ optionTimerInt = 0; setTimerIntOption(); }},
		{"On", [](){ optionTimerInt = 1; setTimerIntOption(); }},
		{"Auto", [](){ optionTimerInt = 2; setTimerIntOption(); }},
	};

	MultiChoiceMenuItem timer
	{
		"Emulate Timer",
		std::min((uint)optionTimerInt, 2u),
		timerItem
	};

	TextMenuItem regionItem[4]
	{
		{"Japan", [](){ conf.country = CTY_JAPAN; optionMVSCountry = conf.country; }},
		{"Europe", [](){ conf.country = CTY_EUROPE; optionMVSCountry = conf.country; }},
		{"USA", [](){ conf.country = CTY_USA; optionMVSCountry = conf.country; }},
		{"Asia", [](){ conf.country = CTY_ASIA; optionMVSCountry = conf.country; }},
	};

	MultiChoiceMenuItem region
	{
		"MVS Region",
		std::min((uint)conf.country, 3u),
		regionItem
	};

	TextMenuItem biosItem[4]
	{
		{"Unibios 2.3", [](){ conf.system = SYS_UNIBIOS; optionMVSCountry = conf.system; }},
		{"Unibios 3.0", [](){ conf.system = SYS_UNIBIOS_3_0; optionMVSCountry = conf.system; }},
		{"Unibios 3.1", [](){ conf.system = SYS_UNIBIOS_3_1; optionMVSCountry = conf.system; }},
		{"MVS", [](){ conf.system = SYS_ARCADE; optionMVSCountry = conf.system; }},
	};

	MultiChoiceMenuItem bios
	{
		"BIOS Type",
		[]() -> uint
		{
			switch(conf.system)
			{
				default: return 0;
				case SYS_UNIBIOS_3_0: return 1;
				case SYS_UNIBIOS_3_1: return 2;
				case SYS_ARCADE: return 3;
			}
		}(),
		biosItem
	};

	BoolMenuItem createAndUseCache
	{
		"Make/Use Cache Files",
		(bool)optionCreateAndUseCache,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionCreateAndUseCache = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem strictROMChecking
	{
		"Strict ROM Checking",
		(bool)optionStrictROMChecking,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionStrictROMChecking = item.flipBoolValue(*this);
		}
	};

public:
	EmuSystemOptionView(Base::Window &win): SystemOptionView{win, true}
	{
		loadStockItems();
		item.emplace_back(&bios);
		item.emplace_back(&region);
		item.emplace_back(&timer);
		item.emplace_back(&createAndUseCache);
		item.emplace_back(&strictROMChecking);
	}
};

class EmuGUIOptionView : public GUIOptionView
{
	BoolMenuItem listAll
	{
		"List All Games",
		(bool)optionListAllGames,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionListAllGames = item.flipBoolValue(*this);
		}
	};

public:
	EmuGUIOptionView(Base::Window &win): GUIOptionView{win, true}
	{
		loadStockItems();
		item.emplace_back(&listAll);
	}
};

struct RomListEntry
{
	const char* filename;
	uint bugs;
};

static const RomListEntry romlist[]
{
	{ "2020bb.zip", 0 },
	{ "2020bba.zip", 0 },
	{ "2020bbh.zip", 0 },
	{ "3countb.zip", 0 },
	{ "alpham2.zip", 0 },
	{ "androdun.zip", 0 },
	{ "aodk.zip", 0 },
	{ "aof.zip", 0 },
	{ "aof2.zip", 0 },
	{ "aof2a.zip", 0 },
	{ "aof3.zip", 0 },
	{ "aof3k.zip", 0 },
	{ "bakatono.zip", 0 },
	{ "bangbead.zip", 0 },
	{ "bbbuster.zip", 0 },
	{ "bjourney.zip", 0 },
	{ "blazstar.zip", 0 },
	{ "breakers.zip", 0 },
	{ "breakrev.zip", 0 },
	{ "bstars.zip", 0 },
	{ "bstars2.zip", 0 },
	{ "burningf.zip", 0 },
	{ "burningfh.zip", 0 },
	{ "crsword.zip", 0 },
	{ "ct2k3sa.zip", 1 },
	{ "ct2k3sp.zip", 1 },
	{ "cthd2003.zip", 1 },
	{ "ctomaday.zip", 0 },
	{ "cyberlip.zip", 0 },
	{ "diggerma.zip", 0 },
	{ "doubledr.zip", 0 },
	{ "eightman.zip", 0 },
	{ "fatfursa.zip", 0 },
	{ "fatfursp.zip", 0 },
	{ "fatfury1.zip", 0 },
	{ "fatfury2.zip", 1 },
	{ "fatfury3.zip", 0 },
	{ "fbfrenzy.zip", 0 },
	{ "fightfev.zip", 0 },
	{ "fightfeva.zip", 0 },
	{ "flipshot.zip", 0 },
	{ "fswords.zip", 0 },
	{ "galaxyfg.zip", 0 },
	{ "ganryu.zip", 0 },
	{ "garou.zip", 0 },
	{ "garoubl.zip", 0 },
	{ "garouo.zip", 0 },
	{ "garoup.zip", 0 },
	{ "ghostlop.zip", 0 },
	{ "goalx3.zip", 0 },
	{ "gowcaizr.zip", 0 },
	{ "gpilots.zip", 0 },
	{ "gpilotsh.zip", 0 },
	{ "gururin.zip", 0 },
	{ "ironclad.zip", 0 },
	{ "ironclado.zip", 0 },
	{ "irrmaze.zip", 1 },
	{ "janshin.zip", 0 },
	{ "jockeygp.zip", 1 },
	{ "joyjoy.zip", 0 },
	{ "kabukikl.zip", 0 },
	{ "karnovr.zip", 0 },
	{ "kf10thep.zip", 1 },
	{ "kf2k2mp.zip", 1 },
	{ "kf2k2mp2.zip", 1 },
	{ "kf2k2pla.zip", 1 },
	{ "kf2k2pls.zip", 1 },
	{ "kf2k3bl.zip", 1 },
	{ "kf2k3bla.zip", 1 },
	{ "kf2k3pcb.zip", 1 },
	{ "kf2k3pl.zip", 1 },
	{ "kf2k3upl.zip", 1 },
	{ "kf2k5uni.zip", 1 },
	{ "kizuna.zip", 0 },
	{ "kof10th.zip", 1 },
	{ "kof2000.zip", 1 },
	{ "kof2000n.zip", 1 },
	{ "kof2001.zip", 1 },
	{ "kof2001h.zip", 1 },
	{ "kof2002.zip", 1 },
	{ "kof2002b.zip", 1 },
	{ "kof2003.zip", 1 },
	{ "kof2003h.zip", 1 },
	{ "kof2k4se.zip", 1 },
	{ "kof94.zip", 0 },
	{ "kof95.zip", 0 },
	{ "kof95h.zip", 0 },
	{ "kof96.zip", 0 },
	{ "kof96h.zip", 0 },
	{ "kof97.zip", 0 },
	{ "kof97a.zip", 0 },
	{ "kof97pls.zip", 0 },
	{ "kof98.zip", 0 },
	{ "kof98k.zip", 0 },
	{ "kof98n.zip", 0 },
	{ "kof99.zip", 0 },
	{ "kof99a.zip", 0 },
	{ "kof99e.zip", 0 },
	{ "kof99n.zip", 0 },
	{ "kof99p.zip", 0 },
	{ "kog.zip", 1 },
	{ "kotm.zip", 0 },
	{ "kotm2.zip", 0 },
	{ "kotmh.zip", 0 },
	{ "lans2004.zip", 1 },
	{ "lastblad.zip", 0 },
	{ "lastbladh.zip", 0 },
	{ "lastbld2.zip", 0 },
	{ "lastsold.zip", 0 },
	{ "lbowling.zip", 0 },
	{ "legendos.zip", 0 },
	{ "lresort.zip", 0 },
	{ "magdrop2.zip", 0 },
	{ "magdrop3.zip", 0 },
	{ "maglord.zip", 0 },
	{ "maglordh.zip", 0 },
	{ "mahretsu.zip", 0 },
	{ "marukodq.zip", 0 },
	{ "matrim.zip", 0 },
	{ "matrimbl.zip", 1 },
	{ "miexchng.zip", 0 },
	{ "minasan.zip", 0 },
	{ "mosyougi.zip", 0 },
	{ "ms4plus.zip", 1 },
	{ "ms5pcb.zip", 1 },
	{ "ms5plus.zip", 1 },
	{ "mslug.zip", 0 },
	{ "mslug2.zip", 0 },
	{ "mslug3.zip", 0 },
	{ "mslug3b6.zip", 0 },
	{ "mslug3h.zip", 0 },
	{ "mslug3n.zip", 0 },
	{ "mslug4.zip", 0 },
	{ "mslug5.zip", 1 },
	{ "mslug5h.zip", 1 },
	{ "mslugx.zip", 0 },
	{ "mutnat.zip", 0 },
	{ "nam1975.zip", 0 },
	{ "ncombat.zip", 0 },
	{ "ncombath.zip", 0 },
	{ "ncommand.zip",
	#ifdef USE_CYCLONE
		0
	#else
		1
	#endif
	},
	{ "neobombe.zip", 0 },
	{ "neocup98.zip", 0 },
	{ "neodrift.zip", 0 },
	{ "neomrdo.zip", 0 },
	{ "ninjamas.zip", 0 },
	{ "nitd.zip", 0 },
	{ "nitdbl.zip", 0 },
	{ "overtop.zip", 0 },
	{ "panicbom.zip", 0 },
	{ "pbobbl2n.zip", 0 },
	{ "pbobblen.zip", 0 },
	{ "pbobblena.zip", 0 },
	{ "pgoal.zip", 0 },
	{ "pnyaa.zip", 1 },
	{ "popbounc.zip", 0 },
	{ "preisle2.zip", 1 },
	{ "pspikes2.zip", 0 },
	{ "pulstar.zip", 0 },
	{ "puzzldpr.zip", 0 },
	{ "puzzledp.zip", 0 },
	{ "quizdai2.zip", 0 },
	{ "quizdais.zip", 0 },
	{ "quizkof.zip", 0 },
	{ "ragnagrd.zip", 0 },
	{ "rbff1.zip", 0 },
	{ "rbff1a.zip", 0 },
	{ "rbff2.zip", 0 },
	{ "rbff2h.zip", 0 },
	{ "rbff2k.zip", 0 },
	{ "rbffspec.zip", 0 },
	{ "ridhero.zip", 1 },
	{ "ridheroh.zip", 1 },
	{ "roboarmy.zip", 0 },
	{ "rotd.zip", 1 },
	{ "s1945p.zip", 1 },
	{ "samsh5sp.zip", 0 },
	{ "samsh5sph.zip", 0 },
	{ "samsh5spn.zip", 0 },
	{ "samsho.zip", 0 },
	{ "samsho2.zip", 0 },
	{ "samsho3.zip", 0 },
	{ "samsho3h.zip", 0 },
	{ "samsho4.zip", 0 },
	{ "samsho5.zip", 1 },
	{ "samsho5b.zip", 1 },
	{ "samsho5h.zip", 1 },
	{ "samshoh.zip", 0 },
	{ "savagere.zip", 0 },
	{ "sdodgeb.zip", 0 },
	{ "sengokh.zip", 0 },
	{ "sengoku.zip", 0 },
	{ "sengoku2.zip", 0 },
	{ "sengoku3.zip", 1 },
	{ "shocktr2.zip", 0 },
	{ "shocktra.zip", 0 },
	{ "shocktro.zip", 0 },
	{ "socbrawl.zip", 0 },
	{ "socbrawla.zip", 0 },
	{ "sonicwi2.zip", 0 },
	{ "sonicwi3.zip", 0 },
	{ "spinmast.zip", 0 },
	{ "ssideki.zip", 0 },
	{ "ssideki2.zip", 0 },
	{ "ssideki3.zip", 0 },
	{ "ssideki4.zip", 0 },
	{ "stakwin.zip", 0 },
	{ "stakwin2.zip", 0 },
	{ "strhoop.zip", 0 },
	{ "superspy.zip", 0 },
	{ "svc.zip", 1 },
	{ "svcboot.zip", 1 },
	{ "svcpcb.zip", 1 },
	{ "svcpcba.zip", 1 },
	{ "svcplus.zip", 1 },
	{ "svcplusa.zip", 1 },
	{ "svcsplus.zip", 1 },
	{ "tophuntr.zip",
	#ifdef USE_CYCLONE
		0
	#else
		1
	#endif
	},
	{ "tophuntra.zip",
	#ifdef USE_CYCLONE
		0
	#else
		1
	#endif
	},
	{ "tpgolf.zip", 0 },
	{ "trally.zip", 0 },
	{ "totcarib.zip", 0 },
	{ "turfmast.zip", 0 },
	{ "twinspri.zip", 0 },
	{ "tws96.zip",
	#ifdef USE_CYCLONE
		0
	#else
		1
	#endif
	},
	{ "viewpoin.zip",
	#ifdef USE_CYCLONE
		0
	#else
		1
	#endif
	},
	{ "vliner.zip", 1 },
	{ "vlinero.zip", 1 },
	{ "wakuwak7.zip", 0 },
	{ "wh1.zip", 0 },
	{ "wh1h.zip", 0 },
	{ "wh1ha.zip", 0 },
	{ "wh2.zip", 0 },
	{ "wh2j.zip", 0 },
	{ "wh2jh.zip", 0 },
	{ "whp.zip", 0 },
	{ "wjammers.zip", 0 },
	{ "zedblade.zip", 0 },
	{ "zintrckb.zip", 0 },
	{ "zupapa.zip", 0 },
};

class GameListView : public TableView
{
private:
	struct GameMenuItem
	{
		std::array<char, 128> longNameStr{};
		TextMenuItem i;

		GameMenuItem(char longName[128], TextMenuItem::SelectDelegate selectDel): i{longNameStr.data(), selectDel}
		{
			string_copy(longNameStr, longName);
		}

		GameMenuItem(const GameMenuItem &o): longNameStr(o.longNameStr), i{longNameStr.data(), o.i.onSelect()} {}
	};

	std::vector<GameMenuItem> item{};

	static void loadGame(const RomListEntry &entry)
	{
		EmuSystem::onLoadGameComplete() =
			[](uint result, Input::Event e)
			{
				loadGameCompleteFromFilePicker(result, e);
			};
		auto res = EmuSystem::loadGameFromPath(FS::makePathString(entry.filename));
		if(res == 1)
		{
			loadGameCompleteFromFilePicker(1, Input::Event{});
		}
		else if(res == 0)
		{
			EmuSystem::clearGamePaths();
		}
	}

public:
	GameListView(Base::Window &win):
		TableView
		{
			"Game List",
			win,
			[this](const TableView &)
			{
				return item.size();
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				return item[idx].i;
			}
		}
	{
		for(const auto &entry : romlist)
		{
			ROM_DEF *drv = dr_check_zip(entry.filename);
			if(drv)
			{
				bool fileExists = FS::exists(entry.filename);
				if(!optionListAllGames && !fileExists)
				{
					// TODO: free via scope exit wrapper
					free(drv);
					continue;
				}
				item.emplace_back(drv->longname,
					[this, &entry](TextMenuItem &item, View &, Input::Event e)
					{
						if(item.active())
						{
							if(entry.bugs)
							{
								auto &ynAlertView = *new YesNoAlertView{window(),
									"This game doesn't yet work properly, load anyway?"};
								ynAlertView.setOnYes(
									[&entry](TextMenuItem &, View &view, Input::Event e)
									{
										view.dismiss();
										loadGame(entry);
									});
								modalViewController.pushAndShow(ynAlertView, e);
							}
							else
							{
								loadGame(entry);
							}
						}
						else
						{
							popup.printf(3, 1, "%s not present", entry.filename);
						}
						return true;
					});
				item.back().i.setActive(fileExists);
			}
			free(drv);
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
		{"Japan", [](){ setRegion(0); }},
		{"USA", [](){ setRegion(1); }},
		{"Europe", [](){ setRegion(2); }},
	};

	MultiChoiceMenuItem region
	{
		"Region",
		(uint)memory.memcard[3] & 0x3,
		regionItem
	};

	BoolMenuItem system
	{
		"Mode",
		bool(memory.memcard[2] & 0x80),
		"Console (AES)", "Arcade (MVS)",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			memory.memcard[2] = IG::updateBits(memory.memcard[2], (Uint8)(on ? IG::bit(7) : 0), (Uint8)0x80);
			memory.sram[2] = IG::updateBits(memory.sram[2], (Uint8)(on ? IG::bit(7) : 0), (Uint8)0x80);
		}
	};

	static void setRegion(Uint8 val)
	{
		memory.memcard[3] = IG::updateBits(memory.memcard[3], (Uint8)val, (Uint8)0x3);
		memory.sram[3] = IG::updateBits(memory.sram[3], (Uint8)val, (Uint8)0x3);
	}

public:
	UnibiosSwitchesView(Base::Window &win):
		TableView
		{
			"Unibios Switches",
			win,
			[this](const TableView &)
			{
				return 2;
			},
			[this](const TableView &, uint idx) -> MenuItem&
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

class EmuMenuView : public MenuView
{
private:

	TextMenuItem gameList
	{
		"Load Game From List",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &gameListMenu = *new GameListView{window()};
			if(!gameListMenu.games())
			{
				popup.post("No games found, use \"Load Game\" command to browse to a directory with valid games.", 6, 1);
				delete &gameListMenu;
				return;
			}
			viewStack.pushAndShow(gameListMenu, e);
		}
	};

	TextMenuItem unibiosSwitches
	{
		"Unibios Switches",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(item.active())
				{
					auto &unibiosSwitchesMenu = *new UnibiosSwitchesView{window()};
					viewStack.pushAndShow(unibiosSwitchesMenu, e);
				}
				else
				{
					popup.post("Only used with Unibios");
				}
			}
		}
	};

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&gameList);
		item.emplace_back(&unibiosSwitches);
		loadStandardItems();
	}

public:
	EmuMenuView(Base::Window &win): MenuView{win, true}
	{
		reloadItems();
		setOnMainMenuItemOptionChanged([this](){ reloadItems(); });
	}

	void onShow()
	{
		MenuView::onShow();
		bool isUnibios = conf.system >= SYS_UNIBIOS && conf.system <= SYS_UNIBIOS_3_1;
		unibiosSwitches.setActive(EmuSystem::gameIsRunning() && isUnibios);
	}
};

View *EmuSystem::makeView(Base::Window &win, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new EmuMenuView(win);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(win);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(win);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(win);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(win);
		default: return nullptr;
	}
}
