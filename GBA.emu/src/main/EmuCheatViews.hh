#pragma once
#include <Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem code;
	uint idx = 0;
	MenuItem *item[5] {nullptr};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(bool highlightFirst, int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGS12CBCode, addGS3Code;
	TextMenuItem cheat[EmuCheats::MAX];

	void loadAddCheatItems(MenuItem *item[], uint &items) override;
	void loadCheatItems(MenuItem *item[], uint &items) override;
	void addNewCheat(int isGSv3);

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
