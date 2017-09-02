#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() final;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS{};

	void loadCheatItems() final;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};

	void renamed(const char *str) final;

public:
	EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat);
};
