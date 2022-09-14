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
#include <imagine/util/format.hh>

namespace EmuEx
{

AudioOptionView::AudioOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Audio Options", attach, item},
	snd
	{
		"Sound", &defaultFace(),
		app().soundIsEnabled(),
		[this](BoolMenuItem &item)
		{
			app().setSoundEnabled(item.flipBoolValue(*this));
		}
	},
	soundDuringFastSlowMode
	{
		"Sound During Fast/Slow Mode", &defaultFace(),
		app().soundDuringFastSlowModeIsEnabled(),
		[this](BoolMenuItem &item)
		{
			app().setSoundDuringFastSlowModeEnabled(item.flipBoolValue(*this));
		}
	},
	soundVolumeItem
	{
		{"100%", &defaultFace(), setVolumeDel(), 100},
		{"50%",  &defaultFace(), setVolumeDel(), 50},
		{"25%",  &defaultFace(), setVolumeDel(), 25},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 100>(attachParams(), e, "Input 0 to 100", "",
					[this](EmuApp &app, auto val)
					{
						app.setSoundVolume(val);
						soundVolume.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		},
	},
	soundVolume
	{
		"Volume", &defaultFace(),
		[this](size_t idx, Gfx::Text &t)
		{
			t.resetString(fmt::format("{}%", app().soundVolume()));
			return true;
		},
		(MenuItem::Id)app().soundVolume(),
		soundVolumeItem
	},
	soundBuffersItem
	{
		{"1", &defaultFace(), setBuffersDel(), 1},
		{"2", &defaultFace(), setBuffersDel(), 2},
		{"3", &defaultFace(), setBuffersDel(), 3},
		{"4", &defaultFace(), setBuffersDel(), 4},
		{"5", &defaultFace(), setBuffersDel(), 5},
		{"6", &defaultFace(), setBuffersDel(), 6},
		{"7", &defaultFace(), setBuffersDel(), 7},
	},
	soundBuffers
	{
		"Buffer Size In Frames", &defaultFace(),
		(MenuItem::Id)app().soundBuffers(),
		soundBuffersItem
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size", &defaultFace(),
		app().addSoundBuffersOnUnderrun(),
		[this](BoolMenuItem &item)
		{
			app().setAddSoundBuffersOnUnderrun(item.flipBoolValue(*this));
		}
	},
	audioRate
	{
		"Sound Rate", &defaultFace(),
		0,
		audioRateItem
	},
	audioSoloMix
	{
		"Mix With Other Apps", &defaultFace(),
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
			items.emplace_back("Auto", &defaultFace(), [this](View &view)
			{
				app().setAudioOutputAPI(Audio::Api::DEFAULT);
				doIfUsed(api, [&](auto &api){ api.setSelected((MenuItem::Id)app().audioManager().makeValidAPI()); });
				view.dismiss();
				return false;
			});
			auto &audioManager = app().audioManager();
			for(auto desc: audioManager.audioAPIs())
			{
				items.emplace_back(desc.name, &defaultFace(), [this](TextMenuItem &item)
				{
					app().setAudioOutputAPI((Audio::Api)item.id());
				}, (MenuItem::Id)desc.api);
			}
			return items;
		}()
	},
	api
	{
		"Audio Driver", &defaultFace(),
		(MenuItem::Id)app().audioManager().makeValidAPI(app().audioOutputAPI()),
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
	if(app().canChangeSoundRate())
	{
		audioRateItem.clear();
		audioRateItem.emplace_back("Device Native", &defaultFace(),
			[this](View &view)
			{
				app().setSoundRate(0);
				audioRate.setSelected((MenuItem::Id)app().soundRate());
				view.dismiss();
				return false;
			});
		audioRateItem.emplace_back("22KHz", &defaultFace(), setRateDel(), 22050);
		audioRateItem.emplace_back("32KHz", &defaultFace(), setRateDel(), 32000);
		audioRateItem.emplace_back("44KHz", &defaultFace(), setRateDel(), 44100);
		if(app().soundRateMax() >= 48000)
			audioRateItem.emplace_back("48KHz", &defaultFace(), setRateDel(), 48000);
		item.emplace_back(&audioRate);
		audioRate.setSelected((MenuItem::Id)app().soundRate());
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

TextMenuItem::SelectDelegate AudioOptionView::setRateDel()
{
	return [this](TextMenuItem &item) { app().setSoundRate(item.id()); };
}

TextMenuItem::SelectDelegate AudioOptionView::setBuffersDel()
{
	return [this](TextMenuItem &item) { app().setSoundBuffers(item.id()); };
}

TextMenuItem::SelectDelegate AudioOptionView::setVolumeDel()
{
	return [this](TextMenuItem &item) { app().setSoundVolume(item.id()); };
}

}
