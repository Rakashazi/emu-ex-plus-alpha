#pragma once
#include "OptionView.hh"

class SystemOptionView : public OptionView
{
	BoolMenuItem ngpLanguage;

	static void ngpLanguageHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		language_english = item.on;
	}

public:
	constexpr SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		ngpLanguage.init("NGP Language", "Japanese", "English", language_english); item[items++] = &ngpLanguage;
		ngpLanguage.selectDelegate().bind<&ngpLanguageHandler>();
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	constexpr SystemMenuView() { }
};
