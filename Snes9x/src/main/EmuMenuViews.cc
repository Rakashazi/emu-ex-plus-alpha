#include <emuframework/OptionView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <snes9x.h>

#ifndef SNES9X_VERSION_1_4
static constexpr bool HAS_NSRT = true;
#else
static constexpr bool HAS_NSRT = false;
#endif

class CustomInputOptionView : public TableView
{
	BoolMenuItem multitap
	{
		"5-Player Adapter",
		(bool)optionMultitap,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionMultitap = item.flipBoolValue(*this);
			setupSNESInput();
		}
	};

	TextMenuItem inputPortsItem[HAS_NSRT ? 4 : 3]
	{
		#ifndef SNES9X_VERSION_1_4
		{"Auto (NSRT)", []() { snesInputPort = SNES_AUTO_INPUT; setupSNESInput(); }},
		#endif
		{"Gamepads", []() { snesInputPort = SNES_JOYPAD; setupSNESInput(); }},
		{"Superscope", []() { snesInputPort = SNES_SUPERSCOPE; setupSNESInput(); }},
		{"Mouse", []() { snesInputPort = SNES_MOUSE_SWAPPED; setupSNESInput(); }},
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

public:
	CustomInputOptionView(ViewAttachParams attach):
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
					default: return multitap;
				}
			}
		}
	{}
};

class CustomVideoOptionView : public VideoOptionView
{
	static void videoSystemChangedMessage()
	{
		if(EmuSystem::gameIsRunning())
		{
			EmuApp::postMessage("Change does not affect currently running game");
		}
	}

	TextMenuItem videoSystemItem[4]
	{
		{
			"Auto",
			[]()
			{
				optionVideoSystem = 0;
				Settings.ForceNTSC = Settings.ForcePAL = 0;
				videoSystemChangedMessage();
			}},
		{
			"NTSC",
			[]()
			{
				optionVideoSystem = 1;
				Settings.ForceNTSC = 1;
				videoSystemChangedMessage();
			}},
		{
			"PAL",
			[]()
			{
				optionVideoSystem = 2;
				Settings.ForcePAL = 1;
				videoSystemChangedMessage();
			}
		},
		{
			"NTSC + PAL Spoof",
			[]()
			{
				optionVideoSystem = 3;
				Settings.ForceNTSC = Settings.ForcePAL = 1;
				videoSystemChangedMessage();
			}
		},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		optionVideoSystem,
		videoSystemItem
	};

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&videoSystem);
	}
};

class CustomSystemOptionView : public SystemOptionView
{
	#ifndef SNES9X_VERSION_1_4
	BoolMenuItem blockInvalidVRAMAccess
	{
		"Block Invalid VRAM Access",
		(bool)optionBlockInvalidVRAMAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionBlockInvalidVRAMAccess = item.flipBoolValue(*this);
			Settings.BlockInvalidVRAMAccessMaster = optionBlockInvalidVRAMAccess;
		}
	};
	#endif

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		#ifndef SNES9X_VERSION_1_4
		item.emplace_back(&blockInvalidVRAMAccess);
		#endif
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::VIDEO_OPTIONS: return new CustomVideoOptionView(attach);
		case ViewID::INPUT_OPTIONS: return new CustomInputOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new CustomSystemOptionView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
