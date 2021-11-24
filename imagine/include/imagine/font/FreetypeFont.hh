#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/font/FontSettings.hh>
#include <imagine/font/GlyphMetrics.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/container/ArrayList.hh>
#include <system_error>
#include <array>
#include <memory>
#include <ft2build.h>
#include FT_FREETYPE_H

class GenericIO;
struct _FcConfig;

namespace Base
{
class ApplicationContext;
}

namespace IG
{

class FreetypeGlyphImage
{
public:
	constexpr FreetypeGlyphImage() {}
	constexpr FreetypeGlyphImage(FT_Library library, FT_Bitmap bitmap):
		library{library}, bitmap{bitmap} {}
	FreetypeGlyphImage(FreetypeGlyphImage &&o);
	FreetypeGlyphImage &operator=(FreetypeGlyphImage &&o);
	~FreetypeGlyphImage();

protected:
	FT_Library library{};
	FT_Bitmap bitmap{};

	void deinit();
};

static constexpr uint32_t MAX_FREETYPE_SLOTS = Config::envIsLinux ? 4 : 2;

class FreetypeFontSize
{
public:
	using FTSizeArray = std::array<FT_Size, MAX_FREETYPE_SLOTS>;

	constexpr FreetypeFontSize() {}
	FreetypeFontSize(FontSettings settings);
	FreetypeFontSize(FreetypeFontSize &&o);
	FreetypeFontSize &operator=(FreetypeFontSize &&o);
	~FreetypeFontSize();

	FTSizeArray &sizeArray();
	FontSettings fontSettings() const;

protected:
	FontSettings settings{};
	FTSizeArray ftSize{};

	void deinit();
};

struct FreetypeFaceData
{
	FT_Face face{};
	std::unique_ptr<FT_StreamRec> streamRecPtr{};

	constexpr FreetypeFaceData() {}
	FreetypeFaceData(FT_Library, GenericIO file);
};

class FreetypeFont
{
public:
	struct GlyphRenderData
	{
		GlyphMetrics metrics{};
		FT_Bitmap bitmap{};
	};

	constexpr FreetypeFont() {}
	constexpr FreetypeFont(FT_Library library, bool isBold = false):
		library{library}, isBold{isBold} {}
	FreetypeFont(FT_Library, GenericIO);
	FreetypeFont(FT_Library, const char *path);
	FreetypeFont(FreetypeFont &&o);
	FreetypeFont &operator=(FreetypeFont &&o);
	~FreetypeFont();

protected:
	FT_Library library{};
	StaticArrayList<FreetypeFaceData, MAX_FREETYPE_SLOTS> f{};
	bool isBold{};

	std::errc loadIntoNextSlot(GenericIO io);
	std::errc loadIntoNextSlot(IG::CStringView name);
	GlyphRenderData makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData, std::errc &ec);
	void deinit();
};

class FreetypeFontManager
{
public:
	FreetypeFontManager(Base::ApplicationContext);

protected:
	struct FtLibraryDeleter
	{
		void operator()(FT_Library ptr) const
		{
			freeFtLibrary(ptr);
		}
	};
	using UniqueFTLibrary = std::unique_ptr<std::remove_pointer_t<FT_Library>, FtLibraryDeleter>;

	struct FcConfigDeleter
	{
		void operator()(_FcConfig *ptr) const
		{
			freeFcConfig(ptr);
		}
	};
	using UniqueFcConfig = std::unique_ptr<_FcConfig, FcConfigDeleter>;

	Base::ApplicationContext ctx{};
	UniqueFTLibrary library{};
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	UniqueFcConfig fcConf{};
	#endif

	static void freeFtLibrary(FT_Library);
	static void freeFcConfig(_FcConfig*);
};

using GlyphImageImpl = FreetypeGlyphImage;
using FontImpl = FreetypeFont;
using FontSize = FreetypeFontSize;
using FontManagerImpl = FreetypeFontManager;

}
