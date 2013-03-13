#pragma once
#include <Cheats.hh>
//#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr {"Address", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, addrHandler)>(this)},
		value {"Value", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, valHandler)>(this)},
		comp {"Compare", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, compHandler)>(this)};
	DualTextMenuItem ggCode {"GG Code", DualTextMenuItem::SelectDelegate::create<template_mfunc(SystemEditCheatView, ggCodeHandler)>(this)};
	uint idx = 0;
	int type = 0;
	char *nameStr = nullptr;
	MenuItem *item[5] = {nullptr};
	char addrStr[5] = {0}, valueStr[3] = {0}, compStr[3] = {0};
	char ggCodeStr[9] = {0};

	void syncCheat(const char *newName = 0);
	uint handleGgCodeFromTextInput(const char *str);
	void ggCodeHandler(DualTextMenuItem &item, const Input::Event &e);
	uint handleAddrFromTextInput(const char *str);
	void addrHandler(DualTextMenuItem &item, const Input::Event &e);
	uint handleValFromTextInput(const char *str);
	void valHandler(DualTextMenuItem &item, const Input::Event &e);
	uint handleCompFromTextInput(const char *str);
	void compHandler(DualTextMenuItem &item, const Input::Event &e);
	void renamed(const char *str);
	void removed();

public:
	constexpr SystemEditCheatView(): EditCheatView("") { }

	void init(bool highlightFirst, int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG {"Add Game Genie Code", TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatListView, addGGHandler)>(this)};
	TextMenuItem addRAM {"Add RAM Patch", TextMenuItem::SelectDelegate::create<template_mfunc(EditCheatListView, addRAMHandler)>(this)};
	TextMenuItem cheat[EmuCheats::MAX];
	uchar addCheatType = 0;

	uint handleNameFromTextInput(const char *str);
	void addGGHandler(TextMenuItem &item, const Input::Event &e);
	void addRAMHandler(TextMenuItem &item, const Input::Event &e);
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
