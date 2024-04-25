#pragma once

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

#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

using namespace IG;
class EmuAudio;

class AudioOptionView : public TableView, public EmuAppHelper
{
public:
	AudioOptionView(ViewAttachParams attach, EmuAudio&, bool customMenu = false);
	void loadStockItems();

protected:
	EmuAudio &audio;
	BoolMenuItem snd;
	BoolMenuItem soundDuringFastSlowMode;
	TextMenuItem soundVolumeItem[4];
	MultiChoiceMenuItem soundVolume;
	TextMenuItem soundBuffersItem[7];
	MultiChoiceMenuItem soundBuffers;
	BoolMenuItem addSoundBuffersOnUnderrun;
	StaticArrayList<TextMenuItem, 5> audioRateItem;
	MultiChoiceMenuItem audioRate;
	ConditionalMember<IG::Audio::Manager::HAS_SOLO_MIX, BoolMenuItem> audioSoloMix;
	using ApiItemContainer = StaticArrayList<TextMenuItem, Audio::systemApis.size() + 1>;
	ConditionalMember<IG::Audio::Config::MULTIPLE_SYSTEM_APIS, ApiItemContainer> apiItem;
	ConditionalMember<IG::Audio::Config::MULTIPLE_SYSTEM_APIS, MultiChoiceMenuItem> api;
	StaticArrayList<MenuItem*, 22> item;
};

}
