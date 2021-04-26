#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#endif
#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <snes9x.h>

static constexpr bool HAS_NSRT = !IS_SNES9X_VERSION_1_4;

#ifndef SNES9X_VERSION_1_4
class CustomAudioOptionView : public AudioOptionView
{
	void setDSPInterpolation(uint8_t val)
	{
		logMsg("set DSP interpolation:%u", val);
		optionAudioDSPInterpolation = val;
		SNES::dsp.spc_dsp.interpolation = val;
	}

	TextMenuItem dspInterpolationItem[5]
	{
		{"None", &defaultFace(), [this](){ setDSPInterpolation(0); }},
		{"Linear", &defaultFace(), [this](){ setDSPInterpolation(1); }},
		{"Gaussian", &defaultFace(), [this](){ setDSPInterpolation(2); }},
		{"Cubic", &defaultFace(), [this](){ setDSPInterpolation(3); }},
		{"Sinc", &defaultFace(), [this](){ setDSPInterpolation(4); }},
	};

	MultiChoiceMenuItem dspInterpolation
	{
		"DSP Interpolation", &defaultFace(),
		optionAudioDSPInterpolation,
		dspInterpolationItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&dspInterpolation);
	}
};
#endif

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	BoolMenuItem multitap
	{
		"5-Player Adapter", &defaultFace(),
		(bool)optionMultitap,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionMultitap = item.flipBoolValue(*this);
			setupSNESInput();
		}
	};

	TextMenuItem inputPortsItem[HAS_NSRT ? 4 : 3]
	{
		#ifndef SNES9X_VERSION_1_4
		{"Auto (NSRT)", &defaultFace(), []() { setInputPorts(SNES_AUTO_INPUT); }},
		#endif
		{"Gamepads", &defaultFace(), []() { setInputPorts(SNES_JOYPAD); }},
		{"Superscope", &defaultFace(), []() { setInputPorts(SNES_SUPERSCOPE); }},
		{"Mouse", &defaultFace(), []() { setInputPorts(SNES_MOUSE_SWAPPED); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		[]()
		{
			constexpr int SNES_JOYPAD_MENU_IDX = HAS_NSRT ? 1 : 0;
			constexpr int SNES_SUPERSCOPE_MENU_IDX = HAS_NSRT ? 2 : 1;
			constexpr int SNES_MOUSE_MENU_IDX = HAS_NSRT ? 3 : 2;
			if(snesInputPort == SNES_JOYPAD)
				return SNES_JOYPAD_MENU_IDX;
			else if(snesInputPort == SNES_SUPERSCOPE)
				return SNES_SUPERSCOPE_MENU_IDX;
			else if(snesInputPort == SNES_MOUSE_SWAPPED)
				return SNES_MOUSE_MENU_IDX;
			return 0;
		}(),
		inputPortsItem
	};

	static void setInputPorts(int val)
	{
		EmuSystem::sessionOptionSet();
		optionInputPort = val;
		snesInputPort = val;
		setupSNESInput();
	}

	TextMenuItem videoSystemItem[4]
	{
		{"Auto", &defaultFace(), [this](Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", &defaultFace(), [this](Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", &defaultFace(), [this](Input::Event e){ setVideoSystem(2, e); }},
		{"NTSC + PAL Spoof", &defaultFace(), [this](Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System", &defaultFace(),
		optionVideoSystem,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	#ifndef SNES9X_VERSION_1_4
	TextHeadingMenuItem emulationHacks{"Emulation Hacks", &defaultBoldFace()};

	BoolMenuItem blockInvalidVRAMAccess
	{
		"Allow Invalid VRAM Access", &defaultFace(),
		(bool)!optionBlockInvalidVRAMAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionBlockInvalidVRAMAccess = !item.flipBoolValue(*this);
			PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess;
		}
	};

	BoolMenuItem separateEchoBuffer
	{
		"Separate Echo Buffer From Ram", &defaultFace(),
		(bool)optionSeparateEchoBuffer,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionSeparateEchoBuffer = item.flipBoolValue(*this);
			SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer;
		}
	};

	void setSuperFXClock(unsigned val)
	{
		EmuSystem::sessionOptionSet();
		optionSuperFXClockMultiplier = val;
		setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier);
	}

	TextMenuItem superFXClockItem[2]
	{
		{"100%", &defaultFace(), [this]() { setSuperFXClock(100); }},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 5 to 250", "",
					[this](EmuApp &app, auto val)
					{
						if(optionSuperFXClockMultiplier.isValidVal(val))
						{
							setSuperFXClock(val);
							superFXClock.setSelected(std::size(superFXClockItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	};

	MultiChoiceMenuItem superFXClock
	{
		"SuperFX Clock Multiplier", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<5>("%u%%", optionSuperFXClockMultiplier.val).data());
			return true;
		},
		[]()
		{
			if(optionSuperFXClockMultiplier.val == 100)
				return 0;
			else
				return 1;
		}(),
		superFXClockItem
	};
	#endif

	std::array<MenuItem*, IS_SNES9X_VERSION_1_4 ? 3 : 7> menuItem
	{
		&inputPorts,
		&multitap,
		&videoSystem,
		#ifndef SNES9X_VERSION_1_4
		&emulationHacks,
		&blockInvalidVRAMAccess,
		&separateEchoBuffer,
		&superFXClock,
		#endif
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

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
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
		item.emplace_back(&options);
		loadStandardItems();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		#ifndef SNES9X_VERSION_1_4
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		#endif
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}
