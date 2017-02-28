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
#include <imagine/util/math/int.hh>

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
		unbind = {"Unbind", &View::defaultFace};
		cancel = {"Cancel", &View::defaultFace};
		unbindB.x2 = 1;
	}
}
#endif

void ButtonConfigSetView::init(Input::Device &dev, const char *actionName, SetDelegate onSet)
{
	text = {str.data(), &View::defaultFace};
	this->dev = &dev;
	savedDev = nullptr;
	string_copy(actionStr, actionName);
	Input::setHandleVolumeKeys(true);
	Input::setKeyRepeat(false);
	onSetD = onSet;
}

ButtonConfigSetView::~ButtonConfigSetView()
{
	Input::setHandleVolumeKeys(false);
	Input::setKeyRepeat(true);
}

void ButtonConfigSetView::place()
{
	text.compile(renderer(), projP);

	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		unbind.compile(renderer(), projP);
		cancel.compile(renderer(), projP);

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

void ButtonConfigSetView::inputEvent(Input::Event e)
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
				auto attach = attachParams();
				dismiss();
				viewStack.popTo(rootIMView);
				auto &imdMenu = *new InputManagerDeviceView{attach, rootIMView, inputDevConf[d->idx]};
				imdMenu.setName(rootIMView.inputDevNameStr[d->idx]);
				rootIMView.pushAndShow(imdMenu, e);
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
	auto &r = renderer();
	r.setBlendMode(0);
	r.noTexProgram.use(r, projP.makeTranslate());
	r.setColor(.4, .4, .4, 1.);
	GeomRect::draw(r, viewFrame, projP);
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		r.setColor(.2, .2, .2, 1.);
		GeomRect::draw(r, unbindB, projP);
		GeomRect::draw(r, cancelB, projP);
	}
	#endif

	r.setColor(COLOR_WHITE);
	r.texAlphaProgram.use(r);
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(pointerUIIsInit())
	{
		unbind.draw(r, projP.unProjectRect(unbindB).pos(C2DO), C2DO, projP);
		cancel.draw(r, projP.unProjectRect(cancelB).pos(C2DO), C2DO, projP);
	}
	#endif
	text.draw(r, 0, 0, C2DO, projP);
}

void ButtonConfigSetView::onAddedToController(Input::Event e)
{
	if(e.isPointer())
		string_printf(str, "Push key to set:\n%s", actionStr.data());
	else
		string_printf(str, "Push key to set:\n%s\n\nTo unbind:\nQuickly push [Left] key twice in previous menu", actionStr.data());
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	if(e.isPointer())
	{
		initPointerUI();
	}
	#endif
}

void ButtonConfigView::BtnConfigMenuItem::draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(r, xPos, yPos, xSize, ySize, align, projP);
	r.setColor(1., 1., 0.); // yellow
	DualTextMenuItem::draw2ndText(r, xPos, yPos, xSize, ySize, align, projP);
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

ButtonConfigView::KeyNameStr ButtonConfigView::makeKeyNameStr(Input::Key key, const char *name)
{
	KeyNameStr str;
	if(strlen(name))
	{
		string_copy(str, name);
	}
	else
	{
		string_printf(str, "Key Code 0x%X", key);
	}
	return str;
}

void ButtonConfigView::onSet(Input::Event e, int keyToSet)
{
	auto conf = mutableConfForDeviceConf(*devConf);
	if(!conf)
		return;
	auto &keyEntry = conf->key(*cat)[keyToSet];
	logMsg("changing key mapping from %s (0x%X) to %s (0x%X)",
			devConf->dev->keyName(keyEntry), keyEntry, devConf->dev->keyName(e.button), e.button);
	keyEntry = e.button;
	auto &b = btn[keyToSet];
	b.str = makeKeyNameStr(e.button, devConf->dev->keyName(e.button));
	b.item.t2.compile(renderer(), projP);
	keyMapping.buildAll();
}

void ButtonConfigView::inputEvent(Input::Event e)
{
	if(e.pushed() && e.isDefaultLeftButton() && selected > 0)
	{
		auto durationSinceLastKeySet = leftKeyPushTime ? e.time - leftKeyPushTime : Input::Time{};
		leftKeyPushTime = e.time;
		if(durationSinceLastKeySet && durationSinceLastKeySet <= Input::Time::makeWithMSecs(500))
		{
			// unset key
			leftKeyPushTime = {};
			onSet(Input::Event(), selected-1);
			postDraw();
		}
	}
	else
	{
		TableView::inputEvent(e);
	}
}

ButtonConfigView::~ButtonConfigView()
{
	logMsg("deinit ButtonConfigView");
	delete[] btn;
}

ButtonConfigView::ButtonConfigView(ViewAttachParams attach, InputManagerView &rootIMView_, const KeyCategory *cat_, InputDeviceConfig &devConf_):
	TableView
	{
		cat_->name,
		attach,
		[this](const TableView &)
		{
			return 1 + cat->keys;
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			if(idx == 0)
				return reset;
			else
				return btn[idx-1].item;
		}
	},
	rootIMView{rootIMView_},
	reset
	{
		"Unbind All",
		[this](TextMenuItem &t, View &, Input::Event e)
		{
			auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really unbind all keys in this category?"};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					auto conf = mutableConfForDeviceConf(*devConf);
					if(!conf)
						return;
					conf->unbindCategory(*cat);
					iterateTimes(cat->keys, i)
					{
						string_copy(btn[i].str, devConf->dev->keyName(devConf->keyConf().key(*cat)[i]));
						btn[i].item.t2.compile(renderer(), projP);
					}
					keyMapping.buildAll();
				});
			modalViewController.pushAndShow(ynAlertView, e);
		}
	}
{
	logMsg("init button config view for %s", Input::Event::mapName(devConf_.dev->map()));
	cat = cat_;
	devConf = &devConf_;
	auto keyConfig = devConf_.keyConf();
	btn = new BtnConfigEntry[cat->keys];
	iterateTimes(cat->keys, i)
	{
		auto key = keyConfig.key(*cat)[i];
		btn[i].str = makeKeyNameStr(key, devConf_.dev->keyName(key));
		btn[i].item.t.setString(cat->keyName[i]);
		btn[i].item.setOnSelect(
			[this, i](DualTextMenuItem &item, View &, Input::Event e)
			{
				auto keyToSet = i;
				auto &btnSetView = *new ButtonConfigSetView{attachParams(), rootIMView};
				btnSetView.init(*devConf->dev, btn[keyToSet].item.t.str,
					[this, keyToSet](Input::Event e)
					{
						onSet(e, keyToSet);
					}
				);
				modalViewController.pushAndShow(btnSetView, e);
			});
	}
}
