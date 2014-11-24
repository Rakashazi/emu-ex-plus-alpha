/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/ButtonConfigView.hh>
#include <emuframework/inGameActionKeys.hh>
#include <emuframework/InputManagerView.hh>
#include <emuframework/EmuApp.hh>

#ifdef CONFIG_INPUT_POINTING_DEVICES
bool ButtonConfigSetView::pointerUIIsInit()
{
	return unbindB.x != unbindB.x2;
}

void ButtonConfigSetView::initPointerUI()
{
	if(!pointerUIIsInit())
	{
		logMsg("init pointer UI elements");
		unbind.init("Unbind", View::defaultFace);
		cancel.init("Cancel", View::defaultFace);
		unbindB.x2 = 1;
	}
}
#endif

void ButtonConfigSetView::init(Input::Device &dev, const char *actionName, bool withPointerInput, SetDelegate onSet)
{
	if(withPointerInput)
		string_printf(str, "Push key to set:\n%s", actionName);
	else
		string_printf(str, "Push key to set:\n%s\n\nTo unbind:\nPush [Left] key in menu screen", actionName);
	text.init(str, View::defaultFace);
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(withPointerInput)
	{
		initPointerUI();
	}
	#endif
	this->dev = &dev;
	savedDev = nullptr;
	Input::setHandleVolumeKeys(true);
	Input::setKeyRepeat(false);
	onSetD = onSet;
}

void ButtonConfigSetView::deinit()
{
	savedDev = nullptr;
	text.deinit();
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		unbind.deinit();
		cancel.deinit();
		unbindB = {};
		cancelB = {};
	}
	#endif
	Input::setHandleVolumeKeys(false);
	Input::setKeyRepeat(true);
}

void ButtonConfigSetView::place()
{
	text.compile(projP);

	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		unbind.compile(projP);
		cancel.compile(projP);

		IG::WindowRect btnFrame;
		btnFrame.setPosRel(viewFrame.pos(LB2DO), IG::makeEvenRoundedUp(projP.projectYSize(unbind.nominalHeight*2)), LB2DO);
		unbindB = btnFrame;
		unbindB.x = (viewFrame.xSize()/2)*0;
		unbindB.x2 = (viewFrame.xSize()/2)*1;
		cancelB = btnFrame;
		cancelB.x = (viewFrame.xSize()/2)*1;
		cancelB.x2 = (viewFrame.xSize()/2)*2;
	}
	#endif
}

void ButtonConfigSetView::inputEvent(const Input::Event &e)
{
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(e.isPointer() && !pointerUIIsInit())
	{
		initPointerUI();
		place();
		postDraw();
	}
	else if(pointerUIIsInit() && e.isPointer() && e.state == Input::RELEASED)
	{
		if(unbindB.overlaps({e.x, e.y}))
		{
			logMsg("unbinding key");
			onSetD(Input::Event());
			dismiss();
		}
		else if(cancelB.overlaps({e.x, e.y}))
		{
			dismiss();
		}
	}
	else
	#endif
	if(!e.isPointer() && e.state == Input::PUSHED)
	{
		auto d = e.device;
		if(d != dev)
		{
			if(d == savedDev)
			{
				popup.clear();
				auto &rootIMView = this->rootIMView;
				auto &win = window();
				dismiss();
				viewStack.popTo(rootIMView);
				auto &imdMenu = *new InputManagerDeviceView{win, rootIMView};
				imdMenu.init(1, inputDevConf[d->idx]);
				imdMenu.name_ = rootIMView.inputDevNameStr[d->idx];
				rootIMView.pushAndShow(imdMenu);
			}
			else
			{
				savedDev = d;
				popup.printf(7, 0, "You pushed a key from device:\n%s\nPush another from it to open its config menu", rootIMView.inputDevNameStr[d->idx]);
				postDraw();
			}
			return;
		}
		onSetD(e);
		dismiss();
	}
}

void ButtonConfigSetView::draw()
{
	using namespace Gfx;
	setBlendMode(0);
	noTexProgram.use(projP.makeTranslate());
	setColor(.4, .4, .4, 1.);
	GeomRect::draw(viewFrame, projP);
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		setColor(.2, .2, .2, 1.);
		GeomRect::draw(unbindB, projP);
		GeomRect::draw(cancelB, projP);
	}
	#endif

	setColor(COLOR_WHITE);
	texAlphaProgram.use();
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		unbind.draw(projP.unProjectRect(unbindB).pos(C2DO), C2DO, projP);
		cancel.draw(projP.unProjectRect(cancelB).pos(C2DO), C2DO, projP);
	}
	#endif
	text.draw(0, 0, C2DO, projP);
}

void ButtonConfigView::BtnConfigMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	setColor(1., 1., 0.); // yellow
	DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align, projP);
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
		for(auto &e : customKeyConfig)
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

void ButtonConfigView::onSet(const Input::Event &e, int keyToSet)
{
	#ifdef BUTTONCONFIGVIEW_CHECK_SPURIOUS_EVENTS
	lastKeySetTime = e.time;
	#endif
	auto conf = mutableConfForDeviceConf(*devConf);
	if(!conf)
		return;
	auto &keyEntry = conf->key(*cat)[keyToSet];
	logMsg("changing key mapping from %s (0x%X) to %s (0x%X)",
			devConf->dev->keyName(keyEntry), keyEntry, devConf->dev->keyName(e.button), e.button);
	keyEntry = e.button;
	btn[keyToSet].t2.setString(devConf->dev->keyName(e.button));
	btn[keyToSet].t2.compile(projP);
	keyMapping.buildAll();
}

void ButtonConfigView::inputEvent(const Input::Event &e)
{
	#ifdef BUTTONCONFIGVIEW_CHECK_SPURIOUS_EVENTS
	if(e.pushed() && e.time && lastKeySetTime)
	{
		auto durationSinceLastKeySet = e.time - lastKeySetTime;
		if(durationSinceLastKeySet < Input::msToTime(100))
		{
			lastKeySetTime = 0;
			logMsg("possible spurious input event after key set, ignoring");
			return;
		}
	}
	#endif
	if(e.pushed() && e.isDefaultLeftButton() && selected > 0)
	{
		// unset key
		onSet(Input::Event(), selected-1);
		postDraw();
	}
	else
		TableView::inputEvent(e);
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
	reset.init(); text[i++] = &reset;
	iterateTimes(cat->keys, i2)
	{
		btn[i2].init(cat->keyName[i2], devConf.dev->keyName(keyConfig.key(*cat)[i2]));
		btn[i2].onSelect() =
			[this, i2](DualTextMenuItem &item, View &, const Input::Event &e)
			{
				auto keyToSet = i2;
				auto &btnSetView = *new ButtonConfigSetView{window(), rootIMView};
				btnSetView.init(*this->devConf->dev, btn[keyToSet].t.str, e.isPointer(),
					[this, keyToSet](const Input::Event &e)
					{
						onSet(e, keyToSet);
					}
				);
				modalViewController.pushAndShow(btnSetView);
			};
		text[i++] = &btn[i2];
	}

	assert(i <= tblEntries);
	TableView::init(text, i, highlightFirst);
}

void ButtonConfigView::deinit()
{
	logMsg("deinit ButtonConfigView");
	TableView::deinit();
	delete[] btn;
	delete[] text;
}

ButtonConfigView::ButtonConfigView(Base::Window &win, InputManagerView &rootIMView):
	TableView{win},
	rootIMView{rootIMView},
	reset
	{
		"Unbind All",
		[this](TextMenuItem &t, View &, const Input::Event &e)
		{
			auto &ynAlertView = *new YesNoAlertView{window()};
			ynAlertView.init("Really unbind all keys in this category?", !e.isPointer());
			ynAlertView.onYes() =
				[this](const Input::Event &e)
				{
					auto conf = mutableConfForDeviceConf(*devConf);
					if(!conf)
						return;
					conf->unbindCategory(*cat);
					iterateTimes(cat->keys, i)
					{
						btn[i].t2.setString(devConf->dev->keyName(devConf->keyConf().key(*cat)[i]));
						btn[i].t2.compile(projP);
					}
					keyMapping.buildAll();
				};
			modalViewController.pushAndShow(ynAlertView);
		}
	}
{}
