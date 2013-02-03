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
#include <main/EmuConfig.hh>
#include <InputManagerView.hh>
#include <MsgPopup.hh>
extern ViewStack viewStack;
extern InputManagerView imMenu;
extern InputManagerDeviceView imdMenu;
extern MsgPopup popup;

void ButtonConfigSetView::init(Input::Device &dev)
{
	if(!Config::envIsPS3)
		text.init("Push a key to set", View::defaultFace);
	else
		text.init("Push a key to set\n\nUse Square in previous screen to unbind", View::defaultFace);
	#ifndef CONFIG_BASE_PS3
	unbind.init("Unbind", View::defaultFace);
	cancel.init("Cancel", View::defaultFace);
	#endif
	this->dev = &dev;
	savedDev = nullptr;
	Input::setHandleVolumeKeys(1);
}

void ButtonConfigSetView::deinit()
{
	savedDev = nullptr;
	text.deinit();
	#ifndef CONFIG_BASE_PS3
	unbind.deinit();
	cancel.deinit();
	#endif
	Input::setHandleVolumeKeys(0);
}

void ButtonConfigSetView::place()
{
	text.compile();

	#ifndef CONFIG_BASE_PS3
	unbind.compile();
	cancel.compile();

	Rect2<int> btnFrame;
	btnFrame.setPosRel(viewFrame.pos(LB2DO), Gfx::toIYSize(unbind.nominalHeight*2), LB2DO);
	unbindB = btnFrame;
	unbindB.x = (viewFrame.xSize()/2)*0;
	unbindB.x2 = (viewFrame.xSize()/2)*1;
	cancelB = btnFrame;
	cancelB.x = (viewFrame.xSize()/2)*1;
	cancelB.x2 = (viewFrame.xSize()/2)*2;
	#endif
}

void ButtonConfigSetView::inputEvent(const Input::Event &e)
{
	if(e.isPointer() && e.state == Input::RELEASED)
	{
		#ifndef CONFIG_BASE_PS3
		if(unbindB.overlaps(e.x, e.y))
		{
			logMsg("unbinding key");
			onSet.invoke(Input::Event());
			removeModalView();
		}
		else if(cancelB.overlaps(e.x, e.y))
		{
			removeModalView();
		}
		#endif
	}
	else if(!e.isPointer() && e.state == Input::PUSHED)
	{
		auto d = e.device;
		if(d != dev)
		{
			if(d == savedDev)
			{
				popup.clear();
				removeModalView();
				viewStack.popTo(&imMenu);
				imdMenu.init(1, inputDevConf[d->idx]);
				imdMenu.name_ = imMenu.inputDevNameStr[d->idx];
				viewStack.pushAndShow(&imdMenu);
			}
			else
			{
				savedDev = d;
				popup.printf(7, 0, "You pushed a key from device:\n%s\nPush another from it to open its config menu", imMenu.inputDevNameStr[d->idx]);
				Base::displayNeedsUpdate();
			}
			return;
		}
		onSet.invoke(e);
		removeModalView();
	}
}

void ButtonConfigSetView::draw()
{
	using namespace Gfx;
	setBlendMode(0);
	resetTransforms();
	setColor(.4, .4, .4, 1.);
	GeomRect::draw(viewFrame);
	#ifndef CONFIG_BASE_PS3
	setColor(.2, .2, .2, 1.);
	GeomRect::draw(unbindB);
	GeomRect::draw(cancelB);
	#endif

	setColor(COLOR_WHITE);
	#ifndef CONFIG_BASE_PS3
	unbind.draw(gXPos(unbindB, C2DO), gYPos(unbindB, C2DO), C2DO);
	cancel.draw(gXPos(cancelB, C2DO), gYPos(cancelB, C2DO), C2DO);
	#endif
	text.draw(0, 0, C2DO);
}

void ButtonConfigView::BtnConfigMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
	TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
	setColor(1., 1., 0.); // yellow
	DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
}

template <size_t S>
void uniqueCustomConfigName(char (&name)[S])
{
	if(customKeyConfig.isFull())
	{
		logWarn("custom key config list is full");
		return;
	}
	iterateTimes(MAX_CUSTOM_KEY_CONFIGS, i)
	{
		string_printf(name, "Custom %d", i+1);
		// Check if this name is free
		logMsg("checking %s", name);
		bool exists = 0;
		forEachInDLList(&customKeyConfig, e)
		{
			logMsg("against %s", e.name);
			if(string_equal(e.name, name))
			{
				exists = 1;
				break;
			}
		}
		if(!exists)
			break;
	}
	logMsg("unique custom key config name: %s", name);
}

static KeyConfig *mutableConfForDeviceConf(InputDeviceConfig &devConf)
{
	auto conf = devConf.mutableKeyConf();
	if(!conf)
	{
		logMsg("current config not mutable, creating one");
		if(customKeyConfig.isFull())
		{
			popup.postError("No space left for new key profiles, please delete one");
			return nullptr;
		}
		char name[96];
		uniqueCustomConfigName(name);
		conf = devConf.setKeyConfCopiedFromExisting(name);
		if(!conf)
		{
			popup.postError("Too many saved device settings, please delete one");
			return nullptr;
		}
		popup.printf(3, 0, "Automatically created profile: %s", conf->name);
	}
	return conf;
}

void ButtonConfigView::onSet(const Input::Event &e)
{
	auto conf = mutableConfForDeviceConf(*devConf);
	if(!conf)
		return;
	auto &keyEntry = conf->key(*cat)[keyToSet];
	logMsg("changing key mapping from %s to %s", Input::buttonName(devConf->dev->map(), keyEntry), Input::buttonName(devConf->dev->map(), e.button));
	keyEntry = e.button;
	btn[keyToSet].t2.setString(Input::buttonName(devConf->dev->map(), e.button));
	btn[keyToSet].t2.compile();
	keyMapping.buildAll();
}

void ButtonConfigView::onSelectElement(const GuiTable1D *, const Input::Event &e, uint i)
{
	if(i == 0)
		item[i]->select(this, e);
	else
	{
		keyToSet = i - 1;
		btnSetView2.init(*devConf->dev);
		btnSetView2.placeRect(Gfx::viewportRect());
		View::modalView = &btnSetView2;
	}
}

void ButtonConfigView::inputEvent(const Input::Event &e)
{
	/*if(Config::envIsPS3 && e.pushed(Input::Ps3::SQUARE) && tbl.selected > 1)
	{
		bug_exit("TODO");
		btn[tbl.selected-2].setButton(Input::Event());
	}
	else*/
		BaseMenuView::inputEvent(e);
}

void ButtonConfigView::init(const KeyCategory *cat,
	InputDeviceConfig &devConf, bool highlightFirst)
{
	name_ = cat->name;
	logMsg("init button config view for %s", Input::Event::mapName(devConf.dev->map()));
	var_selfs(cat);
	this->devConf = &devConf;
	auto keyConfig = devConf.keyConf();

	uint i = 0;
	uint tblEntries = cat->keys + 1;
	text = new MenuItem*[tblEntries];
	btn = new BtnConfigMenuItem[cat->keys];
	reset.init("Unbind All"); text[i++] = &reset;
	reset.selectDelegate().bind<ButtonConfigView, &ButtonConfigView::resetHandler>(this);
	iterateTimes(cat->keys, i2)
	{
		btn[i2].init(cat->keyName[i2], Input::buttonName(devConf.dev->map(), keyConfig.key(*cat)[i2]));
		text[i++] = &btn[i2];
	}

	assert(i <= tblEntries);
	BaseMenuView::init(text, i, highlightFirst);
	btnSetView2.onSet.bind<ButtonConfigView, &ButtonConfigView::onSet>(this);
}

void ButtonConfigView::deinit()
{
	logMsg("deinit ButtonConfigView");
	BaseMenuView::deinit();
	delete[] btn;
	delete[] text;
}

void ButtonConfigView::confirmUnbindKeysAlert(const Input::Event &e)
{
	auto conf = mutableConfForDeviceConf(*devConf);
	if(!conf)
		return;
	conf->unbindCategory(*cat);
	iterateTimes(cat->keys, i)
	{
		btn[i].t2.setString(Input::buttonName(devConf->dev->map(), devConf->keyConf().key(*cat)[i]));
		btn[i].t2.compile();
	}
	keyMapping.buildAll();
}

void ButtonConfigView::resetHandler(TextMenuItem &, const Input::Event &e)
{
	ynAlertView.init("Really unbind all keys in this category?", !e.isPointer());
	ynAlertView.onYesDelegate().bind<ButtonConfigView, &ButtonConfigView::confirmUnbindKeysAlert>(this);
	ynAlertView.placeRect(Gfx::viewportRect());
	modalView = &ynAlertView;
}
