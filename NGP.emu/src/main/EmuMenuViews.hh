#pragma once
#include "OptionView.hh"

class SystemOptionView : public OptionView
{
	BoolMenuItem ngpLanguage
	{
		"NGP Language",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			language_english = item.on;
		}
	};

public:
	SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		ngpLanguage.init("Japanese", "English", language_english); item[items++] = &ngpLanguage;
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	SystemMenuView() { }
};
