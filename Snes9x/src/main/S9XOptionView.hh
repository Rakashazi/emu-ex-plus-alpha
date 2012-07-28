#pragma once
#include "OptionView.hh"

static void setupSNESInput();

class S9xOptionView : public OptionView
{
private:

	static void multitapHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionMultitap = item.on;
		setupSNESInput();
	}

	BoolMenuItem multitap;

	MultiChoiceSelectMenuItem inputPorts;

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

		inputPorts.init("Input Ports", str, setting, sizeofArray(str));
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

	MenuItem *item[24];

public:

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		inputPortsInit(); item[items++] = &inputPorts;
		multitap.init("5-Player Adapter", optionMultitap); item[items++] = &multitap;
		multitap.selectDelegate().bind<&multitapHandler>();
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};
