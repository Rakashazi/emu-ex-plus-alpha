#include <neopop.h>
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>

class EmuSystemOptionView : public SystemOptionView
{
	BoolMenuItem ngpLanguage
	{
		"NGP Language",
		language_english,
		"Japanese", "English",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			language_english = item.flipBoolValue(*this);
		}
	};

public:
	EmuSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&ngpLanguage);
	}
};

View *EmuSystem::makeView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::MAIN_MENU: return new MenuView(attach);
		case ViewID::VIDEO_OPTIONS: return new VideoOptionView(attach);
		case ViewID::AUDIO_OPTIONS: return new AudioOptionView(attach);
		case ViewID::SYSTEM_OPTIONS: return new EmuSystemOptionView(attach);
		case ViewID::GUI_OPTIONS: return new GUIOptionView(attach);
		default: return nullptr;
	}
}
