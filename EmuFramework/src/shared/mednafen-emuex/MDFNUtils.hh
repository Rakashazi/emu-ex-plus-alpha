/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/pixmap/Pixmap.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/io/IO.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/util/zlib.hh>
#include <emuframework/EmuApp.hh>
#include <mednafen/types.h>
#include <mednafen/video/surface.h>
#include <mednafen/hash/md5.h>
#include <mednafen/git.h>
#include <mednafen/FileStream.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/cdrom/CDInterface.h>
#include <main/MainSystem.hh>
#include <string_view>

namespace Mednafen
{

struct DriveMediaStatus
{
	uint32 state_idx{};
	uint32 media_idx{};
	uint32 orientation_idx{};
};

}

namespace EmuEx
{

inline Mednafen::MDFN_Surface toMDFNSurface(IG::MutablePixmapView pix)
{
	using namespace Mednafen;
	MDFN_PixelFormat fmt =
		[&]()
		{
			switch(pix.format().id)
			{
				case IG::PixelFmtBGRA8888: return MDFN_PixelFormat::ARGB32_8888;
				case IG::PixelFmtRGBA8888: return MDFN_PixelFormat::ABGR32_8888;
				case IG::PixelFmtRGB565: return MDFN_PixelFormat::RGB16_565;
				default: std::unreachable();
			};
		}();
	return {pix.data(), uint32(pix.w()), uint32(pix.h()), uint32(pix.pitchPx()), fmt};
}

inline FS::FileString stateFilenameMDFN(const Mednafen::MDFNGI &gameInfo, int slot, std::string_view name, char autoChar, bool skipMD5)
{
	auto saveSlotChar = [&] -> char
	{
		switch(slot)
		{
			case -1: return autoChar;
			case 0 ... 9: return '0' + slot;
			default: bug_unreachable("slot == %d", slot);
		}
	}();
	if(skipMD5)
		return format<FS::FileString>("{}.nc{}", name, saveSlotChar);
	else
		return format<FS::FileString>("{}.{}.nc{}", name, Mednafen::md5_context::asciistr(gameInfo.MD5, 0), saveSlotChar);
}

inline FS::FileString saveExtMDFN(std::string_view ext, bool skipMD5)
{
	IG::FileString str{'.'};
	if(!skipMD5)
	{
		str += Mednafen::md5_context::asciistr(Mednafen::MDFNGameInfo->MD5, 0);
		str += '.';
	}
	str += ext;
	return str;
}

inline std::string savePathMDFN(const EmuApp &app, [[maybe_unused]] int id1, const char *cd1, bool skipMD5)
{
	assert(cd1);
	IG::FileString ext{'.'};
	if(!skipMD5)
	{
		ext += Mednafen::md5_context::asciistr(Mednafen::MDFNGameInfo->MD5, 0);
		ext += '.';
	}
	ext += cd1;
	auto path = app.contentSaveFilePath(ext);
	return std::string{path};
}

inline std::string savePathMDFN(int id1, const char *cd1)
{
	auto &app = EmuEx::gApp();
	return savePathMDFN(app, id1, cd1, static_cast<MainSystem&>(app.system()).noMD5InFilenames);
}

inline BoolMenuItem saveFilenameTypeMenuItem(auto &view, auto &system)
{
	return {"Save Filename Type", view.attachParams(),
		system.noMD5InFilenames,
		"Default", "No MD5",
		[&](BoolMenuItem &item) { system.noMD5InFilenames = item.flipBoolValue(view); }
	};
}

inline void loadContent(EmuSystem &sys, Mednafen::MDFNGI &mdfnGameInfo, IO &io, size_t maxContentSize)
{
	using namespace Mednafen;
	auto stream = std::make_unique<MemoryStream>(maxContentSize, true);
	auto size = io.read(stream->map(), stream->map_size());
	if(size <= 0)
		sys.throwFileReadError();
	stream->setSize(size);
	MDFNFILE fp(&NVFS, std::move(stream));
	GameFile gf{&NVFS, std::string{sys.contentDirectory()}, {}, fp.stream(),
		std::string{dotExtension(sys.contentFileName())},
		std::string{sys.contentName()}};
	mdfnGameInfo.Load(&gf);
}

inline void runFrame(EmuSystem &sys, Mednafen::MDFNGI &mdfnGameInfo, EmuSystemTaskContext taskCtx,
	EmuVideo *videoPtr, MutablePixmapView pixView, EmuAudio *audioPtr, size_t maxAudioFrames, size_t maxLineWidths = 0)
{
	using namespace Mednafen;
	int16 audioBuff[maxAudioFrames * 2];
	EmulateSpecStruct espec{};
	if(audioPtr)
	{
		espec.SoundBuf = audioBuff;
		espec.SoundBufMaxSize = maxAudioFrames;
	}
	espec.taskCtx = taskCtx;
	espec.sys = &sys;
	espec.video = videoPtr;
	espec.skip = !videoPtr;
	auto mSurface = toMDFNSurface(pixView);
	espec.surface = &mSurface;
	int32 lineWidth[maxLineWidths ?: 1];
	if(maxLineWidths)
		espec.LineWidths = lineWidth;
	mdfnGameInfo.Emulate(&espec);
	if(audioPtr)
	{
		assert((unsigned)espec.SoundBufSize <= audioPtr->format().bytesToFrames(sizeof(audioBuff)));
		audioPtr->writeFrames((uint8_t*)audioBuff, espec.SoundBufSize);
	}
}

// Save states

inline size_t stateSizeMDFN()
{
	using namespace Mednafen;
	MemoryStream s;
	MDFNSS_SaveSM(&s);
	return s.size();
}

inline void readStateMDFN(std::span<uint8_t> buff)
{
	using namespace Mednafen;
	if(hasGzipHeader(buff))
	{
		MemoryStream s{gzipUncompressedSize(buff), -1};
		auto outputSize = uncompressGzip({s.map(), size_t(s.size())}, buff);
		if(!outputSize)
			throw std::runtime_error("Error uncompressing state");
		if(outputSize <= 32)
			throw std::runtime_error("Invalid state size");
		auto sizeFromHeader = MDFN_de32lsb(s.map() + 16 + 4) & 0x7FFFFFFF;
		if(sizeFromHeader != outputSize)
			throw std::runtime_error(std::format("Bad state header size, got {} but expected {}", sizeFromHeader, outputSize));
		s.setSize(outputSize);
		MDFNSS_LoadSM(&s);
	}
	else
	{
		FileStream s{buff};
		MDFNSS_LoadSM(&s);
	}
}

inline size_t writeStateMDFN(std::span<uint8_t> buff, SaveStateFlags flags)
{
	using namespace Mednafen;
	if(flags.uncompressed)
	{
		FileStream s{buff};
		MDFNSS_SaveSM(&s);
		return s.size();
	}
	else
	{
		MemoryStream s;
		MDFNSS_SaveSM(&s);
		return compressGzip(buff, {s.map(), size_t(s.size())}, MDFN_GetSettingI("filesys.state_comp_level"));
	}
}

inline void writeCDMD5(Mednafen::MDFNGI &mdfnGameInfo, const auto &cdInterfaces)
{
	Mednafen::md5_context layout_md5;
	layout_md5.starts();
	for(auto &cdInterface : cdInterfaces)
	{
		Mednafen::CDUtility::TOC toc;
		cdInterface->ReadTOC(&toc);
		layout_md5.update_u32_as_lsb(toc.first_track);
		layout_md5.update_u32_as_lsb(toc.last_track);
		layout_md5.update_u32_as_lsb(toc.tracks[100].lba);
		for(size_t track = toc.first_track; track <= toc.last_track; track++)
		{
			layout_md5.update_u32_as_lsb(toc.tracks[track].lba);
			layout_md5.update_u32_as_lsb(toc.tracks[track].control & 0x4);
		}
	}
	layout_md5.finish(mdfnGameInfo.MD5);
}

inline void clearCDInterfaces(std::vector<Mednafen::CDInterface *> &ifaces)
{
	for(auto cdIfPtr : ifaces)
	{
		delete cdIfPtr;
	}
	ifaces.clear();
}

}
