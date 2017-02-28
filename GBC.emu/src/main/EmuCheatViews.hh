#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

class EmuCheatsView : public BaseCheatsView
{
private:
	void loadCheatItems() override;

public:
	EmuCheatsView(ViewAttachParams attach);
};

class EmuEditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS{};

	void loadCheatItems() override;

public:
	EmuEditCheatListView(ViewAttachParams attach);
};

class EmuEditCheatView : public BaseEditCheatView
{
private:
	DualTextMenuItem ggCode{};
	GbcCheat *cheat{};

	void renamed(const char *str) override;

public:
	EmuEditCheatView(ViewAttachParams attach, GbcCheat &cheat);
};
