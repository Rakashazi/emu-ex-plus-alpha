#pragma once
#include "OptionView.hh"

class NgpOptionView : public OptionView
{
	BoolTextMenuItem ngpLanguage;

	static void ngpLanguageHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		language_english = item.on;
	}

	MenuItem *item[24];

public:

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		ngpLanguage.init("NGP Language", "Japanese", "English", language_english); item[items++] = &ngpLanguage;
		ngpLanguage.selectDelegate().bind<&ngpLanguageHandler>();
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
