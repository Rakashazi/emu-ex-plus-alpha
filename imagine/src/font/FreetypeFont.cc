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

#define LOGTAG "FreetypeFont"
#include <imagine/font/Font.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/algorithm.h>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#ifdef CONFIG_PACKAGE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_SIZES_H

namespace IG
{

#ifdef CONFIG_PACKAGE_FONTCONFIG
static FS::PathString fontPathWithPattern(FcPattern *pat)
{
	if(!FcConfigSubstitute(nullptr, pat, FcMatchPattern))
	{
		logErr("error applying font substitutions");
		return {};
	}
	FcDefaultSubstitute(pat);
	FcResult result = FcResultMatch;
	auto matchPat = FcFontMatch(nullptr, pat, &result);
	auto destroyPattern = IG::scopeGuard(
		[&]()
		{
			if(matchPat)
			{
				FcPatternDestroy(matchPat);
			}
		});
	if(!matchPat || result == FcResultNoMatch)
	{
		logErr("fontconfig couldn't find a valid font");
		return {};
	}
	FcChar8 *patternStr;
	if(FcPatternGetString(matchPat, FC_FILE, 0, &patternStr) != FcResultMatch)
	{
		logErr("fontconfig font missing file path");
		return {};
	}
	return (char*)patternStr;
}

static FS::PathString fontPathContainingChar(int c, int weight)
{
	logMsg("looking for font with char: %c", c);
	auto pat = FcPatternCreate();
	if(!pat)
	{
		logErr("error allocating fontconfig pattern");
		return {};
	}
	auto destroyPattern = IG::scopeGuard([&](){ FcPatternDestroy(pat); });
	auto charSet = FcCharSetCreate();
	if(!charSet)
	{
		logErr("error allocating fontconfig char set");
		return {};
	}
	auto destroyCharSet = IG::scopeGuard([&](){ FcCharSetDestroy(charSet); });
	FcCharSetAddChar(charSet, c);
	FcPatternAddCharSet(pat, FC_CHARSET, charSet);
	FcPatternAddInteger(pat, FC_WEIGHT, weight);
	return fontPathWithPattern(pat);
}
#endif

FreetypeFontManager::FreetypeFontManager(ApplicationContext ctx):
	ctx{ctx}
{
	FT_Library ftLib;
	auto error = FT_Init_FreeType(&ftLib);
	if(error)
	{
		logErr("error in FT_Init_FreeType");
		return;
	}
	library.reset(ftLib);
	/*FT_Int major, minor, patch;
	FT_Library_Version(library, &major, &minor, &patch);
	logMsg("init freetype version %d.%d.%d", (int)major, (int)minor, (int)patch);*/
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	auto fontConfig = FcInitLoadConfigAndFonts();
	if(!fontConfig)
	{
		logErr("error in FcInitLoadConfigAndFonts");
		return;
	}
	fcConf.reset(fontConfig);
	#endif
}

void FreetypeFontManager::freeFtLibrary(FT_Library lib)
{
	FT_Done_FreeType(lib);
}

void FreetypeFontManager::freeFcConfig(_FcConfig *confPtr)
{
	FcConfigDestroy(confPtr);
	FcFini();
}

static FT_Size makeFTSize(FT_Face face, int x, int y)
{
	logMsg("creating new size object, %dx%d pixels", x, y);
	FT_Size size{};
	auto error = FT_New_Size(face, &size);
	if(error)
	{
		logErr("error creating new size object");
		return {};
	}
	error = FT_Activate_Size(size);
	if(error)
	{
		logErr("error activating size object");
		return {};
	}
	error = FT_Set_Pixel_Sizes(face, x, y);
	if(error)
	{
		logErr("error occurred setting character pixel size");
		return {};
	}
	//logMsg("Face max bounds %dx%d,%dx%d, units per EM %d", face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax, face->units_per_EM);
	//logMsg("scaled ascender x descender %dx%d", (int)size->metrics.ascender >> 6, (int)size->metrics.descender >> 6);
	return size;
}

static FreetypeFont::GlyphRenderData makeGlyphRenderDataWithFace(FT_Library library, FT_Face face, int c, bool keepPixData)
{
	auto idx = FT_Get_Char_Index(face, c);
	if(!idx)
	{
		return {};
	}
	auto error = FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
	if(error)
	{
		logErr("error occurred loading/rendering character 0x%X", c);
		return {};
	}
	auto &glyph = face->glyph;
	FT_Bitmap bitmap{};
	if(glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
	{
		// rendered glyph is not in 8-bit gray-scale
		logMsg("converting mode %d bitmap", glyph->bitmap.pixel_mode);
		auto error = FT_Bitmap_Convert(library, &glyph->bitmap, &bitmap, 1);
		if(error)
		{
			logErr("error occurred converting character 0x%X", c);
			return {};
		}
		assert(bitmap.num_grays == 2); // only handle 2 gray levels for now
		//logMsg("new bitmap has %d gray levels", convBitmap.num_grays);
		// scale 1-bit values to 8-bit range
		for(auto y : iotaCount(bitmap.rows))
		{
			for(auto x : iotaCount(bitmap.width))
			{
				if(bitmap.buffer[(y * bitmap.pitch) + x] != 0)
					bitmap.buffer[(y * bitmap.pitch) + x] = 0xFF;
			}
		}
		FT_Bitmap_Done(library, &glyph->bitmap);
	}
	else
	{
		bitmap = glyph->bitmap;
		glyph->bitmap = {};
	}
	GlyphMetrics metrics;
	metrics.size = {int16_t(bitmap.width), int16_t(bitmap.rows)};
	metrics.offset = {int16_t(glyph->bitmap_left), int16_t(glyph->bitmap_top)};
	metrics.xAdvance = glyph->advance.x >> 6;;
	if(!keepPixData)
	{
		FT_Bitmap_Done(library, &bitmap);
	}
	return {metrics, bitmap};
}

FreetypeFaceData::FreetypeFaceData(FT_Library library, IO file):
	streamRecPtr{std::make_unique<FT_StreamRec>()}
{
	if(!file)
		return;
	streamRecPtr->size = file.size();
	streamRecPtr->pos = file.tell();
	streamRecPtr->descriptor.pointer = std::make_unique<IO>(std::move(file)).release();
	streamRecPtr->read = [](FT_Stream stream, unsigned long offset,
		unsigned char* buffer, unsigned long count) -> unsigned long
		{
			auto &io = *((IO*)stream->descriptor.pointer);
			stream->pos = offset;
			if(!count) // no bytes to read
				return 0;
			auto bytesRead = io.read(buffer, count, offset);
			if(bytesRead == -1)
			{
				logErr("error reading bytes in IO func");
				return 0;
			}
			return bytesRead;
		};
	FT_Open_Args openS{};
	openS.flags = FT_OPEN_STREAM;
	openS.stream = streamRecPtr.get();
	auto error = FT_Open_Face(library, &openS, 0, &face);
	if(error == FT_Err_Unknown_File_Format)
	{
		logErr("unknown font format");
		return;
	}
	else if(error)
	{
		logErr("error occurred opening the font");
		return;
	}
}

FreetypeFont::FreetypeFont(FT_Library library, IO io):
	library{library}
{
	loadIntoNextSlot(std::move(io));
}

FreetypeFont::FreetypeFont(FT_Library library, const char *name):
	library{library}
{
	loadIntoNextSlot(FileIO{name, {.accessHint = IOAccessHint::Random}});
}

Font FontManager::makeFromFile(IO io) const
{
	return {library.get(), std::move(io)};
}

Font FontManager::makeFromFile(const char *name) const
{
	return {library.get(), name};
}

Font FontManager::makeSystem() const
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	logMsg("locating system fonts with fontconfig");
	// Let fontconfig handle loading specific fonts on-demand
	return {library.get()};
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font FontManager::makeBoldSystem() const
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	return {library.get(), FontWeight::BOLD};
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font FontManager::makeFromAsset(const char *name, const char *appName) const
{
	return {library.get(), ctx.openAsset(name, {.accessHint = IOAccessHint::Random}, appName)};
}

FreetypeFaceData::FreetypeFaceData(FreetypeFaceData &&o) noexcept
{
	*this = std::move(o);
}

FreetypeFaceData &FreetypeFaceData::operator=(FreetypeFaceData &&o) noexcept
{
	deinit();
	face = std::exchange(o.face, {});
	streamRecPtr = std::move(o.streamRecPtr);
	return *this;
}

FreetypeFaceData::~FreetypeFaceData()
{
	deinit();
}

Font::operator bool() const
{
	return f.size();
}

void FreetypeFaceData::deinit()
{
	if(face)
	{
		FT_Done_Face(face);
	}
	if(streamRecPtr && streamRecPtr->descriptor.pointer)
	{
		delete (IO*)streamRecPtr->descriptor.pointer;
	}
}

bool FreetypeFont::loadIntoNextSlot(IO io)
{
	if(f.isFull())
		return false;
	auto &data = f.emplace_back(library, std::move(io));
	if(!data.face)
	{
		logErr("error reading font");
		f.pop_back();
		return false;
	}
	return true;
}

bool FreetypeFont::loadIntoNextSlot(CStringView name)
{
	if(f.isFull())
		return false;
	try
	{
		return loadIntoNextSlot(FileIO{name, {.accessHint = IOAccessHint::Random}});
	}
	catch(...)
	{
		logMsg("unable to open file %s", name.data());
		return false;
	}
}

FreetypeFont::GlyphRenderData FreetypeFont::makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData)
{
	for(auto i : iotaCount(f.size()))
	{
		auto &font = f[i];
		if(!font.face)
			continue;
		auto ftError = FT_Activate_Size(fontSize.sizeArray()[i]);
		if(ftError)
		{
			logErr("error activating size object");
			return {};
		}
		auto data = makeGlyphRenderDataWithFace(library, font.face, idx, keepPixData);
		if(!data)
		{
			logMsg("glyph 0x%X not found in slot %zu", idx, i);
			continue;
		}
		return data;
	}
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	// try to find a font with the missing char and load into next free slot
	if(f.isFull())
	{
		logErr("no slots left");
		return {};
	}
	auto fontPath = fontPathContainingChar(idx, weight == FontWeight::BOLD ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM);
	if(fontPath.empty())
	{
		logErr("no font file found for char %c (0x%X)", idx, idx);
		return {};
	}
	auto newSlot = f.size();
	if(!loadIntoNextSlot(fontPath))
		return {};
	auto &font = f[newSlot];
	auto settings = fontSize.fontSettings();
	if(!(fontSize.sizeArray()[newSlot] = makeFTSize(font.face, settings.pixelWidth(), settings.pixelHeight())))
	{
		logErr("couldn't allocate font size");
		return {};
	}
	auto data = makeGlyphRenderDataWithFace(library, font.face, idx, keepPixData);
	if(!data)
	{
		logMsg("glyph 0x%X still not found", idx);
		return {};
	}
	return data;
	#else
	return {};
	#endif
}

Font::Glyph Font::glyph(int idx, FontSize &size)
{
	auto data = makeGlyphRenderData(idx, size, true);
	if(!data)
		return {};
	return {{library, data.bitmap}, data.metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size)
{
	auto data = makeGlyphRenderData(idx, size, false);
	if(!data)
		return {};
	return data.metrics;
}

FontSize Font::makeSize(FontSettings settings)
{
	FontSize size{settings};
	// create FT_Size objects for slots in use
	for(auto i : iotaCount(f.size()))
	{
		if(!f[i].face)
		{
			continue;
		}
		if(!(size.sizeArray()[i] = makeFTSize(f[i].face, settings.pixelWidth(), settings.pixelHeight())))
		{
			return {};
		}
	}
	return size;
}

FreetypeFontSize::FreetypeFontSize(FontSettings settings): settings{settings} {}

FreetypeFontSize::~FreetypeFontSize()
{
	deinit();
}

FreetypeFontSize::FreetypeFontSize(FreetypeFontSize &&o) noexcept
{
	*this = std::move(o);
}

FreetypeFontSize &FreetypeFontSize::operator=(FreetypeFontSize &&o) noexcept
{
	deinit();
	settings = o.settings;
	ftSize = std::exchange(o.ftSize, {});
	return *this;
}

FreetypeFontSize::FTSizeArray &FreetypeFontSize::sizeArray()
{
	return ftSize;
}

FontSettings FreetypeFontSize::fontSettings() const
{
	return settings;
}

void FreetypeFontSize::deinit()
{
	for(auto &s : ftSize)
	{
		if(s)
		{
			//logMsg("freeing size %p", ftSize[i]);
			auto error = FT_Done_Size(s);
			assert(!error);
		}
	}
}

FreetypeGlyphImage::FreetypeGlyphImage(FreetypeGlyphImage &&o) noexcept
{
	*this = std::move(o);
}

FreetypeGlyphImage &FreetypeGlyphImage::operator=(FreetypeGlyphImage &&o) noexcept
{
	deinit();
	library = o.library;
	bitmap = std::exchange(o.bitmap, {});
	return *this;
}

FreetypeGlyphImage::~FreetypeGlyphImage()
{
	deinit();
}

void FreetypeGlyphImage::deinit()
{
	if(bitmap.buffer)
	{
		FT_Bitmap_Done(library, &bitmap);
		bitmap = {};
	}
}

PixmapView IG::GlyphImage::pixmap()
{
	return
		{
			{{(int)bitmap.width, (int)bitmap.rows}, IG::PixelFmtA8},
			bitmap.buffer,
			{bitmap.pitch, PixmapView::Units::BYTE}
		};
}

IG::GlyphImage::operator bool() const
{
	return (bool)bitmap.buffer;
}

}
