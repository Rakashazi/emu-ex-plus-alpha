/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "MainSystem.hh"
#include <emuframework/Option.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"MSX.emu"};
const char *EmuSystem::configFilename = "MsxEmu.config";
int EmuSystem::forcedSoundRate = 44100;

std::span<const AspectRatioInfo> MsxSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool MsxSystem::mixerEnableOption(MixerAudioType type)
{
	return optionMixerVolume(type, [&](auto &v){ return v.value().enable; });
}

void MsxSystem::setMixerEnableOption(MixerAudioType type, bool on)
{
	optionMixerVolume(type, [&](auto &v)
	{
		auto flags = v.value();
		flags.enable = on;
		v.setUnchecked(flags);
	});
	mixerEnableChannelType(mixer, type, on);
}

uint8_t MsxSystem::mixerVolumeOption(MixerAudioType type)
{
	return optionMixerVolume(type, [](auto &v){return v.value().volume;});
}

uint8_t MsxSystem::setMixerVolumeOption(MixerAudioType type, int volume)
{
	return optionMixerVolume(type, [&](auto &v)
	{
		if(volume == -1)
		{
			volume = v.defaultValue().volume;
		}
		auto flags = v.value();
		flags.volume = volume;
		v.set(flags);
		mixerSetChannelTypeVolume(mixer, type, volume);
		return volume;
	});
}

uint8_t MsxSystem::mixerPanOption(MixerAudioType type)
{
	return optionMixerPan(type, [](auto &v){ return v.value(); });
}

uint8_t MsxSystem::setMixerPanOption(MixerAudioType type, int pan)
{
	if(pan == -1)
	{
		optionMixerPan(type, [&](auto &v){ pan = v.reset(); });
	}
	else
	{
		optionMixerPan(type, [&](auto &v){ v = pan; });
	}
	mixerSetChannelTypePan(mixer, type, pan);
	return pan;
}

bool MsxSystem::resetSessionOptions(EmuApp &)
{
	optionSessionMachineNameStr.clear();
	return true;
}

bool MsxSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_DEFAULT_MACHINE_NAME: return readStringOptionValue(io, optionDefaultMachineNameStr);
			case CFGKEY_DEFAULT_COLECO_MACHINE_NAME: return readStringOptionValue(io, optionDefaultColecoMachineNameStr);
			case CFGKEY_SKIP_FDC_ACCESS: return readOptionValue(io, optionSkipFdcAccess);
			case CFGKEY_MACHINE_FILE_PATH: return readStringOptionValue<FS::PathString>(io, [&](auto &&path){firmwarePath_ = IG_forward(path);});
			case CFGKEY_MIXER_PSG_VOLUME: return readOptionValue(io, optionMixerPSGVolume);
			case CFGKEY_MIXER_SCC_VOLUME: return readOptionValue(io, optionMixerSCCVolume);
			case CFGKEY_MIXER_MSX_MUSIC_VOLUME: return readOptionValue(io, optionMixerMSXMUSICVolume);
			case CFGKEY_MIXER_MSX_AUDIO_VOLUME: return readOptionValue(io, optionMixerMSXAUDIOVolume);
			case CFGKEY_MIXER_MOON_SOUND_VOLUME: return readOptionValue(io, optionMixerMoonSoundVolume);
			case CFGKEY_MIXER_YAMAHA_SFG_VOLUME: return readOptionValue(io, optionMixerYamahaSFGVolume);
			case CFGKEY_MIXER_PCM_VOLUME: return readOptionValue(io, optionMixerPCMVolume);
			case CFGKEY_MIXER_PSG_PAN: return readOptionValue(io, optionMixerPSGPan);
			case CFGKEY_MIXER_SCC_PAN: return readOptionValue(io, optionMixerSCCPan);
			case CFGKEY_MIXER_MSX_MUSIC_PAN: return readOptionValue(io, optionMixerMSXMUSICPan);
			case CFGKEY_MIXER_MSX_AUDIO_PAN: return readOptionValue(io, optionMixerMSXAUDIOPan);
			case CFGKEY_MIXER_MOON_SOUND_PAN: return readOptionValue(io, optionMixerMoonSoundPan);
			case CFGKEY_MIXER_YAMAHA_SFG_PAN: return readOptionValue(io, optionMixerYamahaSFGPan);
			case CFGKEY_MIXER_PCM_PAN: return readOptionValue(io, optionMixerPCMPan);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_SESSION_MACHINE_NAME: return readStringOptionValue(io, optionSessionMachineNameStr);
		}
	}
	return false;
}

void MsxSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		if(optionDefaultMachineNameStr != optionMachineNameDefault)
		{
			writeStringOptionValue(io, CFGKEY_DEFAULT_MACHINE_NAME, optionDefaultMachineNameStr);
		}
		if(optionDefaultColecoMachineNameStr != optionColecoMachineNameDefault)
		{
			writeStringOptionValue(io, CFGKEY_DEFAULT_COLECO_MACHINE_NAME, optionDefaultColecoMachineNameStr);
		}
		writeOptionValueIfNotDefault(io, optionSkipFdcAccess);
		writeStringOptionValue(io, CFGKEY_MACHINE_FILE_PATH, firmwarePath_);

		writeOptionValueIfNotDefault(io, optionMixerPSGVolume);
		writeOptionValueIfNotDefault(io, optionMixerSCCVolume);
		writeOptionValueIfNotDefault(io, optionMixerMSXMUSICVolume);
		writeOptionValueIfNotDefault(io, optionMixerMSXAUDIOVolume);
		writeOptionValueIfNotDefault(io, optionMixerMoonSoundVolume);
		writeOptionValueIfNotDefault(io, optionMixerYamahaSFGVolume);
		writeOptionValueIfNotDefault(io, optionMixerPCMVolume);

		writeOptionValueIfNotDefault(io, optionMixerPSGPan);
		writeOptionValueIfNotDefault(io, optionMixerSCCPan);
		writeOptionValueIfNotDefault(io, optionMixerMSXMUSICPan);
		writeOptionValueIfNotDefault(io, optionMixerMSXAUDIOPan);
		writeOptionValueIfNotDefault(io, optionMixerMoonSoundPan);
		writeOptionValueIfNotDefault(io, optionMixerYamahaSFGPan);
		writeOptionValueIfNotDefault(io, optionMixerPCMPan);
	}
	else if(type == ConfigType::SESSION)
	{
		writeStringOptionValue(io, CFGKEY_SESSION_MACHINE_NAME, optionSessionMachineNameStr);
	}
}

void MsxSystem::onOptionsLoaded()
{
	mixerEnableChannelType(mixer, MIXER_CHANNEL_PSG, mixerEnableOption(MIXER_CHANNEL_PSG));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PSG, mixerVolumeOption(MIXER_CHANNEL_PSG));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PSG, optionMixerPSGPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_SCC, mixerEnableOption(MIXER_CHANNEL_SCC));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_SCC, mixerVolumeOption(MIXER_CHANNEL_SCC));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_SCC, optionMixerSCCPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXMUSIC, mixerEnableOption(MIXER_CHANNEL_MSXMUSIC));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXMUSIC, mixerVolumeOption(MIXER_CHANNEL_MSXMUSIC));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXMUSIC, optionMixerMSXMUSICPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MSXAUDIO, mixerEnableOption(MIXER_CHANNEL_MSXAUDIO));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MSXAUDIO, mixerVolumeOption(MIXER_CHANNEL_MSXAUDIO));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MSXAUDIO, optionMixerMSXAUDIOPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_MOONSOUND, mixerEnableOption(MIXER_CHANNEL_MOONSOUND));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_MOONSOUND, mixerVolumeOption(MIXER_CHANNEL_MOONSOUND));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_MOONSOUND, optionMixerMoonSoundPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_YAMAHA_SFG, mixerEnableOption(MIXER_CHANNEL_YAMAHA_SFG));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_YAMAHA_SFG, mixerVolumeOption(MIXER_CHANNEL_YAMAHA_SFG));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_YAMAHA_SFG, optionMixerYamahaSFGPan);

	mixerEnableChannelType(mixer, MIXER_CHANNEL_PCM, mixerEnableOption(MIXER_CHANNEL_PCM));
	mixerSetChannelTypeVolume(mixer, MIXER_CHANNEL_PCM, mixerVolumeOption(MIXER_CHANNEL_PCM));
	mixerSetChannelTypePan(mixer, MIXER_CHANNEL_PCM, optionMixerPCMPan);
}

bool MsxSystem::setDefaultMachineName(std::string_view name)
{
	if(name == optionDefaultMachineNameStr)
		return false;
	log.info("set default MSX machine:{}", name);
	optionDefaultMachineNameStr = name;
	return true;
}

bool MsxSystem::setDefaultColecoMachineName(std::string_view name)
{
	if(name == optionDefaultColecoMachineNameStr)
		return false;
	log.info("set default Coleco machine:{}", name);
	optionDefaultColecoMachineNameStr = name;
	return true;
}

static bool archiveHasMachinesDirectory(ApplicationContext ctx, CStringView path)
{
	return bool(FS::findDirectoryInArchive(ctx.openFileUri(path), [&](auto &entry){ return entry.name().ends_with("Machines/"); }));
}

void MsxSystem::setFirmwarePath(CStringView path, FS::file_type type)
{
	auto ctx = appContext();
	log.info("set firmware path:{}", path);
	if((type == FS::file_type::directory && !ctx.fileUriExists(FS::uriString(path, "Machines")))
		|| (FS::hasArchiveExtension(path) && !archiveHasMachinesDirectory(ctx, path)))
	{
		throw std::runtime_error{"Path is missing Machines folder"};
	}
	firmwarePath_ = path;
	firmwareArch = {};
}

FS::PathString MsxSystem::firmwarePath() const
{
	if(firmwarePath_.empty())
	{
		if constexpr(Config::envIsLinux && !Config::MACHINE_IS_PANDORA)
			return appContext().assetPath();
		else
			return appContext().storagePath();
	}
	else
	{
		return firmwarePath_;
	}
}

}
