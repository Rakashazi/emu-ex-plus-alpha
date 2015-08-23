#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>

static bool isHuCardExtension(const char *name);

class SystemOptionView : public OptionView
{
public:

	char sysCardPathStr[256]{};
	TextMenuItem sysCardPath
	{
		"",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &biosSelectMenu = *new BiosSelectMenu{"System Card", &::sysCardPath, isHuCardExtension, window()};
			biosSelectMenu.init();
			biosSelectMenu.onBiosChange() =
				[this]()
				{
					logMsg("set bios %s", ::sysCardPath.data());
					printBiosMenuEntryStr(sysCardPathStr);
					sysCardPath.compile(projP);
				};
			viewStack.pushAndShow(biosSelectMenu, e);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System Card: %s", strlen(::sysCardPath.data()) ? FS::basename(::sysCardPath).data() : "None set");
	}

	BoolMenuItem arcadeCard
	{
		"Arcade Card",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			optionArcadeCard = item.on;
		}
	},
	sixButtonPad
	{
		"6-button Gamepad",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			item.toggle(*this);
			PCE_Fast::AVPad6Enabled[0] = item.on;
			PCE_Fast::AVPad6Enabled[1] = item.on;
			#ifdef CONFIG_VCONTROLS_GAMEPAD
			vController.gp.activeFaceBtns = item.on ? 6 : 2;
			EmuControls::setupVControllerVars();
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

class SystemMenuView : public MenuView
{
public:
	SystemMenuView(Base::Window &win): MenuView(win) {}
};
