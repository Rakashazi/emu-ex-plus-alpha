#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <snes9x.h>

static constexpr bool HAS_NSRT = !IS_SNES9X_VERSION_1_4;

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
	BoolMenuItem blockInvalidVRAMAccess
	{
		"Block Invalid VRAM Access",
		(bool)optionBlockInvalidVRAMAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			EmuSystem::sessionOptionSet();
			optionBlockInvalidVRAMAccess = item.flipBoolValue(*this);
			Settings.BlockInvalidVRAMAccessMaster = optionBlockInvalidVRAMAccess;
		}
	};
	#endif

	std::array<MenuItem*, IS_SNES9X_VERSION_1_4 ? 3 : 4> menuItem
	{
		&inputPorts,
		&multitap,
		&videoSystem,
		#ifndef SNES9X_VERSION_1_4
		&blockInvalidVRAMAccess
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
				auto &optionView = *new ConsoleOptionView{attachParams()};
				pushAndShow(optionView, e);
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

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return new CustomSystemActionsView(attach);
		case ViewID::EDIT_CHEATS: return new EmuEditCheatListView(attach);
		case ViewID::LIST_CHEATS: return new EmuCheatsView(attach);
		default: return nullptr;
	}
}
