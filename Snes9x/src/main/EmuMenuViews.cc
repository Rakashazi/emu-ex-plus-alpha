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
		{"None", [this](){ setDSPInterpolation(0); }},
		{"Linear", [this](){ setDSPInterpolation(1); }},
		{"Gaussian", [this](){ setDSPInterpolation(2); }},
		{"Cubic", [this](){ setDSPInterpolation(3); }},
		{"Sinc", [this](){ setDSPInterpolation(4); }},
	};

	MultiChoiceMenuItem dspInterpolation
	{
		"DSP Interpolation",
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

class ConsoleOptionView : public TableView
{
	BoolMenuItem multitap
	{
		"5-Player Adapter",
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
		{"Auto (NSRT)", []() { setInputPorts(SNES_AUTO_INPUT); }},
		#endif
		{"Gamepads", []() { setInputPorts(SNES_JOYPAD); }},
		{"Superscope", []() { setInputPorts(SNES_SUPERSCOPE); }},
		{"Mouse", []() { setInputPorts(SNES_MOUSE_SWAPPED); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports",
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
		{"Auto", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(0, e); }},
		{"NTSC", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(1, e); }},
		{"PAL", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(2, e); }},
		{"NTSC + PAL Spoof", [this](TextMenuItem &, View &, Input::Event e){ setVideoSystem(3, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		optionVideoSystem,
		videoSystemItem
	};

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		EmuApp::promptSystemReloadDueToSetOption(attachParams(), e);
	}

	#ifndef SNES9X_VERSION_1_4
	TextHeadingMenuItem emulationHacks{"Emulation Hacks"};

	BoolMenuItem blockInvalidVRAMAccess
	{
		"Allow Invalid VRAM Access",
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
		"Separate Echo Buffer From Ram",
		(bool)optionSeparateEchoBuffer,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionSeparateEchoBuffer = item.flipBoolValue(*this);
			SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer;
		}
	};
	#endif

	std::array<MenuItem*, IS_SNES9X_VERSION_1_4 ? 3 : 6> menuItem
	{
		&inputPorts,
		&multitap,
		&videoSystem,
		#ifndef SNES9X_VERSION_1_4
		&emulationHacks,
		&blockInvalidVRAMAccess,
		&separateEchoBuffer
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
