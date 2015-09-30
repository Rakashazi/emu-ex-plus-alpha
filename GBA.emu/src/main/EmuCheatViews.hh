#pragma once
#include <emuframework/Cheats.hh>
#include <vector>

namespace EmuCheats
{

static const uint MAX = 100;

}

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem code{};
	uint idx = 0;
	MenuItem *item[5]{};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addGS12CBCode{}, addGS3Code{};
	std::vector<TextMenuItem> cheat{};

	void loadAddCheatItems(std::vector<MenuItem*> &item) override;
	void loadCheatItems(std::vector<MenuItem*> &item) override;
	void addNewCheat(int isGSv3);

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
