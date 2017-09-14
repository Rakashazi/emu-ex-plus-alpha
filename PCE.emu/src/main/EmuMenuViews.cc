#include <emuframework/OptionView.hh>
#include <emuframework/EmuMainMenuView.hh>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

class CustomInputOptionView : public TableView
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
	CustomInputOptionView(ViewAttachParams attach):
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

class CustomSystemOptionView : public SystemOptionView
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
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&arcadeCard);
		printBiosMenuEntryStr(sysCardPathStr);
		item.emplace_back(&sysCardPath);
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::INPUT_OPTIONS: return new CustomInputOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new CustomSystemOptionView(attach);
		default: return nullptr;
	}
}
