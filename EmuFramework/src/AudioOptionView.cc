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

static void setAudioRate(uint32_t rate)
{
	if(rate > optionSoundRate.defaultVal)
		return;
	optionSoundRate = rate;
	EmuSystem::configAudioPlayback(rate);
}

static void setSoundBuffers(int val)
{
	optionSoundBuffers = val;
}

static void setSoundVolume(uint8_t val, EmuAudio &audio)
{
	optionSoundVolume = val;
	audio.setVolume(val);
}

AudioOptionView::AudioOptionView(ViewAttachParams attach, bool customMenu):
	TableView{"Audio Options", attach, item},
	snd
	{
		"Sound",
		(bool)soundIsEnabled(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			setSoundEnabled(item.flipBoolValue(*this));
			if(item.boolValue())
				audio->open(appContext(), audioOutputAPI());
			else
				audio->close();
		}
	},
	soundDuringFastForward
	{
		"Sound During Fast Forward",
		(bool)soundDuringFastForwardIsEnabled(),
		[this](BoolMenuItem &item, Input::Event e)
		{
			setSoundDuringFastForwardEnabled(item.flipBoolValue(*this));
		}
	},
	soundVolumeItem
	{
		{"100%", [this]() { setSoundVolume(100, *audio); }},
		{"50%", [this]() { setSoundVolume(50, *audio); }},
		{"25%", [this]() { setSoundVolume(25, *audio); }},
		{"Custom Value",
			[this](Input::Event e)
			{
				EmuApp::pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 0 to 100", "",
					[this](auto val)
					{
						if(optionSoundVolume.isValidVal(val))
						{
							setSoundVolume(val, *audio);
							soundVolume.setSelected(std::size(soundVolumeItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							EmuApp::postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		},
	},
	soundVolume
	{
		"Volume",
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<5>("%u%%", optionSoundVolume.val).data());
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
		{"2", [this]() { setSoundBuffers(2); }},
		{"3", [this]() { setSoundBuffers(3); }},
		{"4", [this]() { setSoundBuffers(4); }},
		{"5", [this]() { setSoundBuffers(5); }},
		{"6", [this]() { setSoundBuffers(6); }},
		{"7", [this]() { setSoundBuffers(7); }},
		{"8", [this]() { setSoundBuffers(8); }},
	},
	soundBuffers
	{
		"Buffer Size In Frames",
		(int)optionSoundBuffers - 2,
		[this](const MultiChoiceMenuItem &) -> int
		{
			return std::size(soundBuffersItem);
		},
		[this](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return soundBuffersItem[idx];
		}
	},
	addSoundBuffersOnUnderrun
	{
		"Auto-increase Buffer Size",
		(bool)optionAddSoundBuffersOnUnderrun,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionAddSoundBuffersOnUnderrun = item.flipBoolValue(*this);
			audio->setAddSoundBuffersOnUnderrun(optionAddSoundBuffersOnUnderrun);
		}
	},
	audioRate
	{
		"Sound Rate",
		0,
		audioRateItem
	}
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	,audioSoloMix
	{
		"Mix With Other Apps",
		!optionAudioSoloMix,
		[this](BoolMenuItem &item, Input::Event e)
		{
			optionAudioSoloMix = !item.flipBoolValue(*this);
			IG::AudioManager::setSoloMix(appContext(), optionAudioSoloMix);
		}
	}
	#endif
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	,api
	{
		"Audio Driver",
		0,
		apiItem
	}
	#endif
{
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	apiItem.emplace_back("Auto",
		[this](View &view)
		{
			auto app = appContext();
			optionAudioAPI = 0;
			auto defaultApi = IG::Audio::makeValidAPI(app);
			audio->open(app, defaultApi);
			api.setSelected(idxOfAPI(defaultApi, IG::Audio::audioAPIs(app)));
			view.dismiss();
			return false;
		});
	for(auto desc: IG::Audio::audioAPIs(appContext()))
	{
		apiItem.emplace_back(desc.name,
			[this, api = desc.api]()
			{
				optionAudioAPI = (uint8_t)api;
				audio->open(appContext(), api);
			});
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
		audioRateItem.emplace_back("Device Native",
			[this](View &view)
			{
				setAudioRate(optionSoundRate.defaultVal);
				updateAudioRateItem();
				view.dismiss();
				return false;
			});
		audioRateItem.emplace_back("22KHz", [this]() { setAudioRate(22050); });
		audioRateItem.emplace_back("32KHz", [this]() { setAudioRate(32000); });
		audioRateItem.emplace_back("44KHz", [this]() { setAudioRate(44100); });
		if(optionSoundRate.defaultVal >= 48000)
			audioRateItem.emplace_back("48KHz", [this]() { setAudioRate(48000); });
		item.emplace_back(&audioRate);
		updateAudioRateItem();
	}
	item.emplace_back(&soundBuffers);
	item.emplace_back(&addSoundBuffersOnUnderrun);
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	item.emplace_back(&audioSoloMix);
	#endif
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	auto app = appContext();
	if(auto apiVec = IG::Audio::audioAPIs(app);
		apiVec.size() > 1)
	{
		item.emplace_back(&api);
		api.setSelected(idxOfAPI(IG::Audio::makeValidAPI(app, audioOutputAPI()), std::move(apiVec)));
	}
	#endif
}

void AudioOptionView::setEmuAudio(EmuAudio &audio_)
{
	audio = &audio_;
}

void AudioOptionView::updateAudioRateItem()
{
	switch(optionSoundRate)
	{
		bcase 22050: audioRate.setSelected(1);
		bcase 32000: audioRate.setSelected(2);
		bdefault: audioRate.setSelected(3); // 44100
		bcase 48000: audioRate.setSelected(4);
	}
}

#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
unsigned AudioOptionView::idxOfAPI(IG::Audio::Api api, std::vector<IG::Audio::ApiDesc> apiVec)
{
	for(unsigned idx = 0; auto desc: apiVec)
	{
		if(desc.api == api)
		{
			assert(idx + 1 < std::size(apiItem));
			return idx + 1;
		}
		idx++;
	}
	return 0;
}
#endif
