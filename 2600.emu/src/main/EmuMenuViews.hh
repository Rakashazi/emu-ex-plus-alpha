#pragma once
#include <OptionView.hh>

class SystemOptionView : public OptionView
{
public:
	constexpr SystemOptionView() { }
};

#include "MenuView.hh"

void softResetConfirmAlert(const Input::Event &e)
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

void softResetHandler(TextMenuItem &, const Input::Event &e)
{
	if(EmuSystem::gameIsRunning())
	{
		ynAlertView.init("Really Soft Reset Game?", !e.isPointer());
		ynAlertView.onYes().bind<&softResetConfirmAlert>();
		ynAlertView.placeRect(Gfx::viewportRect());
		View::modalView = &ynAlertView;
	}
}

void colorHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	vcsColor = item.on;
}

void leftDiffHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	p1DiffB = item.on;
}

void rightDiffHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	p2DiffB = item.on;
}

static class VCSSwitchesView : public BaseMenuView
{
	MenuItem *item[4] {nullptr};
	TextMenuItem softReset {"Soft Reset"};
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
		softReset.init(); item[i++] = &softReset;
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

class SystemMenuView : public MenuView
{
private:
	TextMenuItem switches {"Console Switches"};

	static void switchesHandler(TextMenuItem &, const Input::Event &e)
	{
		if(EmuSystem::gameIsRunning())
		{
			vcsSwitchesView.init(!e.isPointer());
			viewStack.pushAndShow(&vcsSwitchesView);
		}
	}

public:
	constexpr SystemMenuView() { }

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
		switches.selectDelegate().bind<&switchesHandler>();
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
