/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <ButtonConfigView.hh>
#include <inGameActionKeys.hh>
#include <main/EmuControls.hh>
extern KeyConfig<EmuControls::systemTotalKeys> keyConfig;

void BtnConfigMenuItem::init(const char *name, uint *btn, uint dev)
{
	DualTextMenuItem::init(name, Input::buttonName(dev, *btn));
	var_selfs(btn);
	var_selfs(dev);
}

void BtnConfigMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
	TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
	setColor(1., 1., 0.); // yellow
	DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
}

void BtnConfigMenuItem::select(View *view, const InputEvent &e)
{
	btnSetView.onSet = this;
	btnSetView.place(Gfx::viewportRect());
	View::modalView = &btnSetView;
}

void BtnConfigMenuItem::setButton(const InputEvent &e)
{
	t2.setString(Input::buttonName(dev, e.button));
	t2.compile();
	logMsg("changing key mapping from %s to %s", Input::buttonName(dev, *btn), Input::buttonName(dev, e.button));
	*btn = e.button;
	buildKeyMapping();
}


void ButtonConfigView::inputEvent(const InputEvent &e)
{
	if(Config::envIsPS3 && e.pushed(Input::Ps3::SQUARE) && tbl.selected > 1)
	{
		bug_exit("TODO");
		btn[tbl.selected-2].setButton(InputEvent());
	}
	else
		BaseMenuView::inputEvent(e);
}

void ButtonConfigView::init(KeyCategory *cat,
		uint devType, bool highlightFirst)
{
	name_ = InputEvent::devTypeName(devType);
	logMsg("init button config view for %s", InputEvent::devTypeName(devType));
	var_selfs(cat);
	var_selfs(devType);

	uint i = 0;
	uint tblEntries = cat->keys + 1;
	text = new MenuItem*[tblEntries];
	btn = new BtnConfigMenuItem[cat->keys];
	reset.init("Unbind All"); text[i++] = &reset;
	reset.selectDelegate().bind<ButtonConfigView, &ButtonConfigView::resetHandler>(this);
	iterateTimes(cat->keys, i2)
	{
		btn[i2].init(cat->keyName[i2], &keyConfig.key(*cat, devType)[i2], devType);
		text[i++] = &btn[i2];
	}

	assert(i <= tblEntries);
	BaseMenuView::init(text, i, highlightFirst);
	btnSetView.init(devType);
}

void ButtonConfigView::deinit()
{
	logMsg("deinit ButtonConfigView");
	BaseMenuView::deinit();
	btnSetView.deinit();
	delete[] btn;
	delete[] text;
}

void ButtonConfigView::confirmUnbindKeysAlert(const InputEvent &e)
{
	keyConfig.unbindCategory(*cat, devType);
	buildKeyMapping();
	iterateTimes(cat->keys, i)
	{
		btn[i].t2.setString(Input::buttonName(btnSetView.devType, keyConfig.key(*cat, devType)[i]));
		btn[i].t2.compile();
	}
}

void ButtonConfigView::resetHandler(TextMenuItem &, const InputEvent &e)
{
	ynAlertView.init("Really unbind all keys in this category?", !e.isPointer());
	ynAlertView.onYesDelegate().bind<ButtonConfigView, &ButtonConfigView::confirmUnbindKeysAlert>(this);
	ynAlertView.place(Gfx::viewportRect());
	modalView = &ynAlertView;
}
