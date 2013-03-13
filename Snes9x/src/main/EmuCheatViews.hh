#pragma once
#include <Cheats.hh>
//#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr {"Address", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, addrHandler)>(this)},
		value {"Value", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, valHandler)>(this)},
		saved {"Saved Value", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, savedValHandler)>(this)};
	uint idx = 0;
	MenuItem *item[5] = {nullptr};
	char addrStr[7] = {0}, valueStr[3] = {0}, savedStr[3] = {0};

	void syncCheat(const char *newName = 0);
	uint handleAddrFromTextInput(const char *str);
	void addrHandler(DualTextMenuItem &item, const Input::Event &e);
	uint handleValFromTextInput(const char *str);
	void valHandler(DualTextMenuItem &item, const Input::Event &e);
	uint handleSavedValFromTextInput(const char *str);
	void savedValHandler(DualTextMenuItem &item, const Input::Event &e);
	void renamed(const char *str);
	void removed();

public:
	constexpr SystemEditCheatView(): EditCheatView("") { }

	void init(bool highlightFirst, int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG {"Add Game Genie/Action Replay/Gold Finger Code",
		TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatListView, addGGHandler)>(this)};
	TextMenuItem cheat[EmuCheats::MAX];

	uint handleNameFromTextInput(const char *str);
	uint handleCodeFromTextInput(const char *str);
	void addGGHandler(TextMenuItem &item, const Input::Event &e);
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
