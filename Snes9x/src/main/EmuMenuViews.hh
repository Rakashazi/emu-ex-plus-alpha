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

	void inputPortsInit()
	{
		static const char *str[] =
		{
			"Gamepads", "Superscope", "Mouse"
		};
		int setting = 0;
		if(snesInputPort == SNES_SUPERSCOPE)
			setting = 1;
		if(snesInputPort == SNES_MOUSE_SWAPPED)
			setting = 2;

		inputPorts.init(str, setting, sizeofArray(str));
		inputPorts.valueDelegate().bind<&inputPortsSet>();
	}

	static void inputPortsSet(MultiChoiceMenuItem &, int val)
	{
		if(val == 1)
		{
			snesInputPort = SNES_SUPERSCOPE;
		}
		else if(val == 2)
		{
			snesInputPort = SNES_MOUSE_SWAPPED;
		}
		else
		{
			snesInputPort = SNES_JOYPAD;
		}
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
