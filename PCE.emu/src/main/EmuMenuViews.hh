#pragma once
#include "OptionView.hh"

static int pceHuFsFilter(const char *name, int type);

class SystemOptionView : public OptionView
{
public:

	char sysCardPathStr[256] {0};
	TextMenuItem sysCardPath
	{
		"",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &biosSelectMenu = *menuAllocator.allocNew<BiosSelectMenu>("System Card", &::sysCardPath, pceHuFsFilter, window());
			biosSelectMenu.init(!e.isPointer());
			biosSelectMenu.onBiosChange() =
				[this]()
				{
					logMsg("set bios %s", ::sysCardPath);
					printBiosMenuEntryStr(sysCardPathStr);
					sysCardPath.compile();
				};
			viewStack.pushAndShow(biosSelectMenu, &menuAllocator);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		FsSys::cPath basenameTemp;
		string_printf(str, "System Card: %s", strlen(::sysCardPath) ? string_basename(::sysCardPath, basenameTemp) : "None set");
	}

	BoolMenuItem arcadeCard
	{
		"Arcade Card",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			optionArcadeCard = item.on;
		}
	},
	sixButtonPad
	{
		"6-button support",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle(*this);
			PCE_Fast::AVPad6Enabled[0] = item.on;
			PCE_Fast::AVPad6Enabled[1] = item.on;
			#ifdef CONFIG_VCONTROLS_GAMEPAD
			vController.gp.activeFaceBtns = item.on ? 6 : 2;
			vController.place();
			#endif
		}
	};

public:
	SystemOptionView(Base::Window &win):
		OptionView(win)
	{}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		OptionView::loadInputItems(item, items);
		sixButtonPad.init(PCE_Fast::AVPad6Enabled[0]); item[items++] = &sixButtonPad;
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		arcadeCard.init(optionArcadeCard); item[items++] = &arcadeCard;
		printBiosMenuEntryStr(sysCardPathStr);
		sysCardPath.init(sysCardPathStr); item[items++] = &sysCardPath;
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	SystemMenuView(Base::Window &win): MenuView(win) {}
};
