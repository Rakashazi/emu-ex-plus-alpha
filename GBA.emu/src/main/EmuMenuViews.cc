#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "internal.hh"
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>

class ConsoleOptionView : public TableView
{
	TextMenuItem rtcItem[3]
	{
		{"Auto", [](){ setRTCEmulation(RTC_EMU_AUTO); }},
		{"Off", [](){ setRTCEmulation(RTC_EMU_OFF); }},
		{"On", [](){ setRTCEmulation(RTC_EMU_ON); }},
	};

	MultiChoiceMenuItem rtc
	{
		"RTC Emulation",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return rtcIsEnabled() ? "On" : "Off";
			}
			else
				return nullptr;
		},
		optionRtcEmulation,
		rtcItem
	};

	static void setRTCEmulation(uint val)
	{
		EmuSystem::sessionOptionSet();
		optionRtcEmulation = val;
		setRTC(val);
	}

	std::array<MenuItem*, 1> menuItem
	{
		&rtc
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class CustomSystemActionsView : public EmuSystemActionsView
{
	TextMenuItem options
	{
		"Console Options",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				pushAndShow(makeView<ConsoleOptionView>(), e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&options);
		loadStandardItems();
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}
