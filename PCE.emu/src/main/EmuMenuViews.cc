#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include <emuframework/EmuInput.hh>
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
			EmuControls::setActiveFaceButtons(on ? 6 : 2);
		}
	};

public:
	EmuInputOptionView(ViewAttachParams attach):
		TableView
		{
			"Input Options",
			attach,
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
					sysCardPath.compile(renderer(), projP);
				},
				hasHuCardExtension, attachParams()};
			pushAndShow(biosSelectMenu, e);
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
	EmuSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&arcadeCard);
		printBiosMenuEntryStr(sysCardPathStr);
		item.emplace_back(&sysCardPath);
	}
};

View *EmuSystem::makeView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(attach);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(attach);
		case ViewID::INPUT_OPTIONS: return new EmuInputOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(attach);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		default: return nullptr;
	}
}
