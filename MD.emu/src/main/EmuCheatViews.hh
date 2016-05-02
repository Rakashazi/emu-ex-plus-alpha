#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>
#include <vector>
#include "system.h"
uint decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() override;

public:
	EmuCheatsView(Base::Window &win);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode{};

	void loadCheatItems() override;

public:
	EmuEditCheatListView(Base::Window &win);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem code{};
	MdCheat *cheat{};

	void renamed(const char *str) override;

public:
	EmuEditCheatView(Base::Window &win, MdCheat &cheat);
};
