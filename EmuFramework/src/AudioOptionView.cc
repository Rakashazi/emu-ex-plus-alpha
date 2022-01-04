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

#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAudio.hh>
#include "EmuOptions.hh"
#include <imagine/util/format.hh>

namespace EmuEx
{

TextMenuItem::SelectDelegate AudioOptionView::setRateDel(uint32_t val)
{
	assert(val <= optionSoundRate.defaultVal);
	return [this, val]() { app().setSoundRate(val); };
}

TextMenuItem::SelectDelegate AudioOptionView::setBuffersDel(int val)
{
	return [val]() { optionSoundBuffers = val; };
}

TextMenuItem::SelectDelegate AudioOptionView::setVolumeDel(uint8_t val)
{
	return [this, val]()
		{
			optionSoundVolume = val;
			audio->setVolume(val);
		};
}

AudioOptionView::AudioOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Audio Options", attach, item},
	snd
	{
		"Sound", &defaultFace(),
		(bool)soundIsEnabled(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			setSoundEnabled(item.flipBoolValue(*this));
			if(item.boolValue())
				audio->open(audioOutputAPI());
			else
				audio->close();
		}
	},
	soundDuringFastForward
	{
		"Sound During Fast Forward", &defaultFace(),
		(bool)soundDuringFastForwardIsEnabled(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			setSoundDuringFastForwardEnabled(item.flipBoolValue(*this));
		}
	},
	soundVolumeItem
	{
		{"100%", &defaultFace(), setVolumeDel(100)},
		{"50%",  &defaultFace(), setVolumeDel(50)},
		{"25%",  &defaultFace(), setVolumeDel(25)},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
					[this](EmuApp &app, auto val)
					{
						if(optionSoundVolume.isValidVal(val))
						{
							app.setSoundRate(val);
							soundVolume.setSelected(std::size(soundVolumeItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	soundVolume
	{
		"Volume", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(fmt::format("{}%", optionSoundVolume.val));
			return true;
		},
		[]()
		{
			switch(optionSoundVolume.val)
			{
				case 100: return 0;
				case 50: return 1;
				case 25: return 2;
				default: return 3;
			}
		}(),
		soundVolumeItem
	},
	soundBuffersItem
	{
		{"2", &defaultFace(), setBuffersDel(2)},
		{"3", &defaultFace(), setBuffersDel(3)},
		{"4", &defaultFace(), setBuffersDel(4)},
		{"5", &defaultFace(), setBuffersDel(5)},
		{"6", &defaultFace(), setBuffersDel(6)},
		{"7", &defaultFace(), setBuffersDel(7)},
		{"8", &defaultFace(), setBuffersDel(8)},
	},
	soundBuffers
	{
		"Buffer Size In Frames", &defaultFace(),
		(int)optionSoundBuffers - 2,
		soundBuffersItem
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size", &defaultFace(),
		(bool)optionAddSoundBuffersOnUnderrun,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionAddSoundBuffersOnUnderrun = item.flipBoolValue(*this);
			audio->setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
		}
	},
	audioRate
	{
		"Sound Rate", &defaultFace(),
		0,
		audioRateItem
	}
	,audioSoloMix
	{
		"Mix With Other Apps", &defaultFace(),
		!app().audioManager().soloMix(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			app().audioManager().setSoloMix(!item.flipBoolValue(*this));
		}
	}
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	,api
	{
		"Audio Driver", &defaultFace(),
		0,
		apiItem
	}
	#endif
{
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	apiItem.emplace_back("Auto", &defaultFace(),
		[this](View &view)
		{
			auto &audioManager = app().audioManager();
			optionAudioAPI = 0;
			auto defaultApi = audioManager.makeValidAPI();
			audio->open(defaultApi);
			api.setSelected(IG::findIndex(audioManager.audioAPIs(), defaultApi) + 1);
			view.dismiss();
			return false;
		});
	{
		auto &audioManager = app().audioManager();
		auto descs = audioManager.audioAPIs();
		for(auto desc: descs)
		{
			apiItem.emplace_back(desc.name, &defaultFace(),
				[this, api = desc.api]()
				{
					optionAudioAPI = (uint8_t)api;
					audio->open(api);
				});
		}
		api.setSelected(IG::findIndex(descs, audioManager.makeValidAPI(audioOutputAPI())) + 1);
	}
	#endif
	if(!customMenu)
	{
		loadStockItems();
	}
}

void AudioOptionView::loadStockItems()
{
	item.emplace_back(&snd);
	item.emplace_back(&soundDuringFastForward);
	item.emplace_back(&soundVolume);
	if(!optionSoundRate.isConst)
	{
		audioRateItem.clear();
		audioRateItem.emplace_back("Device Native", &defaultFace(),
			[this](View &view)
			{
				app().setSoundRate(optionSoundRate.defaultVal);
				updateRateItem();
				view.dismiss();
				return false;
			});
		audioRateItem.emplace_back("22KHz", &defaultFace(), setRateDel(22050));
		audioRateItem.emplace_back("32KHz", &defaultFace(), setRateDel(32000));
		audioRateItem.emplace_back("44KHz", &defaultFace(), setRateDel(44100));
		if(optionSoundRate.defaultVal >= 48000)
			audioRateItem.emplace_back("48KHz", &defaultFace(), setRateDel(48000));
		item.emplace_back(&audioRate);
		updateRateItem();
	}
	item.emplace_back(&soundBuffers);
	item.emplace_back(&addSoundBuffersOnUnderrun);
	if constexpr(IG::Audio::Manager::HAS_SOLO_MIX)
	{
		item.emplace_back(&audioSoloMix);
	}
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	if(apiItem.size() > 2)
	{
		item.emplace_back(&api);
	}
	#endif
}

void AudioOptionView::setEmuAudio(EmuAudio &audio_)
{
	audio = &audio_;
}

void AudioOptionView::updateRateItem()
{
	switch(optionSoundRate)
	{
		bcase 22050: audioRate.setSelected(1);
		bcase 32000: audioRate.setSelected(2);
		bdefault: audioRate.setSelected(3); // 44100
		bcase 48000: audioRate.setSelected(4);
	}
}

}
