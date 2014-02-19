#pragma once
#include "OptionView.hh"

static void setupSNESInput();

class SystemOptionView : public OptionView
{
public:

	BoolMenuItem multitap
	{
		"5-Player Adapter",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			optionMultitap = item.on;
			setupSNESInput();
		}
	};

	MultiChoiceSelectMenuItem inputPorts
	{
		"Input Ports",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == SNES_JOYPAD_MENU_IDX)
			{
				snesInputPort = SNES_JOYPAD;
			}
			else if(val == SNES_SUPERSCOPE_MENU_IDX)
			{
				snesInputPort = SNES_SUPERSCOPE;
			}
			else if(val == SNES_MOUSE_MENU_IDX)
			{
				snesInputPort = SNES_MOUSE_SWAPPED;
			}
			#ifndef SNES9X_VERSION_1_4
			else
				snesInputPort = SNES_AUTO_INPUT;
			#endif
			setupSNESInput();
		}
	};

	#ifndef SNES9X_VERSION_1_4
	static constexpr int SNES_JOYPAD_MENU_IDX = 1;
	static constexpr int SNES_SUPERSCOPE_MENU_IDX = 2;
	static constexpr int SNES_MOUSE_MENU_IDX = 3;
	#else
	static constexpr int SNES_JOYPAD_MENU_IDX = 0;
	static constexpr int SNES_SUPERSCOPE_MENU_IDX = 1;
	static constexpr int SNES_MOUSE_MENU_IDX = 2;
	#endif
	void inputPortsInit()
	{
		static const char *str[] =
		{
			#ifndef SNES9X_VERSION_1_4
			"Auto (NSRT)",
			#endif
			"Gamepads", "Superscope", "Mouse"
		};
		int setting = 0;
		if(snesInputPort == SNES_JOYPAD)
			setting = SNES_JOYPAD_MENU_IDX;
		else if(snesInputPort == SNES_SUPERSCOPE)
			setting = SNES_SUPERSCOPE_MENU_IDX;
		else if(snesInputPort == SNES_MOUSE_SWAPPED)
			setting = SNES_MOUSE_MENU_IDX;

		inputPorts.init(str, setting, sizeofArray(str));
	}

	#ifndef SNES9X_VERSION_1_4
	BoolMenuItem blockInvalidVRAMAccess
	{
		"Block Invalid VRAM Access",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			optionBlockInvalidVRAMAccess = item.on;
			Settings.BlockInvalidVRAMAccessMaster = item.on;
		}
	};
	#endif

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		#ifndef SNES9X_VERSION_1_4
		blockInvalidVRAMAccess.init(optionBlockInvalidVRAMAccess); item[items++] = &blockInvalidVRAMAccess;
		#endif
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		multitap.init(optionMultitap); item[items++] = &multitap;
	}
};

#include "EmuCheatViews.hh"
#include "MenuView.hh"

class SystemMenuView : public MenuView
{
	TextMenuItem cheats
	{
		"Cheats",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *menuAllocator.allocNew<CheatsView>(window());
				cheatsMenu.init(!e.isPointer());
				pushAndShow(cheatsMenu, &menuAllocator);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView(win) { }

	void onShow()
	{
		MenuView::onShow();
		cheats.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		logMsg("init menu");
		uint items = 0;
		loadFileBrowserItems(item, items);
		cheats.init(); item[items++] = &cheats;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
