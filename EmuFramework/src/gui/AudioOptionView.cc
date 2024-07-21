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
#include <emuframework/viewUtils.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAudio.hh>
#include <format>

namespace EmuEx
{

AudioOptionView::AudioOptionView(ViewAttachParams attach, EmuAudio& audio_, bool customMenu):
	TableView{"Audio Options", attach, item},
	audio{audio_},
	snd
	{
		"Sound", attach,
		audio_.isEnabled(),
		[this](BoolMenuItem &item)
		{
			audio.setEnabled(item.flipBoolValue(*this));
		}
	},
	soundDuringFastSlowMode
	{
		"Sound During Fast/Slow Mode", attach,
		audio_.isEnabledDuringAltSpeed(),
		[this](BoolMenuItem &item)
		{
			audio.setEnabledDuringAltSpeed(item.flipBoolValue(*this));
		}
	},
	soundVolumeItem
	{
		{"100%", attach, {.id = 100}},
		{"50%",  attach, {.id = 50}},
		{"25%",  attach, {.id = 25}},
		{"Custom Value", attach, [this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<int, 0, 125>(attachParams(), e, "Input 0 to 125", "",
					[this](CollectTextInputView &, auto val)
					{
						audio.setMaxVolume(val);
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
		MenuId{audio_.maxVolume()},
		soundVolumeItem,
		{
			.onSetDisplayString = [this](auto, Gfx::Text& t)
			{
				t.resetString(std::format("{}%", audio.maxVolume()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { audio.setMaxVolume(item.id); }
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
		MenuId{audio_.soundBuffers},
		soundBuffersItem,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { audio.soundBuffers = item.id; }
		},
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size", attach,
		audio_.addSoundBuffersOnUnderrunSetting,
		[this](BoolMenuItem &item)
		{
			audio.addSoundBuffersOnUnderrunSetting = item.flipBoolValue(*this);
		}
	},
	audioRateItem
	{
		[&]
		{
			decltype(audioRateItem) items;
			items.emplace_back("Device Native", attach, [this](View &view)
			{
				audio.setRate(0);
				audioRate.setSelected(MenuId{audio.rate()}, view);
				view.dismiss();
				return false;
			});
			auto setRateDel = [this](TextMenuItem &item) { audio.setRate(item.id); };
			items.emplace_back("22KHz", attach, setRateDel, MenuItem::Config{.id = 22050});
			items.emplace_back("32KHz", attach, setRateDel, MenuItem::Config{.id = 32000});
			items.emplace_back("44KHz", attach, setRateDel, MenuItem::Config{.id = 44100});
			if(audio.maxRate() >= 48000)
				items.emplace_back("48KHz", attach, setRateDel, MenuItem::Config{.id = 48000});
			return items;
		}()
	},
	audioRate
	{
		"Sound Rate", attach,
		MenuId{audio_.rate()},
		audioRateItem
	},
	audioSoloMix
	{
		"Mix With Other Apps", attach,
		!audio_.manager.soloMix(),
		[this](BoolMenuItem &item)
		{
			audio.manager.setSoloMix(!item.flipBoolValue(*this));
		}
	},
	apiItem
	{
		[this]()
		{
			ApiItemContainer items{};
			items.emplace_back("Auto", attachParams(), [this](View &view)
			{
				audio.setOutputAPI(Audio::Api::DEFAULT);
				doIfUsed(api, [&](auto &api){ api.setSelected(MenuId{audio.manager.makeValidAPI()}); });
				view.dismiss();
				return false;
			});
			for(auto desc: audio.manager.audioAPIs())
			{
				items.emplace_back(desc.name, attachParams(), [this](TextMenuItem &item)
				{
					audio.setOutputAPI(Audio::Api(item.id.val));
				}, MenuItem::Config{.id = desc.api});
			}
			return items;
		}()
	},
	api
	{
		"Audio Driver", attach,
		MenuId{audio_.manager.makeValidAPI(audio_.outputAPI())},
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
