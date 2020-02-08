#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <fceu/fds.h>
#include <fceu/sound.h>
#include <fceu/fceu.h>

extern int pal_emulation;

class ConsoleOptionView : public TableView
{
	BoolMenuItem fourScore
	{
		"4-Player Adapter",
		(bool)optionFourScore,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionFourScore = item.flipBoolValue(*this);
			setupNESFourScore();
		}
	};

	TextMenuItem inputPortsItem[4]
	{
		{"Auto", [](){ setInputPorts(SI_UNSET, SI_UNSET); }},
		{"Gamepads", [](){ setInputPorts(SI_GAMEPAD, SI_GAMEPAD); }},
		{"Gun (2P, NES)", [](){ setInputPorts(SI_GAMEPAD, SI_ZAPPER); }},
		{"Gun (1P, VS)", [](){ setInputPorts(SI_ZAPPER, SI_GAMEPAD); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports",
		[]()
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

	static void setInputPorts(ESI port1, ESI port2)
	{
		EmuSystem::sessionOptionSet();
		optionInputPort1 = (int)port1;
		optionInputPort2 = (int)port2;
		nesInputPortDev[0] = port1;
		nesInputPortDev[1] = port2;
		setupNESInputPorts();
	}

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(2, e); }},
		{"Dendy", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return dendy ? "Dendy" : pal_emulation ? "PAL" : "NTSC";
			}
			else
				return nullptr;
		},
		optionVideoSystem,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		setRegion(val, optionDefaultVideoSystem.val, autoDetectedRegion);
		EmuApp::promptSystemReloadDueToSetOption(attachParams(), e);
	}

	std::array<MenuItem*, 3> menuItem
	{
		&inputPorts,
		&fourScore,
		&videoSystem
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

class CustomVideoOptionView : public VideoOptionView
{
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

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", [this](TextMenuItem &, View &, Input::Event e){ optionDefaultVideoSystem = 0; }},
		{"NTSC", [this](TextMenuItem &, View &, Input::Event e){ optionDefaultVideoSystem = 1; }},
		{"PAL", [this](TextMenuItem &, View &, Input::Event e){ optionDefaultVideoSystem = 2; }},
		{"Dendy", [this](TextMenuItem &, View &, Input::Event e){ optionDefaultVideoSystem = 3; }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Default Video System",
		optionDefaultVideoSystem,
		videoSystemItem
	};

	static constexpr const char *firebrandXPalPath = "Smooth (FBX).pal";
	static constexpr const char *wavebeamPalPath = "Wavebeam.pal";
	static constexpr const char *classicPalPath = "Classic (FBX).pal";

	std::array<char, 32> paletteFileName{};

	static void setPalette(const char *palPath)
	{
		if(palPath)
			string_copy(defaultPalettePath, palPath);
		else
			defaultPalettePath = {};
		setDefaultPalette(palPath);
	}

	constexpr int defaultPaletteCustomFileIdx()
	{
		return (int)std::size(defaultPalItem) - 1;
	}

	TextMenuItem defaultPalItem[5]
	{
		{"FCEUX", [](){ setPalette({}); }},
		{"FirebrandX", []() { setPalette(firebrandXPalPath); }},
		{"Wavebeam", []() { setPalette(wavebeamPalPath); }},
		{"Classic", []() { setPalette(classicPalPath); }},
		{"Custom File", [this](TextMenuItem &, View &, Input::Event e)
			{
				auto startPath = EmuApp::mediaSearchPath();
				auto fsFilter = [](const char *name)
					{
						return string_hasDotExtension(name, "pal");
					};
				auto fPicker = makeView<EmuFilePicker>(startPath.data(), false, fsFilter, FS::RootPathInfo{}, e, false, false);
				fPicker->setOnSelectFile(
					[this](FSPicker &picker, const char *name, Input::Event)
					{
						setPalette(picker.makePathString(name).data());
						string_copy(paletteFileName,
							FS::makeFileStringWithoutDotExtension(name).data());
						picker.dismiss();
						defaultPal.setSelected(defaultPaletteCustomFileIdx());
						popAndShow();
					});
				EmuApp::pushAndShowModalView(std::move(fPicker), e);
				return false;
			}},
	};

	MultiChoiceMenuItem defaultPal
	{
		"Default Palette",
		[this](int idx) -> const char*
		{
			if(idx == defaultPaletteCustomFileIdx())
			{
				return paletteFileName.data();
			}
			else
				return nullptr;
		},
		[this]()
		{
			if(string_equal(defaultPalettePath.data(), ""))
				return 0;
			if(string_equal(defaultPalettePath.data(), firebrandXPalPath))
				return 1;
			else if(string_equal(defaultPalettePath.data(), wavebeamPalPath))
				return 2;
			else if(string_equal(defaultPalettePath.data(), classicPalPath))
				return 3;
			else
				return defaultPaletteCustomFileIdx();
		}(),
		defaultPalItem
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		auto paletteName = FS::basename(defaultPalettePath);
		string_copy(paletteFileName, FS::makeFileStringWithoutDotExtension(paletteName).data());
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&defaultPal);
		item.emplace_back(&videoSystem);
		item.emplace_back(&spriteLimit);
	}
};

class CustomAudioOptionView : public AudioOptionView
{
	static void setQuality(int quaility)
	{
		optionSoundQuality = quaility;
		FCEUI_SetSoundQuality(quaility);
	}

	TextMenuItem qualityItem[3]
	{
		{"Normal", [](){ setQuality(0); }},
		{"High", []() { setQuality(1); }},
		{"Highest", []() { setQuality(2); }}
	};

	MultiChoiceMenuItem quality
	{
		"Emulation Quality",
		optionSoundQuality,
		qualityItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&quality);
	}
};

class CustomSystemOptionView : public SystemOptionView
{
	char fdsBiosPathStr[256]{};

	TextMenuItem fdsBiosPath
	{
		fdsBiosPathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto biosSelectMenu = makeViewWithName<BiosSelectMenu>("Disk System BIOS", &::fdsBiosPath,
				[this]()
				{
					logMsg("set fds bios %s", ::fdsBiosPath.data());
					printBiosMenuEntryStr(fdsBiosPathStr);
					fdsBiosPath.compile(renderer(), projP);
				},
				hasFDSBIOSExtension);
			pushAndShow(std::move(biosSelectMenu), e);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "Disk System BIOS: %s", strlen(::fdsBiosPath.data()) ? FS::basename(::fdsBiosPath).data() : "None set");
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
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
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				popAndShow();
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

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	char diskLabel[sizeof("FDS Control (Disk 1:A)")+2]{};

	TextMenuItem fdsControl
	{
		diskLabel,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				pushAndShow(makeView<FDSControlView>(), e);
			}
			else
				EmuApp::postMessage(2, false, "Disk System not in use");
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

	TextMenuItem options
	{
		"Console Options",
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
		item.emplace_back(&fdsControl);
		item.emplace_back(&options);
		loadStandardItems();
	}

	void onShow()
	{
		EmuSystemActionsView::onShow();
		refreshFDSItem();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}
