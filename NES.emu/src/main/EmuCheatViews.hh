#pragma once
#include <Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr, value, comp, ggCode;
	uint idx = 0;
	int type = 0;
	char *nameStr = nullptr;
	MenuItem *item[5] {nullptr};
	char addrStr[5] {0}, valueStr[3] {0}, compStr[3] {0}, ggCodeStr[9] {0};

	void syncCheat(const char *newName = nullptr);
	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(bool highlightFirst, int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG, addRAM;
	TextMenuItem cheat[EmuCheats::MAX];
	uchar addCheatType = 0;

	uint handleNameFromTextInput(const char *str);
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
