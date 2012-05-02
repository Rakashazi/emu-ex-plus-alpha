#pragma once
#include "OptionView.hh"

class NeoOptionView : public OptionView
{
private:

	MultiChoiceSelectMenuItem timer;

	static void timerSet(MultiChoiceMenuItem &, int val)
	{
		optionTimerInt = val;
		setTimerIntOption();
	}

	void timerInit()
	{
		static const char *str[] =
		{
				"Off", "On", "Auto"
		};
		timer.init("Emulate Timer", str, IG::min(2, (int)optionTimerInt), sizeofArray(str));
		timer.valueDelegate().bind<&timerSet>();
	}

	MultiChoiceSelectMenuItem region;

	static void regionSet(MultiChoiceMenuItem &, int val)
	{
		conf.country = (COUNTRY)val;
		optionMVSCountry = conf.country;
	}

	void regionInit()
	{
		static const char *str[] =
		{
				"Japan", "Europe", "USA", "Asia"
		};
		int setting = 0;
		if(conf.country < 4)
		{
			setting = conf.country;
		}
		region.init("MVS Region", str, setting, sizeofArray(str));
		region.valueDelegate().bind<&regionSet>();
	}

	MultiChoiceSelectMenuItem bios;

	static void biosSet(MultiChoiceMenuItem &, int val)
	{
		conf.system = val == 0 ? SYS_UNIBIOS : SYS_ARCADE;
		optionBIOSType = conf.system;
	}

	void biosInit()
	{
		static const char *str[] =
		{
			"Unibios", "MVS"
		};
		int setting = 0;
		if(conf.system == SYS_ARCADE)
		{
			setting = 1;
		}
		bios.init("BIOS Type", str, setting, sizeofArray(str));
		bios.valueDelegate().bind<&biosSet>();
	}

	BoolMenuItem listAll;

	static void listAllHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionListAllGames = item.on;
	}

	BoolMenuItem createAndUseCache;

	static void createAndUseCacheHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionCreateAndUseCache = item.on;
	}

	BoolMenuItem strictROMChecking;

	static void strictROMCheckingHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionStrictROMChecking = item.on;
	}

	MenuItem *item[24];

public:

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		biosInit(); item[items++] = &bios;
		regionInit(); item[items++] = &region;
		timerInit(); item[items++] = &timer;
		createAndUseCache.init("Make/Use Cache Files", optionCreateAndUseCache); item[items++] = &createAndUseCache;
		createAndUseCache.selectDelegate().bind<&createAndUseCacheHandler>();
		strictROMChecking.init("Strict ROM Checking", optionStrictROMChecking); item[items++] = &strictROMChecking;
		strictROMChecking.selectDelegate().bind<&strictROMCheckingHandler>();
	}

	void loadGUIItems(MenuItem *item[], uint &items)
	{
		OptionView::loadGUIItems(item, items);
		listAll.init("List All Games", optionListAllGames); item[items++] = &listAll;
		listAll.selectDelegate().bind<&listAllHandler>();
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
