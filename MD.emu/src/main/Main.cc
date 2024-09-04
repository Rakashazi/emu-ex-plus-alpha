/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuAppInlines.hh>
#include <emuframework/EmuSystemInlines.hh>
#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "input.h"
#include "io_ctrl.h"
#include "sram.h"
#include "state.h"
#include "sound.h"
#include "vdp_ctrl.h"
#include "vdp_render.h"
#include "genesis.h"
#include "genplus-config.h"
#ifndef NO_SCD
#include <scd/scd.h>
#include <mednafen/mednafen.h>
#include <mednafen/cdrom/CDAccess.h>
#include <mednafen-emuex/ArchiveVFS.hh>
#endif
#include "Cheats.hh"
#include <imagine/fs/FS.hh>
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ScopeGuard.hh>

t_config config{};
t_bitmap bitmap{};
bool config_ym2413_enabled = true;

namespace EmuEx
{

constexpr SystemLogger log{"MD.emu"};
const char *EmuSystem::creditsViewStr = CREDITS_INFO_STRING "(c) 2011-2024\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nGenesis Plus Team\nsegaretro.org/Genesis_Plus";
bool EmuSystem::hasCheats = true;
bool EmuSystem::hasPALVideoSystem = true;
bool EmuSystem::canRenderRGBA8888 = RENDER_BPP == 32;
bool EmuSystem::hasRectangularPixels = true;
bool EmuApp::needsGlobalInstance = true;

MdApp::MdApp(ApplicationInitParams initParams, ApplicationContext &ctx):
	EmuApp{initParams, ctx}, mdSystem{ctx} {}

static bool hasBinExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".bin");
}

bool hasMDExtension(std::string_view name)
{
	return hasBinExtension(name) || IG::endsWithAnyCaseless(name, ".smd", ".md", ".gen"
		#ifndef NO_SYSTEM_PBC
		, ".sms"
		#endif
		);
}

static bool hasMDCDExtension(std::string_view name)
{
	return IG::endsWithAnyCaseless(name, ".cue", ".iso", ".chd");
}

static bool hasMDWithCDExtension(std::string_view name)
{
	return hasMDExtension(name)
	#ifndef NO_SCD
		|| hasMDCDExtension(name)
	#endif
	;
}

const char *EmuSystem::shortSystemName() const
{
	return "MD-Genesis";
}

const char *EmuSystem::systemName() const
{
	return "Mega Drive (Sega Genesis)";
}

EmuSystem::NameFilterFunc EmuSystem::defaultFsFilter = hasMDWithCDExtension;

void MdSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	RAMCheatUpdate();
	system_frame(taskCtx, video);

	int16 audioBuff[snd.buffer_size * 2];
	int frames = audio_update(audioBuff);
	if(audio)
	{
		audio->writeFrames(audioBuff, frames);
	}
}

void MdSystem::renderFramebuffer(EmuVideo &video)
{
	video.startFrameWithAltFormat({}, framebufferRenderFormatPixmap());
}

VideoSystem MdSystem::videoSystem() const { return vdp_pal ? VideoSystem::PAL : VideoSystem::NATIVE_NTSC; }

void MdSystem::reset(EmuApp &, ResetMode mode)
{
	assert(hasContent());
	#ifndef NO_SCD
	if(sCD.isActive)
		system_reset();
	else
	#endif
		gen_reset(0);
}

FS::FileString MdSystem::stateFilename(int slot, std::string_view name) const
{
	return IG::format<FS::FileString>("{}.0{}.gp", name, saveSlotChar(slot));
}

static FS::PathString saveFilename(EmuApp &app)
{
	return app.contentSaveFilePath(".srm");
}

static FS::PathString bramSaveFilename(EmuApp &app)
{
	return app.contentSaveFilePath(".brm");
}

void MdSystem::readState(EmuApp &app, std::span<uint8_t> buff)
{
	state_load(buff.data());
}

size_t MdSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	assert(buff.size() == maxSaveStateSize);
	return state_save(buff.data(), flags.uncompressed);
}

static bool sramHasContent(std::span<uint8> sram)
{
	for(auto v : sram)
	{
		if(v != 0xFF)
			return true;
	}
	return false;
}

void MdSystem::loadBackupMemory(EmuApp &app)
{
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		auto saveStr = bramSaveFilename(app);
		auto bramFile = appContext().openFileUri(saveStr, {.test = true, .accessHint = IOAccessHint::All});
		if(!bramFile)
		{
			log.info("no BRAM on disk, formatting");
			IG::fill(bram);
			memcpy(bram + sizeof(bram) - sizeof(fmtBram), fmtBram, sizeof(fmtBram));
			auto sramFormatStart = sram.sram + 0x10000 - sizeof(fmt64kSram);
			memcpy(sramFormatStart, fmt64kSram, sizeof(fmt64kSram));
			for(unsigned i = 0; i < 0x40; i += 2) // byte-swap sram cart format region
			{
				std::swap(sramFormatStart[i], sramFormatStart[i+1]);
			}
		}
		else
		{
			bramFile.read(bram, sizeof(bram));
			bramFile.read(sram.sram, 0x10000);
			for(unsigned i = 0; i < 0x10000; i += 2) // byte-swap
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
			}
			log.info("loaded BRAM from disk");
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = saveFilename(app);

		if(FileUtils::readFromUri(appContext(), saveStr, {sram.sram, 0x10000}) <= 0)
			log.info("no SRAM on disk");
		else
			log.info("loaded SRAM from disk{}", optionBigEndianSram ? ", will byte-swap" : "");

		if(optionBigEndianSram)
		{
			for(unsigned i = 0; i < 0x10000; i += 2)
			{
				std::swap(sram.sram[i], sram.sram[i+1]);
			}
		}
	}
}

void MdSystem::onFlushBackupMemory(EmuApp &app, BackupMemoryDirtyFlags)
{
	if(!hasContent())
		return;
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		log.info("saving BRAM");
		auto saveStr = bramSaveFilename(app);
		auto bramFile = appContext().openFileUri(saveStr, OpenFlags::testNewFile());
		if(!bramFile)
			log.error("error creating bram file");
		else
		{
			bramFile.write(bram, sizeof(bram));
			char sramTemp[0x10000];
			memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
			for(unsigned i = 0; i < 0x10000; i += 2)
			{
				std::swap(sramTemp[i], sramTemp[i+1]);
			}
			bramFile.write(sramTemp, 0x10000);
		}
	}
	else
	#endif
	if(sram.on)
	{
		auto saveStr = saveFilename(app);
		if(sramHasContent({sram.sram, 0x10000}))
		{
			log.info("saving SRAM{}", optionBigEndianSram ? ", byte-swapped" : "");
			uint8_t sramTemp[0x10000];
			uint8_t *sramPtr = sram.sram;
			if(optionBigEndianSram)
			{
				memcpy(sramTemp, sram.sram, 0x10000); // make a temp copy to byte-swap
				for(unsigned i = 0; i < 0x10000; i += 2)
				{
					std::swap(sramTemp[i], sramTemp[i+1]);
				}
				sramPtr = sramTemp;
			}
			try
			{
				FileUtils::writeToUri(appContext(), saveStr, {sramPtr, 0x10000});
			}
			catch(...)
			{
				log.error("error creating sram file");
			}
		}
		else
		{
			log.info("SRAM wasn't written to");
			appContext().removeFileUri(saveStr);
		}
	}
}

WallClockTimePoint MdSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(
		app.contentSaveFilePath(sCD.isActive ? ".brm" : ".srm").c_str());
}

void MdSystem::closeSystem()
{
	#ifndef NO_SCD
	if(sCD.isActive)
	{
		scd_deinit();
	}
	#endif
	old_system[0] = old_system[1] = -1;
	input.system[0] = input.system[1] = NO_SYSTEM;
	clearCheatList();
}

static unsigned detectISORegion(uint8 bootSector[0x800])
{
	auto bootByte = bootSector[0x20b];

	if(bootByte == 0x7a)
		return REGION_USA;
	else if(bootByte == 0x64)
		return REGION_EUROPE;
	else
		return REGION_JAPAN_NTSC;
}

FS::PathString EmuSystem::willLoadContentFromPath(std::string_view path, std::string_view displayName)
{
	#ifndef NO_SCD
	// check if loading a .bin with matching .cue
	if(hasBinExtension(path))
	{
		FS::PathString possibleCuePath{path};
		possibleCuePath.replace(possibleCuePath.end() - 3, possibleCuePath.end(), "cue");
		return possibleCuePath;
	}
	#endif
	return {};
}

void MdSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	#ifndef NO_SCD
	using namespace Mednafen;
	CDAccess *cd{};
	auto deleteCDAccess = IG::scopeGuard([&](){ delete cd; });
	if(hasMDCDExtension(contentFileName()) ||
		(hasBinExtension(contentFileName()) && io.size() > 1024*1024*10)) // CD
	{
		bool isArchive = std::holds_alternative<ArchiveIO>(io);
		if(contentDirectory().empty() && !isArchive)
		{
			throwMissingContentDirError();
		}
		if(isArchive)
		{
			if(endsWithAnyCaseless(contentFileName(), ".bin", ".iso"))
			{
				log.info("looking for a .cue file in archive");
				// check the archive for a .cue and load that instead
				FS::ArchiveIterator archIt{std::move(io)};
				for(auto &entry : archIt)
				{
					if(entry.type() == FS::file_type::directory)
					{
						continue;
					}
					auto name = entry.name();
					if(endsWithAnyCaseless(name, ".cue"))
					{
						log.info("found:{}", name);
						contentFileName_ = name;
						break;
					}
				}
				io = std::move(*archIt);
			}
			ArchiveVFS archVFS{ArchiveIO{std::move(io)}};
			cd = CDAccess_Open(&archVFS, std::string{contentFileName()}, true);
		}
		else
		{
			cd = CDAccess_Open(&NVFS, std::string{contentLocation()}, false);
		}

		unsigned region = REGION_USA;
		if (config.region_detect == 1) region = REGION_USA;
	  else if (config.region_detect == 2) region = REGION_EUROPE;
	  else if (config.region_detect == 3) region = REGION_JAPAN_NTSC;
	  else if (config.region_detect == 4) region = REGION_JAPAN_PAL;
	  else
	  {
	  	uint8 bootSector[2048];
	  	cd->Read_Sector(bootSector, 0, 2048);
			region = detectISORegion(bootSector);
	  }

		struct BiosDesc{std::string_view path, name;};
		auto [biosPath, biosName] = [&] -> BiosDesc
		{
			switch(region)
			{
				case REGION_USA: return {cdBiosUSAPath, "USA"};
				case REGION_EUROPE: return {cdBiosEurPath, "Europe"};
				default: return {cdBiosJpnPath, "Japan"};
			}
		}();
		if(biosPath.empty())
		{
			throw std::runtime_error(std::format("Set a {} BIOS in the Options", biosName));
		}
		auto [biosSize, biosFilename] = FileUtils::readFromUriWithArchiveScan(appContext(), biosPath, {cart.rom, MAXROMSIZE}, hasMDExtension);
		if(biosSize <= 0)
			throw std::runtime_error(std::format("Error loading BIOS: {}", biosPath));
		init_rom(biosSize, "");
		if(!sCD.isActive)
		{
			throw std::runtime_error(std::format("Invalid BIOS: {}", biosPath));
		}
	}
	else
	#endif
	// ROM
	{
		log.info("loading ROM:{}", contentLocation());
		auto size = io.read(cart.rom, MAXROMSIZE);
		if(size <= 0)
			throwFileReadError();
		init_rom(size, contentFileName());
	}
	autoDetectedVidSysPAL = vdp_pal;
	if((int)optionVideoSystem == 1)
	{
		vdp_pal = 0;
	}
	else if((int)optionVideoSystem == 2)
	{
		vdp_pal = 1;
	}
	if(videoSystem() == VideoSystem::PAL)
		log.info("using PAL timing");

	system_init();
	for(auto i : iotaCount(2))
	{
		if(old_system[i] != -1)
			old_system[i] = input.system[i]; // store input ports set by game
	}
	setupInput(EmuApp::get(appContext()));

	system_reset();

	#ifndef NO_SCD
	if(sCD.isActive)
	{
		if(Insert_CD(cd) != 0)
		{
			throw std::runtime_error("Error loading CD");
		}
		deleteCDAccess.cancel();
	}
	#endif

	readCheatFile();
	applyCheats();
}

void MdSystem::configAudioRate(FrameTime outputFrameTime, int outputRate)
{
	float outputFrameRate = toHz(outputFrameTime);
	if(snd.sample_rate == outputRate && snd.frame_rate == outputFrameRate)
		return;
	log.info("set sound output rate:{} for fps:{}", outputRate, outputFrameRate);
	audio_init(outputRate, outputFrameRate);
	sound_restore();
	//log.debug("set sound buffer size:{}", snd.buffer_size);
}

bool MdSystem::onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat fmt)
{
	setFramebufferRenderFormat(fmt);
	return false;
}

void EmuApp::onCustomizeNavView(EmuApp::NavView &view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build(0., 0., 1. * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build(0., 0., 1. * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build(0., 0., .6 * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}
