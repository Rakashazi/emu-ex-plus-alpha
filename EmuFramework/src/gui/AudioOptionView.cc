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

#include <emuframework/AudioOptionView.hh>
#include <emuframework/EmuApp.hh>
#include <format>

namespace EmuEx
{

AudioOptionView::AudioOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Audio Options", attach, item},
	snd
	{
		"Sound", attach,
		app().audio().isEnabled(),
		[this](BoolMenuItem &item)
		{
			app().audio().setEnabled(item.flipBoolValue(*this));
		}
	},
	soundDuringFastSlowMode
	{
		"Sound During Fast/Slow Mode", attach,
		app().audio().isEnabledDuringAltSpeed(),
		[this](BoolMenuItem &item)
		{
			app().audio().setEnabledDuringAltSpeed(item.flipBoolValue(*this));
		}
	},
	soundVolumeItem
	{
		{"100%", attach, {.id = 100}},
		{"50%",  attach, {.id = 50}},
		{"25%",  attach, {.id = 25}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 125>(attachParams(), e, "Input 0 to 125", "",
					[this](EmuApp &app, auto val)
					{
						app.audio().setMaxVolume(val);
						soundVolume.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	soundVolume
	{
		"Volume", attach,
		MenuId{app().audio().maxVolume()},
		soundVolumeItem,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(std::format("{}%", app().audio().maxVolume()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().audio().setMaxVolume(item.id); }
		},
	},
	soundBuffersItem
	{
		{"1", attach, {.id = 1}},
		{"2", attach, {.id = 2}},
		{"3", attach, {.id = 3}},
		{"4", attach, {.id = 4}},
		{"5", attach, {.id = 5}},
		{"6", attach, {.id = 6}},
		{"7", attach, {.id = 7}},
	},
	soundBuffers
	{
		"Buffer Size In Frames", attach,
		MenuId{app().audio().soundBuffers},
		soundBuffersItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().audio().soundBuffers = item.id; }
		},
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size", attach,
		app().audio().addSoundBuffersOnUnderrunSetting,
		[this](BoolMenuItem &item)
		{
			app().audio().addSoundBuffersOnUnderrunSetting = item.flipBoolValue(*this);
		}
	},
	audioRateItem
	{
		[&]
		{
			decltype(audioRateItem) items;
			items.emplace_back("Device Native", attach, [this](View &view)
			{
				app().audio().setRate(0);
				audioRate.setSelected(MenuId{app().audio().rate()});
				view.dismiss();
				return false;
			});
			auto setRateDel = [this](TextMenuItem &item) { app().audio().setRate(item.id); };
			items.emplace_back("22KHz", attach, setRateDel, MenuItem::Config{.id = 22050});
			items.emplace_back("32KHz", attach, setRateDel, MenuItem::Config{.id = 32000});
			items.emplace_back("44KHz", attach, setRateDel, MenuItem::Config{.id = 44100});
			if(app().audio().maxRate() >= 48000)
				items.emplace_back("48KHz", attach, setRateDel, MenuItem::Config{.id = 48000});
			return items;
		}()
	},
	audioRate
	{
		"Sound Rate", attach,
		MenuId{app().audio().rate()},
		audioRateItem
	},
	audioSoloMix
	{
		"Mix With Other Apps", attach,
		!app().audioManager().soloMix(),
		[this](BoolMenuItem &item)
		{
			app().audioManager().setSoloMix(!item.flipBoolValue(*this));
		}
	},
	apiItem
	{
		[this]()
		{
			ApiItemContainer items{};
			items.emplace_back("Auto", attachParams(), [this](View &view)
			{
				app().audio().setOutputAPI(Audio::Api::DEFAULT);
				doIfUsed(api, [&](auto &api){ api.setSelected(MenuId{app().audioManager().makeValidAPI()}); });
				view.dismiss();
				return false;
			});
			auto &audioManager = app().audioManager();
			for(auto desc: audioManager.audioAPIs())
			{
				items.emplace_back(desc.name, attachParams(), [this](TextMenuItem &item)
				{
					app().audio().setOutputAPI(Audio::Api(item.id.val));
				}, MenuItem::Config{.id = desc.api});
			}
			return items;
		}()
	},
	api
	{
		"Audio Driver", attach,
		MenuId{app().audioManager().makeValidAPI(app().audio().outputAPI())},
		apiItem
	}
{
	if(!customMenu)
	{
		loadStockItems();
	}
}

void AudioOptionView::loadStockItems()
{
	item.emplace_back(&snd);
	item.emplace_back(&soundDuringFastSlowMode);
	item.emplace_back(&soundVolume);
	if(!EmuSystem::forcedSoundRate)
	{
		item.emplace_back(&audioRate);
	}
	item.emplace_back(&soundBuffers);
	item.emplace_back(&addSoundBuffersOnUnderrun);
	if constexpr(IG::Audio::Manager::HAS_SOLO_MIX)
	{
		item.emplace_back(&audioSoloMix);
	}
	doIfUsed(apiItem, [&](auto &apiItem)
	{
		if(apiItem.size() > 2)
		{
			item.emplace_back(&api);
		}
	});
}

}
