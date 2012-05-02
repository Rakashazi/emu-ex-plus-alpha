#pragma once

#include "MenuView.hh"

class S9xMenuView : public MenuView
{
private:
	MenuItem *item[STANDARD_ITEMS];

public:
	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
