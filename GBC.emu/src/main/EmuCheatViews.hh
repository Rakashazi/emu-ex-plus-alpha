#pragma once
#include <Cheats.hh>
#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem ggCode {"Code",
		DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, ggCodeHandler)>(this)};
	GbcCheat *cheat = nullptr;
	MenuItem *item[3] = {nullptr};

	static bool strIsGGCode(const char *str)
	{
		return strlen(str) == 11 && str[3] == '-' && str[7] == '-' &&
			string_isHexValue(&str[0], 3) &&
			string_isHexValue(&str[4], 3) &&
			string_isHexValue(&str[8], 3);
	}

	static bool strIsGSCode(const char *str)
	{
		return strlen(str) == 8 && string_isHexValue(str, 8);
	}

	uint handleGgCodeFromTextInput(const char *str);
	void ggCodeHandler(DualTextMenuItem &item, const Input::Event &e);
	void renamed(const char *str);
	void removed();

public:
	constexpr SystemEditCheatView(): EditCheatView("Edit Code")	{ }

	void init(bool highlightFirst, GbcCheat &cheat);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS {"Add Game Genie / GameShark Code",
		TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatListView, addGGGSHandler)>(this)};
	TextMenuItem cheat[EmuCheats::MAX];

	uint handleNameFromTextInput(const char *str);
	void addGGGSHandler(TextMenuItem &item, const Input::Event &e);
	void cheatSelected(uint idx, const Input::Event &e) override;
	void loadAddCheatItems(MenuItem *item[], uint &items) override;
	void loadCheatItems(MenuItem *item[], uint &items) override;

public:
	constexpr EditCheatListView() { }
};

class CheatsView : public BaseCheatsView
{
private:
	BoolMenuItem cheat[EmuCheats::MAX];
	void cheatSelected(uint idx, const Input::Event &e) override;
	void loadCheatItems(MenuItem *item[], uint &i);

public:
	constexpr CheatsView() { }
};

extern SystemEditCheatView editCheatView;
extern CheatsView cheatsMenu;
