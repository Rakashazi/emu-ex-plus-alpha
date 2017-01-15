#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "internal.hh"

class EmuInputOptionView : public TableView
{
	BoolMenuItem sixButtonPad
	{
		"6-button Gamepad",
		useSixButtonPad,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			bool on = item.flipBoolValue(*this);
			useSixButtonPad = on;
			EmuSystem::clearInputBuffers();
			#ifdef CONFIG_VCONTROLS_GAMEPAD
			vController.gp.activeFaceBtns = on ? 6 : 2;
			EmuControls::setupVControllerVars();
			vController.place();
			#endif
		}
	};

public:
	EmuInputOptionView(Base::Window &win):
		TableView
		{
			"Input Options",
			win,
			[this](const TableView &)
			{
				return 1;
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				return sixButtonPad;
			}
		}
	{}
};

class EmuSystemOptionView : public SystemOptionView
{
	char sysCardPathStr[256]{};

	TextMenuItem sysCardPath
	{
		sysCardPathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &biosSelectMenu = *new BiosSelectMenu{"System Card", &::sysCardPath,
				[this]()
				{
					logMsg("set bios %s", ::sysCardPath.data());
					printBiosMenuEntryStr(sysCardPathStr);
					sysCardPath.compile(projP);
				},
				hasHuCardExtension, window()};
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
		(bool)optionArcadeCard,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionArcadeCard = item.flipBoolValue(*this);
		}
	};

public:
	EmuSystemOptionView(Base::Window &win): SystemOptionView{win, true}
	{
		loadStockItems();
		item.emplace_back(&arcadeCard);
		printBiosMenuEntryStr(sysCardPathStr);
		item.emplace_back(&sysCardPath);
	}
};

View *EmuSystem::makeView(Base::Window &win, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(win);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(win);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(win);
		case ViewID::INPUT_OPTIONS: return new EmuInputOptionView(win);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(win);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(win);
		default: return nullptr;
	}
}
