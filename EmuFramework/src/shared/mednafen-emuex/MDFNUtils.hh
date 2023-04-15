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
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <emuframework/EmuApp.hh>
#include <mednafen/video/surface.h>
#include <mednafen/hash/md5.h>
#include <mednafen/git.h>
#include <mednafen/MemoryStream.h>
#include <string_view>

namespace EmuEx
{

inline Mednafen::MDFN_Surface toMDFNSurface(IG::MutablePixmapView pix)
{
	using namespace Mednafen;
	MDFN_PixelFormat fmt =
		[&]()
		{
			switch(pix.format().id())
			{
				case IG::PIXEL_BGRA8888: return MDFN_PixelFormat::ARGB32_8888;
				case IG::PIXEL_RGBA8888: return MDFN_PixelFormat::ABGR32_8888;
				case IG::PIXEL_RGB565: return MDFN_PixelFormat::RGB16_565;
				default:
					bug_unreachable("format id == %d", pix.format().id());
			};
		}();
	return {pix.data(), uint32(pix.w()), uint32(pix.h()), uint32(pix.pitchPx()), fmt};
}

inline FS::FileString stateFilenameMDFN(const Mednafen::MDFNGI &gameInfo, int slot, std::string_view name, char autoChar)
{
	auto saveSlotChar = [&](int slot) -> char
	{
		switch(slot)
		{
			case -1: return autoChar;
			case 0 ... 9: return '0' + slot;
			default: bug_unreachable("slot == %d", slot);
		}
	};
	return IG::format<FS::FileString>("{}.{}.nc{}",
		name, Mednafen::md5_context::asciistr(gameInfo.MD5, 0), saveSlotChar(slot));
}

inline std::string savePathMDFN(const EmuApp &app, int id1, const char *cd1)
{
	assert(cd1);
	IG::FileString ext{'.'};
	ext += Mednafen::md5_context::asciistr(Mednafen::MDFNGameInfo->MD5, 0);
	ext += '.';
	ext += cd1;
	auto path = app.contentSaveFilePath(ext);
	return std::string{path};
}

inline std::string savePathMDFN(int id1, const char *cd1)
{
	return savePathMDFN(EmuEx::gApp(), id1, cd1);
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
	GameFile gf{&NVFS, std::string{sys.contentDirectory()}, fp.stream(),
		std::string{withoutDotExtension(sys.contentFileName())},
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

}
