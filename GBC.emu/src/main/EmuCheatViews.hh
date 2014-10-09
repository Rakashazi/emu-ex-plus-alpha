#pragma once
#include <emuframework/Cheats.hh>
#include <main/Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem ggCode;
	GbcCheat *cheat = nullptr;
	MenuItem *item[3] {nullptr};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(bool highlightFirst, GbcCheat &cheat);
};

extern SystemEditCheatView editCheatView;

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGGGS;
	TextMenuItem cheat[EmuCheats::MAX];

	void loadAddCheatItems(MenuItem *item[], uint &items) override;
	void loadCheatItems(MenuItem *item[], uint &items) override;

public:
	EditCheatListView(Base::Window &win);
};

class CheatsView : public BaseCheatsView
{
private:
	BoolMenuItem cheat[EmuCheats::MAX];

	void loadCheatItems(MenuItem *item[], uint &i) override;

public:
	CheatsView(Base::Window &win): BaseCheatsView(win) {}
};
