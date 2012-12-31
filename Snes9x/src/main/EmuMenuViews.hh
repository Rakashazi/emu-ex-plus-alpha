#pragma once
#include "OptionView.hh"

static void setupSNESInput();

class SystemOptionView : public OptionView
{
private:

	static void multitapHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionMultitap = item.on;
		setupSNESInput();
	}

	BoolMenuItem multitap;

	MultiChoiceSelectMenuItem inputPorts {"Input Ports"};

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
		inputPorts.valueDelegate().bind<&inputPortsSet>();
	}

	static void inputPortsSet(MultiChoiceMenuItem &, int val)
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

public:
	constexpr SystemOptionView() { }

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		multitap.init("5-Player Adapter", optionMultitap); item[items++] = &multitap;
		multitap.selectDelegate().bind<&multitapHandler>();
	}
};

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};
