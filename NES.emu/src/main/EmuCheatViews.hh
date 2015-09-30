#pragma once
#include <emuframework/Cheats.hh>

namespace EmuCheats
{

static const uint MAX = 254;

}

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, comp{}, ggCode{};
	uint idx = 0;
	int type = 0;
	char *nameStr{};
	MenuItem *item[5]{};
	char addrStr[5]{}, valueStr[3]{}, compStr[3]{}, ggCodeStr[9]{};

	void syncCheat(const char *newName = nullptr);
	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGG{}, addRAM{};
	std::vector<TextMenuItem> cheat{};
	uchar addCheatType = 0;

	uint handleNameFromTextInput(const char *str);
	void loadAddCheatItems(std::vector<MenuItem*> &item) override;
	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	EditCheatListView(Base::Window &win);
};

class CheatsView : public BaseCheatsView
{
private:
	std::vector<BoolMenuItem> cheat{};

	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	CheatsView(Base::Window &win): BaseCheatsView(win) {}
};
