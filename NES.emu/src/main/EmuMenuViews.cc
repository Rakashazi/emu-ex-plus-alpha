/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/gui/AlertView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <fceu/fds.h>
#include <fceu/sound.h>
#include <fceu/fceu.h>

extern int pal_emulation;

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	BoolMenuItem fourScore
	{
		"4-Player Adapter", &defaultFace(),
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
		{"Auto", &defaultFace(), [](){ setInputPorts(SI_UNSET, SI_UNSET); }},
		{"Gamepads", &defaultFace(), [](){ setInputPorts(SI_GAMEPAD, SI_GAMEPAD); }},
		{"Gun (2P, NES)", &defaultFace(), [](){ setInputPorts(SI_GAMEPAD, SI_ZAPPER); }},
		{"Gun (1P, VS)", &defaultFace(), [](){ setInputPorts(SI_ZAPPER, SI_GAMEPAD); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
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
		{"Auto", &defaultFace(), [this](Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", &defaultFace(), [this](Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", &defaultFace(), [this](Input::Event e){ setVideoSystem(2, e); }},
		{"Dendy", &defaultFace(), [this](Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.setString(dendy ? "Dendy" : pal_emulation ? "PAL" : "NTSC");
				return true;
			}
			return false;
		},
		optionVideoSystem,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		setRegion(val, optionDefaultVideoSystem.val, autoDetectedRegion);
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	BoolMenuItem compatibleFrameskip
	{
		"Frameskip Mode", &defaultFace(),
		(bool)optionCompatibleFrameskip,
		"Fast", "Compatible",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			if(!item.boolValue())
			{
				auto ynAlertView = makeView<YesNoAlertView>(
					"Use compatible mode if the current game has glitches when "
					"fast-forwarding/frame-skipping, at the cost of increased CPU usage.");
				ynAlertView->setOnYes(
					[this, &item]()
					{
						EmuSystem::sessionOptionSet();
						optionCompatibleFrameskip = item.flipBoolValue(*this);
					});
				app().pushAndShowModalView(std::move(ynAlertView), e);
			}
			else
			{
				optionCompatibleFrameskip = item.flipBoolValue(*this);
			}
		}
	};

	std::array<MenuItem*, 4> menuItem
	{
		&inputPorts,
		&fourScore,
		&videoSystem,
		&compatibleFrameskip,
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
		"Sprite Limit", &defaultFace(),
		(bool)optionSpriteLimit,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSpriteLimit = item.flipBoolValue(*this);
			FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
		}
	};

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", &defaultFace(), [this](){ optionDefaultVideoSystem = 0; }},
		{"NTSC", &defaultFace(), [this](){ optionDefaultVideoSystem = 1; }},
		{"PAL", &defaultFace(), [this](){ optionDefaultVideoSystem = 2; }},
		{"Dendy", &defaultFace(), [this](){ optionDefaultVideoSystem = 3; }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Default Video System", &defaultFace(),
		optionDefaultVideoSystem,
		videoSystemItem
	};

	static constexpr const char *firebrandXPalPath = "Smooth (FBX).pal";
	static constexpr const char *wavebeamPalPath = "Wavebeam.pal";
	static constexpr const char *classicPalPath = "Classic (FBX).pal";

	static void setPalette(Base::ApplicationContext ctx, const char *palPath)
	{
		if(palPath)
			string_copy(defaultPalettePath, palPath);
		else
			defaultPalettePath = {};
		setDefaultPalette(ctx, palPath);
		auto &app = EmuApp::get(ctx);
		app.renderSystemFramebuffer(app.video());
	}

	constexpr uint32_t defaultPaletteCustomFileIdx()
	{
		return std::size(defaultPalItem) - 1;
	}

	TextMenuItem defaultPalItem[5]
	{
		{"FCEUX", &defaultFace(), [this](){ setPalette(appContext(), {}); }},
		{"FirebrandX", &defaultFace(), [this]() { setPalette(appContext(), firebrandXPalPath); }},
		{"Wavebeam", &defaultFace(), [this]() { setPalette(appContext(), wavebeamPalPath); }},
		{"Classic", &defaultFace(), [this]() { setPalette(appContext(), classicPalPath); }},
		{"Custom File", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e)
			{
				auto startPath = app().mediaSearchPath();
				auto fsFilter = [](const char *name)
					{
						return string_hasDotExtension(name, "pal");
					};
				auto fPicker = makeView<EmuFilePicker>(startPath.data(), false, fsFilter, FS::RootPathInfo{}, e, false, false);
				fPicker->setOnSelectFile(
					[this](FSPicker &picker, const char *name, Input::Event)
					{
						setPalette(appContext(), picker.makePathString(name).data());
						defaultPal.setSelected(defaultPaletteCustomFileIdx());
						dismissPrevious();
						picker.dismiss();
					});
				app().pushAndShowModalView(std::move(fPicker), e);
				return false;
			}},
	};

	MultiChoiceMenuItem defaultPal
	{
		"Default Palette", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			if(idx == defaultPaletteCustomFileIdx())
			{
				auto paletteName = FS::basename(defaultPalettePath);
				t.setString(FS::makeFileStringWithoutDotExtension(paletteName).data());
				return true;
			}
			return false;
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
				return (int)defaultPaletteCustomFileIdx();
		}(),
		defaultPalItem
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
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
		{"Normal", &defaultFace(), [](){ setQuality(0); }},
		{"High", &defaultFace(), []() { setQuality(1); }},
		{"Highest", &defaultFace(), []() { setQuality(2); }}
	};

	MultiChoiceMenuItem quality
	{
		"Emulation Quality", &defaultFace(),
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
	TextMenuItem fdsBiosPath
	{
		nullptr, &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto biosSelectMenu = makeViewWithName<BiosSelectMenu>("Disk System BIOS", &::fdsBiosPath,
				[this]()
				{
					logMsg("set fds bios %s", ::fdsBiosPath.data());
					fdsBiosPath.compile(makeBiosMenuEntryStr().data(), renderer(), projP);
				},
				hasFDSBIOSExtension);
			pushAndShow(std::move(biosSelectMenu), e);
		}
	};

	static std::array<char, 256> makeBiosMenuEntryStr()
	{
		return string_makePrintf<256>("Disk System BIOS: %s", strlen(::fdsBiosPath.data()) ? FS::basename(::fdsBiosPath).data() : "None set");
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		fdsBiosPath.setName(makeBiosMenuEntryStr().data());
		item.emplace_back(&fdsBiosPath);
	}
};

class FDSControlView : public TableView
{
private:
	static constexpr unsigned DISK_SIDES = 4;
	TextMenuItem setSide[DISK_SIDES]
	{
		{
			"Set Disk 1 Side A", &defaultFace(),
			[](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(0);
				view.dismiss();
			}
		},
		{
			"Set Disk 1 Side B", &defaultFace(),
			[](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(1);
				view.dismiss();
			}
		},
		{
			"Set Disk 2 Side A", &defaultFace(),
			[](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(2);
				view.dismiss();
			}
		},
		{
			"Set Disk 2 Side B", &defaultFace(),
			[](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(3);
				view.dismiss();
			}
		}
	};

	TextMenuItem insertEject
	{
		"Eject", &defaultFace(),
		[this](View &view, Input::Event e)
		{
			if(FCEU_FDSInserted())
			{
				FCEU_FDSInsert();
				view.dismiss();
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
			[this](const TableView &, unsigned idx) -> MenuItem&
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
	TextMenuItem fdsControl
	{
		nullptr, &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning() && isFDS)
			{
				pushAndShow(makeView<FDSControlView>(), e);
			}
			else
				app().postMessage(2, false, "Disk System not in use");
		}
	};

	void refreshFDSItem()
	{
		fdsControl.setActive(isFDS);
		char diskLabel[sizeof("FDS Control (Disk 1:A)")+2]{};
		if(!isFDS)
			strcpy(diskLabel, "FDS Control");
		else if(!FCEU_FDSInserted())
			strcpy(diskLabel, "FDS Control (No Disk)");
		else
			sprintf(diskLabel, "FDS Control (Disk %d:%c)", (FCEU_FDSCurrentSide()>>1)+1, (FCEU_FDSCurrentSide() & 1)? 'B' : 'A');
		fdsControl.compile(diskLabel, renderer(), projP);
	}

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
