#pragma once
#include <Cheats.hh>
#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem ggCode {"Code", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, ggCodeHandler)>(this)};
	MdCheat *cheat = nullptr;
	MenuItem *item[3] = {nullptr};

	static bool strIs16BitGGCode(const char *str)
	{
		return strlen(str) == 9 && str[4] == '-';
	}

	static bool strIs8BitGGCode(const char *str)
	{
		return strlen(str) == 11 && str[3] == '-' && str[7] == '-';
	}

	static bool strIs16BitARCode(const char *str)
	{
		return strlen(str) == 11 && str[6] == ':';
	}

	static bool strIs8BitARCode(const char *str)
	{
		return strlen(str) == 9 && str[6] == ':';
	}

	static bool strIs8BitCode(const char *str)
	{
		return strIs8BitGGCode(str) || strIs8BitARCode(str);
	}

	static bool strIs16BitCode(const char *str)
	{
		return strIs16BitGGCode(str) || strIs16BitARCode(str);
	}

	uint handleGgCodeFromTextInput(const char *str);
	void ggCodeHandler(DualTextMenuItem &item, const Input::Event &e);
	void renamed(const char *str);
	void removed();

public:
	constexpr SystemEditCheatView(): EditCheatView("Edit Code")	{ }

	void init(bool highlightFirst, MdCheat &cheat);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS {"Add Game Genie / Action Replay Code",
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
