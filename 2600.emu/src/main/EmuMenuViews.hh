#pragma once
#include <emuframework/OptionView.hh>
#include <emuframework/MenuView.hh>
#include <stella/emucore/Console.hxx>
#include "OSystem.hxx"

static constexpr uint TV_PHOSPHOR_AUTO = 2;
extern Byte1Option optionTVPhosphor, optionVideoSystem;
extern Console *console;
extern Properties currGameProps;
extern OSystem osystem;
extern bool p1DiffB, p2DiffB, vcsColor;

class SystemOptionView : public OptionView
{
	MultiChoiceSelectMenuItem tvPhosphor
	{
		"Simulate TV Phosphor",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionTVPhosphor.val = val;
			if(!EmuSystem::gameIsRunning())
			{
				return;
			}

			// change runtime phosphor value
			bool usePhosphor = false;
			if((int)optionTVPhosphor == TV_PHOSPHOR_AUTO)
			{
				usePhosphor = currGameProps.get(Display_Phosphor) == "YES";
			}
			else
			{
				usePhosphor = optionTVPhosphor;
			}
			bool phospherInUse = console->properties().get(Display_Phosphor) == "YES";
			logMsg("Phosphor effect %s", usePhosphor ? "on" : "off");
			if(usePhosphor != phospherInUse)
			{
				logMsg("toggling phoshpor on console");
				console->togglePhosphor();
			}
		}
	};

	void tvPhosphorInit()
	{
		static const char *str[] =
		{
			"Off", "On", "Auto"
		};
		if(optionTVPhosphor > 2)
			optionTVPhosphor = 2;
		tvPhosphor.init(str, int(optionTVPhosphor), sizeofArray(str));
	}

	MultiChoiceSelectMenuItem videoSystem
	{
		"Video System",
		[](MultiChoiceMenuItem &, View &, int val)
		{
			optionVideoSystem = val;
		}
	};

	void videoSystemInit()
	{
		static const char *str[] =
		{
			"Auto", "NTSC", "PAL", "SECAM", "NTSC 50", "PAL 60", "SECAM 60"
		};
		assert(optionVideoSystem < (int)sizeofArray(str));
		videoSystem.init(str, optionVideoSystem, sizeofArray(str));
	}

public:
	SystemOptionView(Base::Window &win): OptionView(win) {}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		OptionView::loadVideoItems(item, items);
		tvPhosphorInit(); item[items++] = &tvPhosphor;
		videoSystemInit(); item[items++] = &videoSystem;
	}
};

class VCSSwitchesView : public TableView
{
	MenuItem *item[4]{};

	TextMenuItem softReset
	{
		"Soft Reset",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *new YesNoAlertView{window()};
				ynAlertView.init("Really Soft Reset Game?", !e.isPointer());
				ynAlertView.onYes() =
					[](const Input::Event &e)
					{
						Event &ev = osystem.eventHandler().event();
						ev.clear();
						ev.set(Event::ConsoleReset, 1);
						console->switches().update();
						TIA& tia = console->tia();
						tia.update();
						ev.set(Event::ConsoleReset, 0);
						startGameFromMenu();
					};
				modalViewController.pushAndShow(ynAlertView);
			}
		}
	};

	BoolMenuItem diff1
	{
		"Left (P1) Difficulty",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			p1DiffB = item.on;
		}
	};

	BoolMenuItem diff2
	{
		"Right (P2) Difficulty",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			p2DiffB = item.on;
		}
	};

	BoolMenuItem color
	{
		"Color",
		[this](BoolMenuItem &item, View &, const Input::Event &e)
		{
			item.toggle(*this);
			vcsColor = item.on;
		}
	};

public:
	VCSSwitchesView(Base::Window &win): TableView{"Switches", win} {}

	void init(bool highlightFirst)
	{
		uint i = 0;
		diff1.init("A", "B", p1DiffB); item[i++] = &diff1;
		diff2.init("A", "B", p2DiffB); item[i++] = &diff2;
		color.init(vcsColor); item[i++] = &color;
		softReset.init(); item[i++] = &softReset;
		assert(i <= sizeofArray(item));
		TableView::init(item, i, highlightFirst);
	}

	void onShow() override
	{
		diff1.set(p1DiffB, *this);
		diff2.set(p2DiffB, *this);
		color.set(vcsColor, *this);
	}

};

class SystemMenuView : public MenuView
{
private:
	TextMenuItem switches
	{
		"Console Switches",
		[this](TextMenuItem &, View &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &vcsSwitchesView = *new VCSSwitchesView{window()};
				vcsSwitchesView.init(!e.isPointer());
				viewStack.pushAndShow(vcsSwitchesView);
			}
		}
	};

public:
	SystemMenuView(Base::Window &win): MenuView{win} {}

	void onShow() override
	{
		MenuView::onShow();
		switches.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		name_ = appViewTitle();
		uint items = 0;
		loadFileBrowserItems(item, items);
		switches.init(); item[items++] = &switches;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		TableView::init(item, items, highlightFirst);
	}
};
