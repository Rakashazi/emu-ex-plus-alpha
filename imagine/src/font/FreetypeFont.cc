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
#include <imagine/util/string.h>
#include <imagine/io/FileIO.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#ifdef CONFIG_PACKAGE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_SIZES_H

static FT_Library library{};

#ifdef CONFIG_PACKAGE_FONTCONFIG
static FcConfig *fcConf{};

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
	FS::PathString path;
	string_copy(path, (char*)patternStr);
	return path;
}

static FS::PathString fontPathContainingChar(int c, int weight)
{
	if(!fcConf)
	{
		fcConf = FcInitLoadConfigAndFonts();
		if(!fcConf)
		{
			logErr("error initializing fontconfig");
			return {};
		}
	}
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
	FS::PathString path = fontPathWithPattern(pat);
	if(!strlen(path.data()))
	{
		return {};
	}
	return path;
}

[[gnu::destructor]] static void finalizeFc()
{
	if(fcConf)
	{
		FcConfigDestroy(fcConf);
		FcFini();
	}
}
#endif

namespace IG
{

static FT_Size makeFTSize(FT_Face face, int x, int y, std::errc &ec)
{
	logMsg("creating new size object, %dx%d pixels", x, y);
	FT_Size size{};
	auto error = FT_New_Size(face, &size);
	if(error)
	{
		logErr("error creating new size object");
		ec = std::errc::invalid_argument;
		return {};
	}
	error = FT_Activate_Size(size);
	if(error)
	{
		logErr("error activating size object");
		ec = std::errc::invalid_argument;
		return {};
	}
	error = FT_Set_Pixel_Sizes(face, x, y);
	if(error)
	{
		logErr("error occurred setting character pixel size");
		ec = std::errc::invalid_argument;
		return {};
	}
	ec = {};
	//logMsg("Face max bounds %dx%d,%dx%d, units per EM %d", face->bbox.xMin, face->bbox.xMax, face->bbox.yMin, face->bbox.yMax, face->units_per_EM);
	//logMsg("scaled ascender x descender %dx%d", (int)size->metrics.ascender >> 6, (int)size->metrics.descender >> 6);
	return size;
}

static FreetypeFont::GlyphRenderData makeGlyphRenderDataWithFace(FT_Face face, int c, bool keepPixData, std::errc &ec)
{
	auto idx = FT_Get_Char_Index(face, c);
	if(!idx)
	{
		ec = std::errc::invalid_argument;
		return {};
	}
	auto error = FT_Load_Glyph(face, idx, FT_LOAD_RENDER);
	if(error)
	{
		logErr("error occurred loading/rendering character 0x%X", c);
		ec = std::errc::invalid_argument;
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
			ec = std::errc::invalid_argument;
			return {};
		}
		assert(bitmap.num_grays == 2); // only handle 2 gray levels for now
		//logMsg("new bitmap has %d gray levels", convBitmap.num_grays);
		// scale 1-bit values to 8-bit range
		iterateTimes(bitmap.rows, y)
		{
			iterateTimes(bitmap.width, x)
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
	metrics.xSize = bitmap.width;
	metrics.ySize = bitmap.rows;
	metrics.xOffset = glyph->bitmap_left;
	metrics.yOffset = glyph->bitmap_top;
	metrics.xAdvance = glyph->advance.x >> 6;;
	if(!keepPixData)
	{
		FT_Bitmap_Done(library, &bitmap);
	}
	ec = {};
	return {metrics, bitmap};
}

std::errc FreetypeFaceData::openFont(GenericIO file)
{
	if(!file)
		return std::errc::invalid_argument;
	if(!library) [[unlikely]]
	{
		auto error = FT_Init_FreeType(&library);
		if(error)
		{
			logErr("error in FT_Init_FreeType");
			return std::errc::invalid_argument;
		}
		/*FT_Int major, minor, patch;
		FT_Library_Version(library, &major, &minor, &patch);
		logMsg("init freetype version %d.%d.%d", (int)major, (int)minor, (int)patch);*/
	}
	streamRec.size = file.size();
	auto fileOffset =	file.tell();
	streamRec.pos = fileOffset;
	streamRec.descriptor.pointer = file.release();
	streamRec.read = [](FT_Stream stream, unsigned long offset,
		unsigned char* buffer, unsigned long count) -> unsigned long
		{
			auto &io = *((IO*)stream->descriptor.pointer);
			stream->pos = offset;
			if(!count) // no bytes to read
				return 0;
			auto bytesRead = io.readAtPos(buffer, count, offset);
			if(bytesRead == -1)
			{
				logErr("error reading bytes in IO func");
				return 0;
			}
			return bytesRead;
		};
	FT_Open_Args openS{};
	openS.flags = FT_OPEN_STREAM;
	openS.stream = &streamRec;
	auto error = FT_Open_Face(library, &openS, 0, &face);
	if(error == FT_Err_Unknown_File_Format)
	{
		logErr("unknown font format");
		return std::errc::invalid_argument;
	}
	else if(error)
	{
		logErr("error occurred opening the font");
		return std::errc::io_error;
	}
	return {};
}

Font::Font(GenericIO io)
{
	loadIntoNextSlot(std::move(io));
}

Font::Font(const char *name)
{
	FileIO io;
	io.open(name, IO::AccessHint::ALL);
	if(!io)
	{
		logMsg("unable to open file");
		return;
	}
	loadIntoNextSlot(io.makeGeneric());
}

Font Font::makeSystem(Base::ApplicationContext)
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	logMsg("locating system fonts with fontconfig");
	// Let fontconfig handle loading specific fonts on-demand
	return {};
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font Font::makeBoldSystem(Base::ApplicationContext ctx)
{
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	Font font = makeSystem(ctx);
	font.isBold = true;
	return font;
	#else
	return makeFromAsset("Vera.ttf");
	#endif
}

Font Font::makeFromAsset(Base::ApplicationContext ctx, const char *name, const char *appName)
{
	return {ctx.openAsset(name, IO::AccessHint::ALL, appName).makeGeneric()};
}

FreetypeFont::FreetypeFont(FreetypeFont &&o)
{
	*this = std::move(o);
}

FreetypeFont &FreetypeFont::operator=(FreetypeFont &&o)
{
	deinit();
	f = std::exchange(o.f, {});
	isBold = o.isBold;
	return *this;
}

FreetypeFont::~FreetypeFont()
{
	deinit();
}

Font::operator bool() const
{
	return f.size();
}

void FreetypeFont::deinit()
{
	for(auto &e : f)
	{
		if(e.face)
		{
			FT_Done_Face(e.face);
		}
		if(e.streamRec.descriptor.pointer)
		{
			delete (IO*)e.streamRec.descriptor.pointer;
		}
	}
}

std::errc FreetypeFont::loadIntoNextSlot(GenericIO io)
{
	if(f.isFull())
		return std::errc::no_space_on_device;
	auto ec = f.emplace_back().openFont(std::move(io));
	if((bool)ec)
	{
		logErr("error reading font");
		f.pop_back();
		return ec;
	}
	return {};
}

std::errc FreetypeFont::loadIntoNextSlot(const char *name)
{
	if(f.isFull())
		return std::errc::no_space_on_device;
	FileIO io;
	io.open(name, IO::AccessHint::ALL);
	if(!io)
	{
		logMsg("unable to open file %s", name);
		return std::errc::invalid_argument;
	}
	if(auto ec = loadIntoNextSlot(io.makeGeneric());
		(bool)ec)
	{
		return ec;
	}
	return {};
}

FreetypeFont::GlyphRenderData FreetypeFont::makeGlyphRenderData(int idx, FreetypeFontSize &fontSize, bool keepPixData, std::errc &ec)
{
	iterateTimes(f.size(), i)
	{
		auto &font = f[i];
		if(!font.face)
			continue;
		auto ftError = FT_Activate_Size(fontSize.sizeArray()[i]);
		if(ftError)
		{
			logErr("error activating size object");
			ec = std::errc::invalid_argument;
			return {};
		}
		std::errc ec;
		auto data = makeGlyphRenderDataWithFace(font.face, idx, keepPixData, ec);
		if((bool)ec)
		{
			logMsg("glyph 0x%X not found in slot %d", idx, i);
			continue;
		}
		return data;
	}
	#ifdef CONFIG_PACKAGE_FONTCONFIG
	// try to find a font with the missing char and load into next free slot
	if(f.isFull())
	{
		logErr("no slots left");
		ec = std::errc::no_space_on_device;
		return {};
	}
	auto fontPath = fontPathContainingChar(idx, isBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM);
	if(!strlen(fontPath.data()))
	{
		logErr("no font file found for char %c (0x%X)", idx, idx);
		ec = std::errc::no_such_file_or_directory;
		return {};
	}
	uint32_t newSlot = f.size();
	ec = loadIntoNextSlot(fontPath.data());
	if((bool)ec)
		return {};
	auto &font = f[newSlot];
	auto settings = fontSize.fontSettings();
	fontSize.sizeArray()[newSlot] = makeFTSize(font.face, settings.pixelWidth(), settings.pixelHeight(), ec);
	if((bool)ec)
	{
		logErr("couldn't allocate font size");
		return {};
	}
	auto data = makeGlyphRenderDataWithFace(font.face, idx, keepPixData, ec);
	if((bool)ec)
	{
		logMsg("glyph 0x%X still not found", idx);
		return {};
	}
	return data;
	#else
	ec = std::errc::invalid_argument;
	return {};
	#endif
}

Font::Glyph Font::glyph(int idx, FontSize &size, std::errc &ec)
{
	auto data = makeGlyphRenderData(idx, size, true, ec);
	if((bool)ec)
	{
		return {};
	}
	return {{data.bitmap}, data.metrics};
}

GlyphMetrics Font::metrics(int idx, FontSize &size, std::errc &ec)
{
	auto data = makeGlyphRenderData(idx, size, false, ec);
	if((bool)ec)
	{
		return {};
	}
	return data.metrics;
}

FontSize Font::makeSize(FontSettings settings, std::errc &ec)
{
	FontSize size{settings};
	// create FT_Size objects for slots in use
	iterateTimes(f.size(), i)
	{
		if(!f[i].face)
		{
			continue;
		}
		size.sizeArray()[i] = makeFTSize(f[i].face, settings.pixelWidth(), settings.pixelHeight(), ec);
		if((bool)ec)
		{
			return {};
		}
	}
	ec = {};
	return size;
}

FreetypeFontSize::FreetypeFontSize(FontSettings settings): settings{settings} {}

FreetypeFontSize::~FreetypeFontSize()
{
	deinit();
}

FreetypeFontSize::FreetypeFontSize(FreetypeFontSize &&o)
{
	*this = std::move(o);
}

FreetypeFontSize &FreetypeFontSize::operator=(FreetypeFontSize &&o)
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
	iterateTimes(std::size(ftSize), i)
	{
		if(ftSize[i])
		{
			//logMsg("freeing size %p", ftSize[i]);
			auto error = FT_Done_Size(ftSize[i]);
			assert(!error);
		}
	}
}

FreetypeGlyphImage::FreetypeGlyphImage(FT_Bitmap bitmap): bitmap{bitmap} {}

FreetypeGlyphImage::FreetypeGlyphImage(FreetypeGlyphImage &&o)
{
	*this = std::move(o);
}

FreetypeGlyphImage &FreetypeGlyphImage::operator=(FreetypeGlyphImage &&o)
{
	static_cast<GlyphImage*>(this)->unlock();
	bitmap = std::exchange(o.bitmap, {});
	return *this;
}

FreetypeGlyphImage::~FreetypeGlyphImage()
{
	static_cast<GlyphImage*>(this)->unlock();
}

void GlyphImage::unlock()
{
	if(bitmap.buffer)
	{
		FT_Bitmap_Done(library, &bitmap);
		bitmap = {};
	}
}

IG::Pixmap IG::GlyphImage::pixmap()
{
	return
		{
			{{(int)bitmap.width, (int)bitmap.rows}, IG::PIXEL_FMT_A8},
			bitmap.buffer,
			{(uint32_t)bitmap.pitch, IG::Pixmap::BYTE_UNITS}
		};
}

IG::GlyphImage::operator bool() const
{
	return (bool)bitmap.buffer;
}

}
