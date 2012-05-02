#pragma once
#include "OptionView.hh"

class GbcOptionView : public OptionView
{
	MultiChoiceSelectMenuItem gbPalette;

	void gbPaletteInit()
	{
		static const char *str[] =
		{
			"Original", "Brown", "Red", "Dark Brown", "Pastel", "Orange", "Yellow", "Blue", "Dark Blue", "Gray", "Green", "Dark Green", "Reverse"
		};
		gbPalette.init("GB Palette", str, int(optionGBPal), sizeofArray(str));
		gbPalette.valueDelegate().bind<&gbPaletteSet>();
	}

	static void gbPaletteSet(MultiChoiceMenuItem &, int val)
	{
		optionGBPal.val = val;
		applyGBPalette(val);
	}

	BoolMenuItem reportAsGba;

	static void reportAsGbaHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionReportAsGba = item.on;
	}

	BoolMenuItem fullSaturation;

	static void fullSaturationHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionFullGbcSaturation = item.on;
		if(EmuSystem::gameIsRunning())
		{
			gbEmu.refreshPalettes();
		}
	}


	MenuItem *item[24];

public:

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		gbPaletteInit(); item[items++] = &gbPalette;
		fullSaturation.init("Saturated GBC Colors", optionFullGbcSaturation); item[items++] = &fullSaturation;
		fullSaturation.selectDelegate().bind<&fullSaturationHandler>();
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		reportAsGba.init("Report system as GBA", optionReportAsGba); item[items++] = &reportAsGba;
		reportAsGba.selectDelegate().bind<&reportAsGbaHandler>();
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
