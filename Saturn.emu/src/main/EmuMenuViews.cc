#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include "internal.hh"

static constexpr uint MAX_SH2_CORES = 4;

class EmuSystemOptionView : public SystemOptionView
{
	char biosPathStr[256]{};

	TextMenuItem biosPath
	{
		biosPathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &biosSelectMenu = *new BiosSelectMenu{"BIOS", &::biosPath,
				[this]()
				{
					logMsg("set bios %s", ::biosPath.data());
					printBiosMenuEntryStr(biosPathStr);
					biosPath.compile(projP);
				},
				hasBIOSExtension, window()};
			viewStack.pushAndShow(biosSelectMenu, e);
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "BIOS: %s", strlen(::biosPath.data()) ? FS::basename(::biosPath).data() : "None set");
	}

	StaticArrayList<TextMenuItem, MAX_SH2_CORES> sh2CoreItem{};

	MultiChoiceMenuItem sh2Core
	{
		"SH2",
		[]() -> uint
		{
			iterateTimes(std::min(SH2Cores, MAX_SH2_CORES), i)
			{
				if(SH2CoreList[i]->id == yinit.sh2coretype)
					return i;
			}
			return 0;
		}(),
		sh2CoreItem
	};

public:
	EmuSystemOptionView(Base::Window &win): SystemOptionView{win, true}
	{
		loadStockItems();
		if(SH2Cores > 1)
		{
			iterateTimes(std::min(SH2Cores, MAX_SH2_CORES), i)
			{
				int id = SH2CoreList[i]->id;
				sh2CoreItem.emplace_back(SH2CoreList[i]->Name,
					[id]()
					{
						yinit.sh2coretype = id;
						optionSH2Core = id;
					});
			}
			item.emplace_back(&sh2Core);
		}
		printBiosMenuEntryStr(biosPathStr);
		item.emplace_back(&biosPath);
	}
};

View *EmuSystem::makeView(Base::Window &win, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(win);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(win);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(win);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(win);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(win);
		default: return nullptr;
	}
}
