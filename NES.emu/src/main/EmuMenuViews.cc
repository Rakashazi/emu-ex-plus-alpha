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
#include <emuframework/AudioOptionView.hh>
#include <emuframework/VideoOptionView.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include "EmuCheatViews.hh"
#include "MainApp.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <fceu/fds.h>
#include <fceu/sound.h>
#include <fceu/fceu.h>

extern int pal_emulation;

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	BoolMenuItem fourScore
	{
		"4-Player Adapter", &defaultFace(),
		(bool)system().optionFourScore,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().sessionOptionSet();
			system().optionFourScore = item.flipBoolValue(*this);
			system().setupNESFourScore();
		}
	};

	static uint16_t packInputEnums(ESI port1, ESI port2)
	{
		return (uint16_t)port1 | ((uint16_t)port2 << 8);
	}

	static std::pair<ESI, ESI> unpackInputEnums(uint16_t packed)
	{
		return {ESI(packed & 0xFF), ESI(packed >> 8)};
	}

	TextMenuItem inputPortsItem[4]
	{
		{"Auto",          &defaultFace(), setInputPortsDel(), packInputEnums(SI_UNSET, SI_UNSET)},
		{"Gamepads",      &defaultFace(), setInputPortsDel(), packInputEnums(SI_GAMEPAD, SI_GAMEPAD)},
		{"Gun (2P, NES)", &defaultFace(), setInputPortsDel(), packInputEnums(SI_GAMEPAD, SI_ZAPPER)},
		{"Gun (1P, VS)",  &defaultFace(), setInputPortsDel(), packInputEnums(SI_ZAPPER, SI_GAMEPAD)},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		(MenuItem::Id)packInputEnums(system().nesInputPortDev[0], system().nesInputPortDev[1]),
		inputPortsItem
	};

	TextMenuItem::SelectDelegate setInputPortsDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			auto [port1, port2] = unpackInputEnums(item.id());
			system().optionInputPort1 = (int)port1;
			system().optionInputPort2 = (int)port2;
			system().nesInputPortDev[0] = port1;
			system().nesInputPortDev[1] = port2;
			system().setupNESInputPorts();
		};
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
		"System", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			if(idx == 0)
			{
				t.resetString(dendy ? "Dendy" : pal_emulation ? "PAL" : "NTSC");
				return true;
			}
			return false;
		},
		system().optionVideoSystem.val,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		system().sessionOptionSet();
		system().optionVideoSystem = val;
		setRegion(val, system().optionDefaultVideoSystem.val, system().autoDetectedRegion);
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	BoolMenuItem compatibleFrameskip
	{
		"Frameskip Mode", &defaultFace(),
		(bool)system().optionCompatibleFrameskip,
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
						system().sessionOptionSet();
						system().optionCompatibleFrameskip = item.flipBoolValue(*this);
					});
				app().pushAndShowModalView(std::move(ynAlertView), e);
			}
			else
			{
				system().optionCompatibleFrameskip = item.flipBoolValue(*this);
			}
		}
	};

	TextHeadingMenuItem videoHeading{"Video", &defaultBoldFace()};

	TextMenuItem visibleVideoLinesItem[4]
	{
		{"8+224", &defaultFace(), setVisibleVideoLinesDel(8, 224)},
		{"8+232", &defaultFace(), setVisibleVideoLinesDel(8, 232)},
		{"0+232", &defaultFace(), setVisibleVideoLinesDel(0, 232)},
		{"0+240", &defaultFace(), setVisibleVideoLinesDel(0, 240)},
	};

	MultiChoiceMenuItem visibleVideoLines
	{
		"Visible Lines", &defaultFace(),
		[this]()
		{
			switch(system().optionVisibleVideoLines.val)
			{
				default: return 0;
				case 232: return system().optionStartVideoLine == 8 ? 1 : 2;
				case 240: return 3;
			}
		}(),
		visibleVideoLinesItem
	};

	TextMenuItem::SelectDelegate setVisibleVideoLinesDel(uint8_t startLine, uint8_t lines)
	{
		return [this, startLine, lines]()
		{
			system().sessionOptionSet();
			system().optionStartVideoLine = startLine;
			system().optionVisibleVideoLines = lines;
			system().updateVideoPixmap(app().video(), system().optionHorizontalVideoCrop, system().optionVisibleVideoLines);
			system().renderFramebuffer(app().video());
			app().viewController().placeEmuViews();
		};
	}

	BoolMenuItem horizontalVideoCrop
	{
		"Crop 8 Pixels On Sides", &defaultFace(),
		(bool)system().optionHorizontalVideoCrop,
		[this](BoolMenuItem &item)
		{
			system().sessionOptionSet();
			system().optionHorizontalVideoCrop = item.flipBoolValue(*this);
			system().updateVideoPixmap(app().video(), system().optionHorizontalVideoCrop, system().optionVisibleVideoLines);
			system().renderFramebuffer(app().video());
			app().viewController().placeEmuViews();
		}
	};

	std::array<MenuItem*, 7> menuItem
	{
		&inputPorts,
		&fourScore,
		&compatibleFrameskip,
		&videoHeading,
		&videoSystem,
		&visibleVideoLines,
		&horizontalVideoCrop,
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

class CustomVideoOptionView : public VideoOptionView, public MainAppHelper<CustomVideoOptionView>
{
	using  MainAppHelper<CustomVideoOptionView>::app;
	using  MainAppHelper<CustomVideoOptionView>::system;

	BoolMenuItem spriteLimit
	{
		"Sprite Limit", &defaultFace(),
		(bool)system().optionSpriteLimit,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionSpriteLimit = item.flipBoolValue(*this);
			FCEUI_DisableSpriteLimitation(!system().optionSpriteLimit);
		}
	};

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", &defaultFace(), [this](){ system().optionDefaultVideoSystem = 0; }},
		{"NTSC", &defaultFace(), [this](){ system().optionDefaultVideoSystem = 1; }},
		{"PAL", &defaultFace(), [this](){ system().optionDefaultVideoSystem = 2; }},
		{"Dendy", &defaultFace(), [this](){ system().optionDefaultVideoSystem = 3; }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Default Video System", &defaultFace(),
		system().optionDefaultVideoSystem.val,
		videoSystemItem
	};

	static constexpr const char *firebrandXPalPath = "Smooth (FBX).pal";
	static constexpr const char *wavebeamPalPath = "Wavebeam.pal";
	static constexpr const char *classicPalPath = "Classic (FBX).pal";

	void setPalette(IG::ApplicationContext ctx, IG::CStringView palPath)
	{
		if(palPath.size())
			system().defaultPalettePath = palPath;
		else
			system().defaultPalettePath = {};
		system().setDefaultPalette(ctx, palPath);
		auto &app = EmuApp::get(ctx);
		app.renderSystemFramebuffer(app.video());
	}

	constexpr uint32_t defaultPaletteCustomFileIdx()
	{
		return std::size(defaultPalItem) - 1;
	}

	TextMenuItem defaultPalItem[5]
	{
		{"FCEUX", &defaultFace(), [this](){ setPalette(appContext(), ""); }},
		{"FirebrandX", &defaultFace(), [this]() { setPalette(appContext(), firebrandXPalPath); }},
		{"Wavebeam", &defaultFace(), [this]() { setPalette(appContext(), wavebeamPalPath); }},
		{"Classic", &defaultFace(), [this]() { setPalette(appContext(), classicPalPath); }},
		{"Custom File", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e)
			{
				auto fsFilter = [](std::string_view name)
					{
						return IG::stringEndsWithAny(name, ".pal", ".PAL");
					};
				auto fPicker = makeView<EmuFilePicker>(FSPicker::Mode::FILE, fsFilter, e, false);
				fPicker->setOnSelectPath(
					[this](FSPicker &picker, IG::CStringView path, std::string_view name, Input::Event)
					{
						setPalette(appContext(), path.data());
						defaultPal.setSelected(defaultPaletteCustomFileIdx());
						dismissPrevious();
						picker.dismiss();
					});
				fPicker->setPath(app().contentSearchPath(), e);
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
				t.resetString(IG::stringWithoutDotExtension(appContext().fileUriDisplayName(system().defaultPalettePath)));
				return true;
			}
			return false;
		},
		[this]()
		{
			if(system().defaultPalettePath.empty())
				return 0;
			if(system().defaultPalettePath == firebrandXPalPath)
				return 1;
			else if(system().defaultPalettePath == wavebeamPalPath)
				return 2;
			else if(system().defaultPalettePath == classicPalPath)
				return 3;
			else
				return (int)defaultPaletteCustomFileIdx();
		}(),
		defaultPalItem
	};

	TextMenuItem visibleVideoLinesItem[4]
	{
		{"8+224", &defaultFace(), setVisibleVideoLinesDel(8, 224)},
		{"8+232", &defaultFace(), setVisibleVideoLinesDel(8, 232)},
		{"0+232", &defaultFace(), setVisibleVideoLinesDel(0, 232)},
		{"0+240", &defaultFace(), setVisibleVideoLinesDel(0, 240)},
	};

	MultiChoiceMenuItem visibleVideoLines
	{
		"Default Visible Lines", &defaultFace(),
		[this]()
		{
			switch(system().optionDefaultVisibleVideoLines.val)
			{
				default: return 0;
				case 232: return system().optionDefaultStartVideoLine == 8 ? 1 : 2;
				case 240: return 3;
			}
		}(),
		visibleVideoLinesItem
	};

	TextMenuItem::SelectDelegate setVisibleVideoLinesDel(uint8_t startLine, uint8_t lines)
	{
		return [this, startLine, lines]()
		{
			system().optionDefaultStartVideoLine = startLine;
			system().optionDefaultVisibleVideoLines = lines;
			system().optionStartVideoLine.defaultVal = startLine;
			system().optionVisibleVideoLines.defaultVal = lines;
		};
	}

	BoolMenuItem correctLineAspect
	{
		"Correct Line Aspect Ratio", &defaultFace(),
		(bool)system().optionCorrectLineAspect,
		[this](BoolMenuItem &item)
		{
			system().optionCorrectLineAspect = item.flipBoolValue(*this);
			app().viewController().placeEmuViews();
		}
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&defaultPal);
		item.emplace_back(&videoSystem);
		item.emplace_back(&spriteLimit);
		item.emplace_back(&visibleVideoLines);
		item.emplace_back(&correctLineAspect);
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;

	void setQuality(int quaility)
	{
		system().optionSoundQuality = quaility;
		FCEUI_SetSoundQuality(quaility);
	}

	TextMenuItem qualityItem[3]
	{
		{"Normal", &defaultFace(), [this](){ setQuality(0); }},
		{"High", &defaultFace(), [this]() { setQuality(1); }},
		{"Highest", &defaultFace(), [this]() { setQuality(2); }}
	};

	MultiChoiceMenuItem quality
	{
		"Emulation Quality", &defaultFace(),
		system().optionSoundQuality.val,
		qualityItem
	};

	BoolMenuItem lowPassFilter
	{
		"Low Pass Filter", &defaultFace(),
		(bool)FSettings.lowpass,
		[this](BoolMenuItem &item)
		{
			FCEUI_SetLowPass(item.flipBoolValue(*this));
		}
	};

	BoolMenuItem swapDutyCycles
	{
		"Swap Duty Cycles", &defaultFace(),
		swapDuty,
		[this](BoolMenuItem &item)
		{
			swapDuty = item.flipBoolValue(*this);
		}
	};

	TextHeadingMenuItem mixer{"Mixer", &defaultBoldFace()};

	BoolMenuItem squareWave1
	{
		"Square Wave #1", &defaultFace(),
		(bool)FSettings.Square1Volume,
		[this](BoolMenuItem &item)
		{
			FSettings.Square1Volume = item.flipBoolValue(*this) ? 256 : 0;
		}
	};

	BoolMenuItem squareWave2
	{
		"Square Wave #2", &defaultFace(),
		(bool)FSettings.Square2Volume,
		[this](BoolMenuItem &item)
		{
			FSettings.Square2Volume = item.flipBoolValue(*this) ? 256 : 0;
		}
	};

	BoolMenuItem triangleWave1
	{
		"Triangle Wave", &defaultFace(),
		(bool)FSettings.TriangleVolume,
		[this](BoolMenuItem &item)
		{
			FSettings.TriangleVolume = item.flipBoolValue(*this) ? 256 : 0;
		}
	};

	BoolMenuItem noise
	{
		"Noise", &defaultFace(),
		(bool)FSettings.NoiseVolume,
		[this](BoolMenuItem &item)
		{
			FSettings.NoiseVolume = item.flipBoolValue(*this) ? 256 : 0;
		}
	};

	BoolMenuItem dpcm
	{
		"DPCM", &defaultFace(),
		(bool)FSettings.PCMVolume,
		[this](BoolMenuItem &item)
		{
			FSettings.PCMVolume = item.flipBoolValue(*this) ? 256 : 0;
		}
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&quality);
		item.emplace_back(&lowPassFilter);
		item.emplace_back(&swapDutyCycles);
		item.emplace_back(&mixer);
		item.emplace_back(&squareWave1);
		item.emplace_back(&squareWave2);
		item.emplace_back(&triangleWave1);
		item.emplace_back(&noise);
		item.emplace_back(&dpcm);
	}
};

class CustomFilePathOptionView : public FilePathOptionView
{
	TextMenuItem fdsBiosPath
	{
		biosMenuEntryStr(appContext().fileUriDisplayName(EmuEx::fdsBiosPath)), &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto biosSelectMenu = makeViewWithName<BiosSelectMenu>("Disk System BIOS", &EmuEx::fdsBiosPath,
				[this](std::string_view displayName)
				{
					logMsg("set fds bios %s", EmuEx::fdsBiosPath.data());
					fdsBiosPath.compile(biosMenuEntryStr(displayName), renderer(), projP);
				},
				hasFDSBIOSExtension);
			pushAndShow(std::move(biosSelectMenu), e);
		}
	};

	std::string biosMenuEntryStr(std::string_view displayName) const
	{
		return fmt::format("Disk System BIOS: {}", displayName);
	}

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&fdsBiosPath);
	}
};

class FDSControlView : public TableView, public MainAppHelper<FDSControlView>
{
private:
	static constexpr unsigned DISK_SIDES = 4;
	TextMenuItem setSide[DISK_SIDES]
	{
		{
			"Set Disk 1 Side A", &defaultFace(),
			[this](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(0, system());
				view.dismiss();
			}
		},
		{
			"Set Disk 1 Side B", &defaultFace(),
			[this](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(1, system());
				view.dismiss();
			}
		},
		{
			"Set Disk 2 Side A", &defaultFace(),
			[this](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(2, system());
				view.dismiss();
			}
		},
		{
			"Set Disk 2 Side B", &defaultFace(),
			[this](View &view, Input::Event e)
			{
				FCEU_FDSSetDisk(3, system());
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
		u"", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(system().hasContent() && isFDS)
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
		if(!isFDS)
			fdsControl.compile("FDS Control", renderer(), projP);
		else if(!FCEU_FDSInserted())
			fdsControl.compile("FDS Control (No Disk)", renderer(), projP);
		else
			fdsControl.compile(fmt::format("FDS Control (Disk {}:{})", (FCEU_FDSCurrentSide() >> 1) + 1, (FCEU_FDSCurrentSide() & 1) ? 'B' : 'A'),
				renderer(), projP);
	}

	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(system().hasContent())
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

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper<CustomSystemOptionView>
{
	using MainAppHelper<CustomSystemOptionView>::system;

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO", &defaultFace(),
		(bool)system().fastForwardDuringFdsAccess,
		[this](BoolMenuItem &item)
		{
			system().fastForwardDuringFdsAccess = item.flipBoolValue(*this);
		}
	};

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&skipFdcAccess);
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
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}

}
