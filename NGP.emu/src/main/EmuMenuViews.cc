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
	EmuSystemOptionView(Base::Window &win): SystemOptionView{win, true}
	{
		loadStockItems();
		item.emplace_back(&ngpLanguage);
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
