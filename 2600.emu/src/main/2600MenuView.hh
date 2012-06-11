#pragma once

#include "MenuView.hh"

void softResetConfirmAlert(const InputEvent &e)
{
	Event &ev = osystem.eventHandler().event();
	ev.clear();
	ev.set(Event::ConsoleReset, 1);
	console->switches().update();
	TIA& tia = console->tia();
	tia.update();
	ev.set(Event::ConsoleReset, 0);
	startGameFromMenu();
}

void softResetHandler(TextMenuItem &, const InputEvent &e)
{
	if(EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Soft Reset Game?", !e.isPointer());
		ynAlertView.onYesDelegate().bind<&softResetConfirmAlert>();
		ynAlertView.place(Gfx::viewportRect());
		modalView = &ynAlertView;
	}
}

struct DifficultyMenuItem : public BoolTextMenuItem
{
	constexpr DifficultyMenuItem(): BoolTextMenuItem(), left(0) { }
	bool left;
	void init(bool left)
	{
		var_selfs(left);
		BoolTextMenuItem::init(left ? "Left (P1) Difficulty" : "Right (P2) Difficulty",
				"A", "B", left ? p1DiffB : p2DiffB);
	}

	void refreshActive()
	{
		if(left)
			set(p1DiffB);
		else
			set(p2DiffB);
	}

	void select(View *view, const InputEvent &e)
	{
		toggle();
		if(left)
		{
			p1DiffB = on;
		}
		else
		{
			p2DiffB = on;
		}
	}
};

struct ColorBWMenuItem : public BoolMenuItem
{
	constexpr ColorBWMenuItem() { }
	void init() { BoolMenuItem::init("Color", vcsColor); }

	void refreshActive()
	{
		set(vcsColor);
	}

	void select(View *view, const InputEvent &e)
	{
		toggle();
		vcsColor = on;
	}
};

static class VCSSwitchesView : public BaseMenuView
{
	MenuItem *item[4] = {nullptr};
	TextMenuItem softReset;
	DifficultyMenuItem diff1, diff2;
	ColorBWMenuItem color;
public:
	constexpr VCSSwitchesView(): BaseMenuView("Switches")	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		diff1.init(1); item[i++] = &diff1;
		diff2.init(0); item[i++] = &diff2;
		color.init(); item[i++] = &color;
		softReset.init("Soft Reset"); item[i++] = &softReset;
		softReset.selectDelegate().bind<&softResetHandler>();
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}

	void onShow()
	{
		diff1.refreshActive();
		diff2.refreshActive();
		color.refreshActive();
	}

} vcsSwitchesView;

class VCSMenuView : public MenuView
{
private:
	struct SwitchesMenuItem : public TextMenuItem
	{
		void init()
		{
			TextMenuItem::init("Console Switches");
		}

		void refreshActive()
		{
			active = EmuSystem::gameIsRunning();
		}

		void select(View *view, const InputEvent &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				vcsSwitchesView.init(!e.isPointer());
				viewStack.pushAndShow(&vcsSwitchesView);
			}
		}
	} switches;

	MenuItem *item[STANDARD_ITEMS + 1];

public:

	void onShow()
	{
		MenuView::onShow();
		switches.refreshActive();
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
