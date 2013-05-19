#pragma once
#include <OptionView.hh>

class SystemOptionView : public OptionView
{
public:
	SystemOptionView() { }
};

#include "MenuView.hh"

static class VCSSwitchesView : public BaseMenuView
{
	MenuItem *item[4] {nullptr};

	TextMenuItem softReset
	{
		"Soft Reset",
		[this](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *allocModalView<YesNoAlertView>();
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
				View::addModalView(ynAlertView);
			}
		}
	};

	BoolMenuItem diff1
	{
		"Left (P1) Difficulty",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			p1DiffB = item.on;
		}
	};

	BoolMenuItem diff2
	{
		"Right (P2) Difficulty",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			p2DiffB = item.on;
		}
	};

	BoolMenuItem color
	{
		"Color",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			vcsColor = item.on;
		}
	};

public:
	VCSSwitchesView(): BaseMenuView("Switches")	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		diff1.init("A", "B", p1DiffB); item[i++] = &diff1;
		diff2.init("A", "B", p2DiffB); item[i++] = &diff2;
		color.init(vcsColor); item[i++] = &color;
		softReset.init(); item[i++] = &softReset;
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}

	void onShow()
	{
		diff1.set(p1DiffB);
		diff2.set(p2DiffB);
		color.set(vcsColor);
	}

} vcsSwitchesView;

class SystemMenuView : public MenuView
{
private:
	TextMenuItem switches
	{
		"Console Switches",
		[](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				vcsSwitchesView.init(!e.isPointer());
				viewStack.pushAndShow(&vcsSwitchesView);
			}
		}
	};

public:
	SystemMenuView() { }

	void onShow()
	{
		MenuView::onShow();
		switches.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		switches.init(); item[items++] = &switches;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
