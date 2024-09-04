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
#include <emuframework/AppKeyCode.hh>
#include <emuframework/EmuApp.hh>
#include "InputManagerView.hh"
#include "../InputDeviceConfig.hh"
#include "../InputDeviceData.hh"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/variant.hh>
#include <imagine/logger/logger.h>
#include <format>

namespace EmuEx
{

constexpr SystemLogger log{"ButtonConfigView"};
constexpr int resetItemsSize = 2;

static std::string keyNames(MappedKeys keys, const Input::Device &dev)
{
	Input::KeyNameFlags flags{.basicModifiers = keys.size() > 1};
	std::string s{dev.keyString(keys[0], flags)};
	for(const auto &b : keys | std::ranges::views::drop(1))
	{
		s += " + ";
		s += dev.keyString(b, flags);
	}
	return s;
}

ButtonConfigView::ButtonConfigView(ViewAttachParams attach, InputManagerView &rootIMView_, const KeyCategory &cat_, InputDeviceConfig &devConf_):
	TableView
	{
		cat_.name,
		attach,
		[this](ItemMessage msg) -> ItemReply
		{
			return msg.visit(overloaded
			{
				[&](const ItemsMessage&) -> ItemReply { return resetItemsSize + cat.keys.size(); },
				[&](const GetItemMessage& m) -> ItemReply
				{
					if(m.idx == 0)
						return &resetDefaults;
					else if(m.idx == 1)
						return &reset;
					else
						return &btn[m.idx - resetItemsSize];
				},
			});
		}
	},
	rootIMView{rootIMView_},
	reset
	{
		"Unbind All", attach,
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really unbind all keys in this category?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						auto conf = devConf.makeMutableKeyConf(app());
						if(!conf)
							return;
						conf->unbindCategory(cat);
						updateKeyNames(*conf);
						devConf.buildKeyMap(app().inputManager);
					}
				}), e);
		}
	},
	resetDefaults
	{
		"Reset Defaults", attach,
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really reset all keys in this category to defaults?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						auto conf = devConf.mutableKeyConf(app().inputManager);
						if(!conf)
							return;
						conf->resetCategory(cat, app().inputManager.defaultConfig(devConf.device()));
						updateKeyNames(*conf);
						devConf.buildKeyMap(app().inputManager);
					}
				}), e);
		}
	},
	cat{cat_},
	devConf{devConf_}
{
	log.info("init button config view for {}", Input::KeyEvent::mapName(devConf_.device().map()));
	auto keyConfig = devConf_.keyConf(app().inputManager);
	btn = std::make_unique<DualTextMenuItem[]>(cat_.keys.size());
	for(auto &&[i, key]: enumerate(cat.keys))
	{
		btn[i] =
		{
			app().inputManager.toString(key),
			keyNames(keyConfig.get(key), devConf_.device()),
			attach,
			[this, keyIdxToSet = i](const Input::Event &e)
			{
				auto btnSetView = makeView<ButtonConfigSetView>(rootIMView,
					devConf.device(), app().inputManager.toString(cat.keys[keyIdxToSet]),
					[this, keyIdxToSet](const MappedKeys &val)
					{
						onSet(keyIdxToSet, val);
					});
				pushAndShowModal(std::move(btnSetView), e);
			}
		};
		btn[i].text2Color = Gfx::ColorName::YELLOW;
	}
}

void ButtonConfigView::onSet(int catIdx, MappedKeys mapKey)
{
	if(!devConf.setKey(app(), cat.keys[catIdx], mapKey))
		return;
	devConf.buildKeyMap(app().inputManager);
	auto &b = btn[catIdx];
	b.set2ndName(keyNames(mapKey, devConf.device()));
	b.place2nd();
}

bool ButtonConfigView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	if(e.keyEvent() && e.keyEvent()->pushed(Input::DefaultKey::LEFT) && selected >= resetItemsSize)
	{
		auto &keyEv = *e.keyEvent();
		auto durationSinceLastKeySet = hasTime(leftKeyPushTime) ? keyEv.time() - leftKeyPushTime : SteadyClockTime{};
		leftKeyPushTime = keyEv.time();
		if(durationSinceLastKeySet.count() && durationSinceLastKeySet <= Milliseconds(500))
		{
			// unset key
			leftKeyPushTime = {};
			onSet(selected - resetItemsSize, {});
			postDraw();
		}
		return true;
	}
	else
	{
		return TableView::inputEvent(e);
	}
}

void ButtonConfigView::updateKeyNames(const KeyConfig &conf)
{
	for(auto &&[i, key]: enumerate(cat.keys))
	{
		btn[i].set2ndName(keyNames(conf.get(key), devConf.device()));
		btn[i].place2nd();
	}
}

ButtonConfigSetView::ButtonConfigSetView(ViewAttachParams attach,
	InputManagerView &rootIMView, Input::Device &dev, std::string_view actionName,
	SetDelegate onSet):
		View{attach},
		text{attach.rendererTask, &defaultFace()},
		quads{attach.rendererTask, {.size = 3}},
		onSetD{onSet},
		dev{dev},
		rootIMView{rootIMView},
		actionStr{actionName} {}

bool ButtonConfigSetView::pointerUIIsInit() const
{
	return unbindB.x != unbindB.x2;
}

void ButtonConfigSetView::initPointerUI()
{
	if(pointerUIIsInit())
		return;
	log.info("init pointer UI elements");
	unbind = {renderer().mainTask, "Unbind", &defaultFace()};
	cancel = {renderer().mainTask, "Cancel", &defaultFace()};
	unbindB.x2 = 1;
}

void ButtonConfigSetView::place()
{
	text.compile({.alignment = Gfx::TextAlignment::center});
	using Quad = decltype(quads)::Type;
	auto map = quads.map();
	Quad{{.bounds = viewRect().as<int16_t>()}}.write(map, 0);
	if(pointerUIIsInit())
	{
		unbind.compile();
		cancel.compile();
		WRect btnFrame;
		btnFrame.setPosRel(viewRect().pos(LB2DO), unbind.nominalHeight() * 2, LB2DO);
		unbindB = btnFrame;
		unbindB.x = (viewRect().xSize()/2)*0;
		unbindB.x2 = (viewRect().xSize()/2)*1;
		cancelB = btnFrame;
		cancelB.x = (viewRect().xSize()/2)*1;
		cancelB.x2 = (viewRect().xSize()/2)*2;
		Quad{{.bounds = unbindB.as<int16_t>()}}.write(map, 1);
		Quad{{.bounds = cancelB.as<int16_t>()}}.write(map, 2);
	}
}

bool ButtonConfigSetView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	return e.visit(overloaded
	{
		[&](const Input::MotionEvent &motionEv)
		{
			if(!Config::Input::POINTING_DEVICES || !motionEv.isPointer())
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
					log.info("unbinding key");
					auto onSet = onSetD;
					dismiss();
					onSet(MappedKeys{});
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
			if(keyEv.pushed() && !keyEv.repeated())
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
							std::format("You pushed a key from device:\n{}\nPush another from it to open its config menu",
							inputDevData(*d).displayName));
						postDraw();
					}
					return true;
				}
				if(std::ranges::contains(pushedKeys, keyEv.key()))
				{
					return true;
				}
				if((std::ranges::contains(pushedKeys, Input::Keycode::GAME_L2) || std::ranges::contains(pushedKeys, Input::Keycode::GAME_R2)) &&
					(keyEv.key() == Input::Keycode::JS_LTRIGGER_AXIS || keyEv.key() == Input::Keycode::JS_RTRIGGER_AXIS))
				{
					log.info("ignoring trigger axis to avoid duplicate events since L2/R2 keys are pushed");
					return true;
				}
				pushedKeys.tryPushBack(keyEv.key());
			}
			else if(keyEv.released())
			{
				if(pushedKeys.size())
					finalize();
			}
			return true;
		}
	});
}

void ButtonConfigSetView::finalize()
{
	auto onSet = onSetD;
	auto mappedKeys = pushedKeys;
	dismiss();
	onSet(mappedKeys);
}

void ButtonConfigSetView::draw(Gfx::RendererCommands&__restrict__ cmds, ViewDrawParams) const
{
	using namespace IG::Gfx;
	auto &basicEffect = cmds.basicEffect();
	cmds.set(BlendMode::OFF);
	basicEffect.disableTexture(cmds);
	cmds.setColor({.4, .4, .4});
	cmds.setVertexArray(quads);
	cmds.drawQuad(0); // bg
	if(pointerUIIsInit())
	{
		cmds.setColor({.2, .2, .2});
		cmds.drawQuads(1, 2); // button bg
	}
	basicEffect.enableAlphaTexture(cmds);
	if(pointerUIIsInit())
	{
		unbind.draw(cmds, unbindB.pos(C2DO), C2DO, ColorName::WHITE);
		cancel.draw(cmds, cancelB.pos(C2DO), C2DO, ColorName::WHITE);
	}
	text.draw(cmds, viewRect().center(), C2DO, ColorName::WHITE);
}

void ButtonConfigSetView::onAddedToController(ViewController *, const Input::Event &e)
{
	if(e.motionEvent())
		text.resetString(std::format("Push up to 3 keys, release any to set:\n{}", actionStr));
	else
		text.resetString(std::format("Push up to 3 keys, release any to set:\n{}\n\nTo unbind:\nQuickly push [Left] key twice in previous menu", actionStr));
	if(e.motionEvent())
	{
		initPointerUI();
	}
}

}
