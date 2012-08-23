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
		View::modalView = &ynAlertView;
	}
}

void colorHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	vcsColor = item.on;
}

void leftDiffHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	p1DiffB = item.on;
}

void rightDiffHandler(BoolMenuItem &item, const InputEvent &e)
{
	item.toggle();
	p2DiffB = item.on;
}

static class VCSSwitchesView : public BaseMenuView
{
	MenuItem *item[4] = {nullptr};
	TextMenuItem softReset;
	BoolMenuItem diff1, diff2;
	BoolMenuItem color;
public:
	constexpr VCSSwitchesView(): BaseMenuView("Switches")	{ }

	void init(bool highlightFirst)
	{
		uint i = 0;
		diff1.init("Left (P1) Difficulty", "A", "B", p1DiffB); item[i++] = &diff1;
		diff1.selectDelegate().bind<&leftDiffHandler>();
		diff2.init("Right (P2) Difficulty", "A", "B", p2DiffB); item[i++] = &diff2;
		diff2.selectDelegate().bind<&rightDiffHandler>();
		color.init("Color", vcsColor); item[i++] = &color;
		color.selectDelegate().bind<&colorHandler>();
		softReset.init("Soft Reset"); item[i++] = &softReset;
		softReset.selectDelegate().bind<&softResetHandler>();
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

class VCSMenuView : public MenuView
{
private:
	TextMenuItem switches;

	static void switchesHandler(TextMenuItem &, const InputEvent &e)
	{
		if(EmuSystem::gameIsRunning())
		{
			vcsSwitchesView.init(!e.isPointer());
			viewStack.pushAndShow(&vcsSwitchesView);
		}
	}

	MenuItem *item[STANDARD_ITEMS + 1];

public:

	void onShow()
	{
		MenuView::onShow();
		switches.active = EmuSystem::gameIsRunning();
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		switches.init("Console Switches"); item[items++] = &switches;
		switches.selectDelegate().bind<&switchesHandler>();
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
