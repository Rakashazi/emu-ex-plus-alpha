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

#define LOGTAG "GlyphTexture"

#include <imagine/util/bitset.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/data-type/image/GfxImageSource.hh>
#include <imagine/logger/logger.h>
#include <cstdlib>

void GfxImageSource::freePixmap() {}

namespace Gfx
{

static constexpr char firstDrawableAsciiChar = '!';
static constexpr char lastDrawableAsciiChar = '~';
static constexpr uint32_t numDrawableAsciiChars = (lastDrawableAsciiChar - firstDrawableAsciiChar) + 1;

// definitions for the Unicode Basic Multilingual Plane (BMP)
static constexpr uint32_t unicodeBmpChars = 0xFFFE;

// location & size of the surrogate/private chars
static constexpr uint32_t unicodeBmpPrivateStart = 0xD800, unicodeBmpPrivateEnd = 0xF8FF;
static constexpr uint32_t unicodeBmpPrivateChars = 0x2100;

static constexpr uint32_t unicodeBmpUsedChars = unicodeBmpChars - unicodeBmpPrivateChars;

static constexpr uint32_t glyphTableEntries = GlyphTextureSet::supportsUnicode ? unicodeBmpUsedChars : numDrawableAsciiChars;

static std::errc mapCharToTable(uint32_t c, uint32_t &tableIdx);

class GfxGlyphImage : public GfxImageSource
{
public:
	GfxGlyphImage(IG::GlyphImage glyphBuff): lockBuff{std::move(glyphBuff)} {}

	std::errc write(IG::Pixmap out) final
	{
		auto src = lockBuff.pixmap();
		//logDMsg("copying char %dx%d, pitch %d to dest %dx%d, pitch %d", src.x, src.y, src.pitch, out.x, out.y, out.pitch);
		assert(src.w() != 0 && src.h() != 0 && src.data());
		if(Config::envIsAndroid && !src.pitchBytes()) // Hack for JXD S7300B which returns y = x, and pitch = 0
		{
			logWarn("invalid pitch returned for char bitmap");
			src = {{out.size(), out.format()}, src.data(), {out.pitchBytes(), IG::Pixmap::BYTE_UNITS}};
		}
		out.write(src, {});
		return {};
	}

	IG::Pixmap pixmapView() final
	{
		assert((bool)*this);
		return lockBuff.pixmap();
	}

	void freePixmap() final
	{
		lockBuff.unlock();
	}

	explicit operator bool() const final
	{
		return (bool)lockBuff;
	}

protected:
	IG::GlyphImage lockBuff{};
};

static int charIsDrawableAscii(int c)
{
	if(c >= firstDrawableAsciiChar && c <= lastDrawableAsciiChar)
		return 1;
	else return 0;
}

static int charIsDrawableUnicode(int c)
{
	return !(
			(c >= 0x0 && c < '!')
			|| (c > '~' && c < 0xA1)
			|| (c >= 0x2000 && c <= 0x200F)
			|| (c == 0x3000)
			);
}

void GlyphTextureSet::resetGlyphTable()
{
	if(!usedGlyphTableBits)
		return;
	logMsg("resetting glyph table");
	usedGlyphTableBits = 0;
	glyphTable.resetElements();
}

void GlyphTextureSet::freeCaches(uint32_t purgeBits)
{
	if(purgeBits == ~0u)
	{
		// free the whole table
		resetGlyphTable();
		return;
	}
	auto tableBits = usedGlyphTableBits;
	iterateTimes(32, i)
	{
		if((tableBits & 1) && (purgeBits & 1))
		{
			logMsg("purging glyphs from table range %d/31", i);
			int firstChar = i << 11;
			iterateTimesFromStart(2048, firstChar, c)
			{
				uint32_t tableIdx;
				if((bool)mapCharToTable(c, tableIdx))
				{
					//logMsg( "%c not a known drawable character, skipping", c);
					continue;
				}
				glyphTable[i].~GlyphEntry();
			}
			usedGlyphTableBits = IG::clearBits(usedGlyphTableBits, IG::bit(i));
		}
		tableBits >>= 1;
		purgeBits >>= 1;
	}
}

GlyphTextureSet::GlyphTextureSet(Renderer &r, const char *path, IG::FontSettings set):
	GlyphTextureSet(r, std::make_unique<IG::Font>(path), set)
{}

GlyphTextureSet::GlyphTextureSet(Renderer &r, GenericIO io, IG::FontSettings set):
	GlyphTextureSet(r, std::make_unique<IG::Font>(std::move(io)), set)
{}

GlyphTextureSet::GlyphTextureSet(Renderer &r, std::unique_ptr<IG::Font> font, IG::FontSettings set):
	font{std::move(font)}
{
	glyphTable.resize(glyphTableEntries);
	if(glyphTable.empty())
	{
		logErr("failed allocating glyph table (%d entries)", glyphTableEntries);
		return;
	}
	if(set)
	{
		setFontSettings(r, set);
	}
}

GlyphTextureSet GlyphTextureSet::makeSystem(Renderer &r, IG::FontSettings set)
{
	return {r, std::make_unique<IG::Font>(IG::Font::makeSystem(r.appContext())), set};
}

GlyphTextureSet GlyphTextureSet::makeBoldSystem(Renderer &r, IG::FontSettings set)
{
	return {r, std::make_unique<IG::Font>(IG::Font::makeBoldSystem(r.appContext())), set};
}

GlyphTextureSet GlyphTextureSet::makeFromAsset(Renderer &r,
	const char *name, IG::FontSettings set, const char *appName)
{
	return {r, r.appContext().openAsset(name, IO::AccessHint::ALL, appName).makeGeneric(), set};
}

uint32_t GlyphTextureSet::nominalHeight() const
{
	return nominalHeight_;
}

void GlyphTextureSet::calcNominalHeight(Renderer &r)
{
	//logMsg("calcNominalHeight");
	GlyphEntry *mGly = glyphEntry(r, 'M');
	GlyphEntry *gGly = glyphEntry(r, 'g');

	assert(mGly && gGly);

	nominalHeight_ = mGly->metrics.ySize + (gGly->metrics.ySize/2);
}

IG::FontSettings GlyphTextureSet::fontSettings() const
{
	return settings;
}

bool GlyphTextureSet::setFontSettings(Renderer &r, IG::FontSettings set)
{
	if(set.pixelWidth() < font->minUsablePixels())
		set.setPixelWidth(font->minUsablePixels());
	if(set.pixelHeight() < font->minUsablePixels())
		set.setPixelHeight(font->minUsablePixels());
	if(set == settings)
		return false;
	resetGlyphTable();
	settings = set;
	std::errc ec{};
	faceSize = font->makeSize(settings, ec);
	calcNominalHeight(r);
	return true;
}

std::errc GlyphTextureSet::cacheChar(Renderer &r, int c, int tableIdx)
{
	assert(settings);
	if(glyphTable[tableIdx].metrics.ySize == -1)
	{
		// failed to previously cache char
		return std::errc::invalid_argument;
	}
	// make sure applySize() has been called on the font object first
	std::errc ec{};
	auto res = font->glyph(c, faceSize, ec);
	if((bool)ec)
	{
		// mark failed attempt
		glyphTable[tableIdx].metrics.ySize = -1;
		return ec;
	}
	//logMsg("setting up table entry %d", tableIdx);
	glyphTable[tableIdx].metrics = res.metrics;
	auto img = GfxGlyphImage(std::move(res.image));
	glyphTable[tableIdx].glyph_ = r.makePixmapTexture(img, &r.make(glyphCommonTextureSampler), false);
	usedGlyphTableBits |= IG::bit((c >> 11) & 0x1F); // use upper 5 BMP plane bits to map in range 0-31
	//logMsg("used table bits 0x%X", usedGlyphTableBits);
	return {};
}

static std::errc mapCharToTable(uint32_t c, uint32_t &tableIdx)
{
	if(GlyphTextureSet::supportsUnicode)
	{
		//logMsg("mapping char 0x%X", c);
		if(c < unicodeBmpChars && charIsDrawableUnicode(c))
		{
			if(c < unicodeBmpPrivateStart)
			{
				tableIdx = c;
				return {};
			}
			else if(c > unicodeBmpPrivateEnd)
			{
				tableIdx = c - unicodeBmpPrivateChars; // surrogate & private chars are a hole in the table
				return {};
			}
			else
			{
				return std::errc::invalid_argument;
			}
		}
		else
			return std::errc::invalid_argument;
	}
	else
	{
		if(charIsDrawableAscii(c))
		{
			tableIdx = c - firstDrawableAsciiChar;
			return {};
		}
		else
			return std::errc::invalid_argument;
	}
}

// TODO: update for unicode
unsigned GlyphTextureSet::precache(Renderer &r, const char *string)
{
	assert(settings);
	unsigned glyphsCached = 0;
	iterateTimes(strlen(string), i)
	{
		auto c = string[i];
		uint32_t tableIdx;
		if((bool)mapCharToTable(c, tableIdx))
		{
			//logMsg( "%c not a known drawable character, skipping", c);
			continue;
		}
		if(glyphTable[tableIdx].glyph())
		{
			//logMsg( "%c already cached", c);
			continue;
		}
		logMsg("making glyph:%c (0x%X)", c, c);
		cacheChar(r, c, tableIdx);
		glyphsCached++;
	}
	return glyphsCached;
}

GlyphEntry *GlyphTextureSet::glyphEntry(Renderer &r, int c, bool allowCache)
{
	assert(settings);
	uint32_t tableIdx;
	if((bool)mapCharToTable(c, tableIdx))
		return nullptr;
	assert(tableIdx < glyphTableEntries);
	if(!glyphTable[tableIdx].glyph())
	{
		if(!allowCache)
		{
			logErr("cannot make glyph:%c (0x%X) during draw operation", c, c);
			return nullptr;
		}
		if((bool)cacheChar(r, c, tableIdx))
			return nullptr;
		//logMsg("glyph:%c (0x%X) was not in table", c, c);
	}
	return &glyphTable[tableIdx];
}

}
