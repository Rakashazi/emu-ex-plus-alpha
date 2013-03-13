#pragma once

#include <util/gui/BaseMenuView.hh>
#include <EmuSystem.hh>

class BaseCheatsView : public BaseMenuView
{
private:
	TextMenuItem edit {"Add/Edit", TextMenuItem::SelectDelegate::create<&editHandler>()};
	MenuItem *item[EmuCheats::MAX + 1] = {nullptr};

	static void editHandler(TextMenuItem &item, const Input::Event &e);

public:
	constexpr BaseCheatsView(): BaseMenuView("Cheats") { }

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		if(i == 0)
			item[i]->select(this, e);
		else
		{
			cheatSelected(i - 1, e);
		}
	}

	virtual void cheatSelected(uint idx, const Input::Event &e) = 0;
	virtual void loadCheatItems(MenuItem *item[], uint &items) = 0;
	void init(bool highlightFirst);
};

class EditCheatView : public BaseMenuView
{
protected:
	TextMenuItem name {TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatView, nameHandler)>(this)};
	TextMenuItem remove {"Delete Cheat", TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatView, removeHandler)>(this)};

	uint handleNameFromTextInput(const char *str);
	void nameHandler(TextMenuItem &item, const Input::Event &e);
	void removeHandler(TextMenuItem &item, const Input::Event &e);
	void loadNameItem(const char *name, MenuItem *item[], uint &items);
	void loadRemoveItem(MenuItem *item[], uint &items);
	virtual void renamed(const char *str) = 0;
	virtual void removed() = 0;

public:
	constexpr EditCheatView(const char *name): BaseMenuView(name) { }
};

class BaseEditCheatListView : public BaseMenuView
{
private:
	MenuItem *item[EmuCheats::MAX + EmuCheats::MAX_CODE_TYPES] = {nullptr};

public:
	constexpr BaseEditCheatListView(): BaseMenuView("Edit Cheats") { }

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		if(i < EmuCheats::MAX_CODE_TYPES)
			item[i]->select(this, e);
		else
		{
			cheatSelected(i - EmuCheats::MAX_CODE_TYPES, e);
		}
	}

	virtual void cheatSelected(uint idx, const Input::Event &e) = 0;
	virtual void loadAddCheatItems(MenuItem *item[], uint &items) = 0;
	virtual void loadCheatItems(MenuItem *item[], uint &items) = 0;

	void init(bool highlightFirst)
	{
		uint i = 0;
		loadAddCheatItems(item, i);
		assert(i == EmuCheats::MAX_CODE_TYPES);
		loadCheatItems(item, i);
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

void refreshCheatViews();
