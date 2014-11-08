#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include <imagine/util/bits.h>

class SystemOptionView : public OptionView
{
private:
	MultiChoiceSelectMenuItem timer
	{
		"Emulate Timer",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTimerInt = val;
			setTimerIntOption();
		}
	};

	void timerInit()
	{
		static const char *str[] =
		{
				"Off", "On", "Auto"
		};
		timer.init(str, std::min(2, (int)optionTimerInt), sizeofArray(str));
	}

	MultiChoiceSelectMenuItem region
	{
		"MVS Region",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			conf.country = (COUNTRY)val;
			optionMVSCountry = conf.country;
		}
	};

	void regionInit()
	{
		static const char *str[] =
		{
				"Japan", "Europe", "USA", "Asia"
		};
		int setting = 0;
		if(conf.country < 4)
		{
			setting = conf.country;
		}
		region.init(str, setting, sizeofArray(str));
	}

	MultiChoiceSelectMenuItem bios
	{
		"BIOS Type",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			switch(val)
			{
				bcase 0: conf.system = SYS_UNIBIOS;
				bcase 1: conf.system = SYS_UNIBIOS_3_0;
				bcase 2: conf.system = SYS_UNIBIOS_3_1;
				bcase 3: conf.system = SYS_ARCADE;
			}
			optionBIOSType = conf.system;
		}
	};

	void biosInit()
	{
		static const char *str[] =
		{
			"Unibios 2.3", "Unibios 3.0", "Unibios 3.1", "MVS"
		};
		int setting = 0;
		switch(conf.system)
		{
			bdefault: setting = 0;
			bcase SYS_UNIBIOS_3_0: setting = 1;
			bcase SYS_UNIBIOS_3_1: setting = 2;
			bcase SYS_ARCADE: setting = 3;
		}
		bios.init(str, setting, sizeofArray(str));
	}

	BoolMenuItem listAll
	{
		"List All Games",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionListAllGames = item.on;
		}
	};

	BoolMenuItem createAndUseCache
	{
		"Make/Use Cache Files",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle((*this));
			optionCreateAndUseCache = item.on;
		}
	};

	BoolMenuItem strictROMChecking
	{
		"Strict ROM Checking",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			optionStrictROMChecking = item.on;
		}
	};

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		biosInit(); item[items++] = &bios;
		regionInit(); item[items++] = &region;
		timerInit(); item[items++] = &timer;
		createAndUseCache.init(optionCreateAndUseCache); item[items++] = &createAndUseCache;
		strictROMChecking.init(optionStrictROMChecking); item[items++] = &strictROMChecking;
	}

	void loadGUIItems(MenuItem *item[], uint &items)
	{
		OptionView::loadGUIItems(item, items);
		listAll.init(optionListAllGames); item[items++] = &listAll;
	}
};

struct RomListEntry
{
	const char* filename;
	uint bugs;
};

static const RomListEntry romlist[] = {
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
	char longName[sizeofArrayConst(romlist)][128]{};
	TextMenuItem rom[sizeofArrayConst(romlist)];
	MenuItem *item[sizeofArrayConst(romlist)]{};

	static void loadGame(const RomListEntry &entry)
	{
		EmuSystem::onLoadGameComplete() =
			[](uint result, const Input::Event &e)
			{
				loadGameCompleteFromFilePicker(result, e);
			};
		auto res = EmuSystem::loadGame(entry.filename);
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
	GameListView(Base::Window &win): TableView{"Game List", win} { }

	bool init(bool highlightFirst)
	{
		uint i = 0, roms = 0;
		for(const auto &entry : romlist)
		{
			ROM_DEF *drv = dr_check_zip(entry.filename);
			if(drv)
			{
				bool fileExists = FsSys::fileExists(entry.filename);
				if(!optionListAllGames && !fileExists)
				{
					// TODO: free via scope exit wrapper
					free(drv);
					continue;
				}
				string_copy(longName[roms], drv->longname);
				rom[roms].init(longName[roms], fileExists); item[i++] = &rom[roms];
				rom[roms].onSelect() =
					[this, &entry](TextMenuItem &item, View &, const Input::Event &e)
					{
						if(item.active)
						{
							if(entry.bugs)
							{
								auto &ynAlertView = *new YesNoAlertView{window()};
								ynAlertView.init("This game doesn't yet work properly, load anyway?", !e.isPointer());
								ynAlertView.onYes() =
									[&entry](const Input::Event &e)
									{
										loadGame(entry);
									};
								modalViewController.pushAndShow(ynAlertView);
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
					};
				roms++;
			}
			free(drv);
		}
		if(!roms)
			return 0;

		assert(i <= sizeofArray(item));
		TableView::init(item, i, highlightFirst);
		return 1;
	}
};

class UnibiosSwitchesView : public TableView
{
	MenuItem *item[2] {nullptr};
	MultiChoiceSelectMenuItem region
	{
		"Region",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			updateBits(memory.memcard[3], val, 0x3);
			updateBits(memory.sram[3], val, 0x3);
		}
	};

	BoolMenuItem system
	{
		"Mode",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			updateBits(memory.memcard[2], item.on ? IG::bit(7) : 0, 0x80);
			updateBits(memory.sram[2], item.on ? IG::bit(7) : 0, 0x80);
		}
	};
public:
	UnibiosSwitchesView(Base::Window &win): TableView{"Unibios Switches", win} {}

	void regionInit()
	{
		static const char *str[] =
		{
				"Japan", "USA", "Europe"
		};
		int setting = 0;
		setting = memory.memcard[3] & 0x3;
		region.init(str, setting, sizeofArray(str));
	}

	void init(bool highlightFirst)
	{
		uint i = 0;
		regionInit(); item[i++] = &region;
		system.init("Console (AES)", "Arcade (MVS)", memory.memcard[2] & 0x80); item[i++] = &system;
		assert(i <= sizeofArray(item));
		TableView::init(item, i, highlightFirst);
	}

	/*void onShow()
	{
		// TODO
		region.refreshActive();
		system.refreshActive();
	}*/

};

class SystemMenuView : public MenuView
{
private:

	TextMenuItem gameList
	{
		"Load Game From List",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			auto &gameListMenu = *new GameListView{window()};
			if(!gameListMenu.init(!e.isPointer()))
			{
				popup.post("No games found, use \"Load Game\" command to browse to a directory with valid games.", 6, 1);
				return;
			}
			viewStack.pushAndShow(gameListMenu);
		}
	};

	TextMenuItem unibiosSwitches
	{
		"Unibios Switches",
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(item.active)
				{
					auto &unibiosSwitchesMenu = *new UnibiosSwitchesView{window()};
					unibiosSwitchesMenu.init(!e.isPointer());
					viewStack.pushAndShow(unibiosSwitchesMenu);
				}
				else
				{
					popup.post("Only used with Unibios");
				}
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow()
	{
		MenuView::onShow();
		bool isUnibios = conf.system >= SYS_UNIBIOS && conf.system <= SYS_UNIBIOS_3_1;
		unibiosSwitches.active = EmuSystem::gameIsRunning() && isUnibios;
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		gameList.init(); item[items++] = &gameList;
		unibiosSwitches.init(); item[items++] = &unibiosSwitches;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};
