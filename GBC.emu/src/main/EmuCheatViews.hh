#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

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
	TextMenuItem addGGGS{};

	void loadCheatItems() override;

public:
	EmuEditCheatListView(Base::Window &win);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};

	void renamed(const char *str) override;

public:
	EmuEditCheatView(Base::Window &win, GbcCheat &cheat);
};
