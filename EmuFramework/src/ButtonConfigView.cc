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
#include "privateInput.hh"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/math/int.hh>
#include <imagine/util/format.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

class KeyConflictAlertView : public AlertView
{
public:
	struct Context
	{
		Input::Key mapKey;
		int keyToSet;
		const KeyCategory *conflictCat{};
		int conflictKey;
	};

	Context ctx;

	KeyConflictAlertView(ViewAttachParams attach, UTF16Convertible auto &&label):
		AlertView(attach, IG_forward(label), 3)
	{
		setItem(2, "Cancel", [](){});
	}
};

bool ButtonConfigSetView::pointerUIIsInit()
{
	return unbindB.x != unbindB.x2;
}

void ButtonConfigSetView::initPointerUI()
{
	if(!pointerUIIsInit())
	{
		logMsg("init pointer UI elements");
		waitForDrawFinished();
		unbind = {"Unbind", &defaultFace()};
		cancel = {"Cancel", &defaultFace()};
		unbindB.x2 = 1;
	}
}

ButtonConfigSetView::ButtonConfigSetView(ViewAttachParams attach,
	InputManagerView &rootIMView, Input::Device &dev, std::string_view actionName,
	SetDelegate onSet):
		View{attach},
		text{&defaultFace()},
		onSetD{onSet},
		dev{dev},
		rootIMView{rootIMView},
		actionStr{actionName} {}

void ButtonConfigSetView::place()
{
	text.compile(renderer(), projP);

	if(pointerUIIsInit())
	{
		unbind.compile(renderer(), projP);
		cancel.compile(renderer(), projP);

		IG::WindowRect btnFrame;
		btnFrame.setPosRel(viewRect().pos(LB2DO), IG::makeEvenRoundedUp((int)projP.projectYSize(unbind.nominalHeight()*2)), LB2DO);
		unbindB = btnFrame;
		unbindB.x = (viewRect().xSize()/2)*0;
		unbindB.x2 = (viewRect().xSize()/2)*1;
		cancelB = btnFrame;
		cancelB.x = (viewRect().xSize()/2)*1;
		cancelB.x2 = (viewRect().xSize()/2)*2;
	}
}

bool ButtonConfigSetView::inputEvent(const Input::Event &e)
{
	return visit(overloaded
	{
		[&](const Input::MotionEvent &motionEv)
		{
			if(!Config::Input::POINTING_DEVICES || !motionEv.isAbsolute())
				return false;
			if(!pointerUIIsInit())
			{
				initPointerUI();
				place();
				postDraw();
				return true;
			}
			else if(pointerUIIsInit() && motionEv.released())
			{
				if(unbindB.overlaps(motionEv.pos()))
				{
					logMsg("unbinding key");
					auto onSet = onSetD;
					dismiss();
					onSet(Input::KeyEvent{});
					return true;
				}
				else if(cancelB.overlaps(motionEv.pos()))
				{
					dismiss();
					return true;
				}
			}
			return false;
		},
		[&](const Input::KeyEvent &keyEv)
		{
			if(keyEv.pushed())
			{
				auto d = keyEv.device();
				if(d != &dev)
				{
					if(d == savedDev)
					{
						app().unpostMessage();
						auto &rootIMView = this->rootIMView;
						popTo(rootIMView);
						rootIMView.pushAndShowDeviceView(*d, e);
					}
					else
					{
						savedDev = d;
						app().postMessage(7, false,
							fmt::format("You pushed a key from device:\n{}\nPush another from it to open its config menu",
							inputDevData(*d).displayName));
						postDraw();
					}
					return true;
				}
				auto onSet = onSetD;
				dismiss();
				onSet(keyEv);
				return true;
			}
			return false;
		}
	}, e);
}

void ButtonConfigSetView::draw(Gfx::RendererCommands &__restrict__ cmds)
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::OFF);
	basicEffect.disableTexture(cmds);
	cmds.setColor(.4, .4, .4, 1.);
	GeomRect::draw(cmds, viewRect(), projP);
	if(pointerUIIsInit())
	{
		cmds.setColor(.2, .2, .2, 1.);
		GeomRect::draw(cmds, unbindB, projP);
		GeomRect::draw(cmds, cancelB, projP);
	}

	cmds.set(ColorName::WHITE);
	basicEffect.enableAlphaTexture(cmds);
	if(pointerUIIsInit())
	{
		unbind.draw(cmds, projP.unProjectRect(unbindB).pos(C2DO), C2DO, projP);
		cancel.draw(cmds, projP.unProjectRect(cancelB).pos(C2DO), C2DO, projP);
	}
	text.draw(cmds, {}, C2DO, projP);
}

void ButtonConfigSetView::onAddedToController(ViewController *, const Input::Event &e)
{
	if(e.motionEvent())
		text.resetString(fmt::format("Push key to set:\n{}", actionStr));
	else
		text.resetString(fmt::format("Push key to set:\n{}\n\nTo unbind:\nQuickly push [Left] key twice in previous menu", actionStr));
	if(e.motionEvent())
	{
		initPointerUI();
	}
}

void ButtonConfigView::BtnConfigMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	MenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	DualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, Gfx::color(Gfx::ColorName::YELLOW));
}

static std::pair<const KeyCategory *, int> findCategoryAndKeyInConfig(EmuApp &app, Input::Key key,
	InputDeviceConfig &devConf, const KeyCategory *skipCat, int skipIdx_)
{
	for(auto &cat : app.inputControlCategories())
	{
		auto keyPtr = devConf.keyConf().key(cat);
		int skipIdx = -1;
		if(skipCat && skipCat == &cat)
		{
			skipIdx = skipIdx_;
		}
		for(auto k : iotaCount(cat.keys()))
		{
			if((int)k != skipIdx && keyPtr[k] == key)
			{
				return {&cat, k};
			}
		}
	}
	return {};
}

std::string ButtonConfigView::makeKeyNameStr(Input::Key key, std::string_view name)
{
	if(name.size())
	{
		return std::string{name};
	}
	else
	{
		return fmt::format("Key Code {:#X}", key);
	}
}

void ButtonConfigView::onSet(Input::Key mapKey, int keyToSet)
{
	if(!devConf->setKey(app(), mapKey, *cat, keyToSet))
		return;
	auto &b = btn[keyToSet];
	b.set2ndName(makeKeyNameStr(mapKey, devConf->device().keyName(mapKey)));
	b.compile2nd(renderer(), projP);
	devConf->buildKeyMap();
}

bool ButtonConfigView::inputEvent(const Input::Event &e)
{
	if(e.keyEvent() && e.asKeyEvent().pushed(Input::DefaultKey::LEFT) && selected > 0)
	{
		auto &keyEv = e.asKeyEvent();
		auto durationSinceLastKeySet = leftKeyPushTime.count() ? keyEv.time() - leftKeyPushTime : Input::Time{};
		leftKeyPushTime = keyEv.time();
		if(durationSinceLastKeySet.count() && durationSinceLastKeySet <= IG::Milliseconds(500))
		{
			// unset key
			leftKeyPushTime = {};
			onSet(0, selected-1);
			postDraw();
		}
		return true;
	}
	else
	{
		return TableView::inputEvent(e);
	}
}

ButtonConfigView::ButtonConfigView(ViewAttachParams attach, InputManagerView &rootIMView_, const KeyCategory &cat_, InputDeviceConfig &devConf_):
	TableView
	{
		cat_.name,
		attach,
		[this](const TableView &)
		{
			return 1 + cat->keys();
		},
		[this](const TableView &, size_t idx) -> MenuItem&
		{
			if(idx == 0)
				return reset;
			else
				return btn[idx-1];
		}
	},
	rootIMView{rootIMView_},
	reset
	{
		"Unbind All", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really unbind all keys in this category?");
			ynAlertView->setOnYes(
				[this]()
				{
					auto conf = devConf->makeMutableKeyConf(app());
					if(!conf)
						return;
					conf->unbindCategory(*cat);
					for(auto i : iotaCount(cat->keys()))
					{
						btn[i].set2ndName(devConf->device().keyName(devConf->keyConf().key(*cat)[i]));
						btn[i].compile2nd(renderer(), projP);
					}
					devConf->buildKeyMap();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	}
{
	logMsg("init button config view for %s", Input::KeyEvent::mapName(devConf_.device().map()).data());
	cat = &cat_;
	devConf = &devConf_;
	auto keyConfig = devConf_.keyConf();
	btn = std::make_unique<BtnConfigMenuItem[]>(cat_.keys());
	for(int i : iotaCount(cat_.keys()))
	{
		auto key = keyConfig.key(cat_)[i];
		btn[i] =
		{
			cat_.keyName[i],
			makeKeyNameStr(key, devConf_.device().keyName(key)),
			&defaultFace(),
			[this, keyToSet = i](const Input::Event &e)
			{
				auto btnSetView = makeView<ButtonConfigSetView>(rootIMView,
					devConf->device(), cat->keyName[keyToSet],
					[this, keyToSet](const Input::KeyEvent &e)
					{
						auto mapKey = e.mapKey();
						if(mapKey)
						{
							auto [conflictCat, conflictKey] = findCategoryAndKeyInConfig(app(), mapKey, *devConf, cat, keyToSet);
							if(conflictCat)
							{
								// prompt to resolve key conflict
								auto alertView = makeView<KeyConflictAlertView>(
									fmt::format("Key \"{}\" already used for action \"{}\", unbind it before setting?",
									devConf->device().keyName(mapKey),
									conflictCat->keyName[conflictKey]));
								alertView->ctx = {mapKey, keyToSet, conflictCat, conflictKey};
								alertView->setItem(0, "Yes",
									[this, ctx = &alertView->ctx]()
									{
										if(ctx->conflictCat == this->cat)
											onSet(0, ctx->conflictKey);
										else
										{
											devConf->setKey(app(), 0, *ctx->conflictCat, ctx->conflictKey);
										}
										onSet(ctx->mapKey, ctx->keyToSet);
									});
								alertView->setItem(1, "No",
									[this, ctx = &alertView->ctx]()
									{
										onSet(ctx->mapKey, ctx->keyToSet);
									});
								pushAndShowModal(std::move(alertView), e);
								return;
							}
						}
						onSet(mapKey, keyToSet);
					});
				pushAndShowModal(std::move(btnSetView), e);
			}
		};
	}
}

}
