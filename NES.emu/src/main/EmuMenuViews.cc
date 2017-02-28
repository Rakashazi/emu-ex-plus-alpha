#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <fceu/fds.h>
#include <fceu/sound.h>

class EmuInputOptionView : public TableView
{
	BoolMenuItem fourScore
	{
		"4-Player Adapter",
		(bool)optionFourScore,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionFourScore = item.flipBoolValue(*this);
			setupNESFourScore();
		}
	};

	TextMenuItem inputPortsItem[4]
	{
		{"Auto", [](){ nesInputPortDev[0] = nesInputPortDev[1] = SI_UNSET; setupNESInputPorts(); }},
		{"Gamepads", [](){ nesInputPortDev[0] = nesInputPortDev[1] = SI_GAMEPAD; setupNESInputPorts(); }},
		{"Gun (2P, NES)", [](){ nesInputPortDev[0] = SI_GAMEPAD; nesInputPortDev[1] = SI_ZAPPER; setupNESInputPorts(); }},
		{"Gun (1P, VS)", [](){ nesInputPortDev[0] = SI_ZAPPER; nesInputPortDev[1] = SI_GAMEPAD; setupNESInputPorts(); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports",
		[]() -> uint
		{
			if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_GAMEPAD)
				return 1;
			else if(nesInputPortDev[0] == SI_GAMEPAD && nesInputPortDev[1] == SI_ZAPPER)
				return 2;
			else if(nesInputPortDev[0] == SI_ZAPPER && nesInputPortDev[1] == SI_GAMEPAD)
				return 3;
			else
				return 0;
		}(),
		inputPortsItem
	};

public:
	EmuInputOptionView(ViewAttachParams attach):
		TableView
		{
			"Input Options",
			attach,
			[this](const TableView &)
			{
				return 2;
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				switch(idx)
				{
					case 0: return inputPorts;
					default: return fourScore;
				}
			}
		}
	{}
};

class EmuVideoOptionView : public VideoOptionView
{
	TextMenuItem videoSystemItem[4]
	{
		{
			"Auto",
			[]()
			{
				optionVideoSystem = 0;
				logMsg("Detected Region:%s", regionToStr(autoDetectedRegion));
				FCEUI_SetRegion(autoDetectedRegion, false);
				EmuSystem::configAudioPlayback();
			}},
		{
			"NTSC",
			[]()
			{
				optionVideoSystem = 1;
				FCEUI_SetRegion(0, false);
				EmuSystem::configAudioPlayback();
			}},
		{
			"PAL",
			[]()
			{
				optionVideoSystem = 2;
				FCEUI_SetRegion(1, false);
				EmuSystem::configAudioPlayback();
			}
		},
		{
			"Dendy",
			[]()
			{
				optionVideoSystem = 3;
				FCEUI_SetRegion(2, false);
				EmuSystem::configAudioPlayback();
			}
		},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		optionVideoSystem,
		videoSystemItem
	};

	BoolMenuItem spriteLimit
	{
		"Sprite Limit",
		(bool)optionSpriteLimit,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSpriteLimit = item.flipBoolValue(*this);
			FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
		}
	};

public:
	EmuVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&videoSystem);
		item.emplace_back(&spriteLimit);
	}
};

class EmuAudioOptionView : public AudioOptionView
{
	static void setQuality(int quaility)
	{
		optionSoundQuality = quaility;
		FCEUI_SetSoundQuality(quaility);
	}

	TextMenuItem qualityItem[3]
	{
		{
			"Normal",
			[]()
			{
				setQuality(0);
			}},
		{
			"High",
			[]()
			{
				setQuality(1);
			}},
		{
			"Highest",
			[]()
			{
				setQuality(2);
			}
		}
	};

	MultiChoiceMenuItem quality
	{
		"Emulation Quality",
		optionSoundQuality,
		qualityItem
	};

public:
	EmuAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&quality);
	}
};

class EmuSystemOptionView : public SystemOptionView
{
	char fdsBiosPathStr[256]{};

	TextMenuItem fdsBiosPath
	{
		fdsBiosPathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &biosSelectMenu = *new BiosSelectMenu{"Disk System BIOS", &::fdsBiosPath,
				[this]()
				{
					logMsg("set fds bios %s", ::fdsBiosPath.data());
					printBiosMenuEntryStr(fdsBiosPathStr);
					fdsBiosPath.compile(renderer(), projP);
				},
				hasFDSBIOSExtension, attachParams()};
			viewStack.pushAndShow(biosSelectMenu, e);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "Disk System BIOS: %s", strlen(::fdsBiosPath.data()) ? FS::basename(::fdsBiosPath).data() : "None set");
	}

public:
	EmuSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		printBiosMenuEntryStr(fdsBiosPathStr);
		item.emplace_back(&fdsBiosPath);
	}
};

class FDSControlView : public TableView
{
private:
	static constexpr uint DISK_SIDES = 4;
	TextMenuItem setSide[DISK_SIDES]
	{
		{
			"Set Disk 1 Side A",
			[](TextMenuItem &, View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(0);
				view.popAndShow();
			}
		},
		{
			"Set Disk 1 Side B",
			[](TextMenuItem &, View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(1);
				view.popAndShow();
			}
		},
		{
			"Set Disk 2 Side A",
			[](TextMenuItem &, View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(2);
				view.popAndShow();
			}
		},
		{
			"Set Disk 2 Side B",
			[](TextMenuItem &, View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(3);
				view.popAndShow();
			}
		}
	};

	TextMenuItem insertEject
	{
		"Eject",
		[](TextMenuItem &, View &, Input::Event e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				viewStack.popAndShow();
			}
		}
	};

public:
	FDSControlView(ViewAttachParams attach):
		TableView
		{
			"FDS Control",
			attach,
			[this](const TableView &)
			{
				return 5;
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				switch(idx)
				{
					case 0: return setSide[0];
					case 1: return setSide[1];
					case 2: return setSide[2];
					case 3: return setSide[3];
					default: return insertEject;
				}
			}
		}
	{
		setSide[0].setActive(0 < FCEU_FDSSides());
		setSide[1].setActive(1 < FCEU_FDSSides());
		setSide[2].setActive(2 < FCEU_FDSSides());
		setSide[3].setActive(3 < FCEU_FDSSides());
		insertEject.setActive(FCEU_FDSInserted());
	}
};

class EmuMenuView : public MenuView
{
private:
	char diskLabel[sizeof("FDS Control (Disk 1:A)")]{};

	TextMenuItem fdsControl
	{
		diskLabel,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				auto &fdsMenu = *new FDSControlView{attachParams()};
				pushAndShow(fdsMenu, e);
			}
			else
				popup.post("Disk System not in use", 2);
		}
	};

	void refreshFDSItem()
	{
		fdsControl.setActive(isFDS);
		if(!isFDS)
			strcpy(diskLabel, "FDS Control");
		else if(!FCEU_FDSInserted())
			strcpy(diskLabel, "FDS Control (No Disk)");
		else
			sprintf(diskLabel, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');
		fdsControl.t.setString(diskLabel);
		fdsControl.compile(renderer(), projP);
	}

	void reloadItems()
	{
		item.clear();
		loadFileBrowserItems();
		item.emplace_back(&fdsControl);
		loadStandardItems();
	}

public:
	EmuMenuView(ViewAttachParams attach): MenuView{attach, true}
	{
		reloadItems();
		setOnMainMenuItemOptionChanged([this](){ reloadItems(); });
	}

	void onShow()
	{
		MenuView::onShow();
		refreshFDSItem();
	}
};

View *EmuSystem::makeView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new EmuMenuView(attach);
		case ViewID::VIDEO_OPTIONS: return new EmuVideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new EmuAudioOptionView(attach);
		case ViewID::INPUT_OPTIONS: return new EmuInputOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(attach);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
