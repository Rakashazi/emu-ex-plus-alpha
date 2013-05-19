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

#define thisModuleName "res:font:freetype"
#include <engine-globals.h>
#include <gfx/Gfx.hh>
#include <util/strings.h>
#include "ResourceFontFreetype.hh"
#ifdef CONFIG_PACKAGE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#ifdef CONFIG_PACKAGE_FONTCONFIG
static CallResult getFontFilenameWithPattern(FcPattern *pat, char *&filename, FcPattern *&matchPatOut)
{
	FcDefaultSubstitute(pat);
	if(!FcConfigSubstitute(nullptr, pat, FcMatchPattern))
	{
		logErr("error applying font substitutions");
		FcPatternDestroy(pat);
		return INVALID_PARAMETER;
	}
	FcResult result = FcResultMatch;
	auto matchPat = FcFontMatch(nullptr, pat, &result);
	FcPatternDestroy(pat);
	if(!matchPat || result == FcResultNoMatch)
	{
		logErr("fontconfig couldn't find a valid font");
		if(matchPat)
			FcPatternDestroy(matchPat);
		return INVALID_PARAMETER;
	}
	if(!FcPatternGetString(matchPat, FC_FILE, 0, (FcChar8**)&filename) == FcResultMatch)
	{
		logErr("fontconfig font missing file path");
		FcPatternDestroy(matchPat);
		return INVALID_PARAMETER;
	}
	matchPatOut = matchPat;
	return OK;
}

static bool addonSystemFontContainingChar(ResourceFontFreetype &font, int c)
{
	logMsg("looking for font with char: %c", c);
	auto pat = FcPatternCreate();
	if(!pat)
	{
		logErr("error allocating fontconfig pattern");
		return false;
	}
	auto charSet = FcCharSetCreate();
	FcCharSetAddChar(charSet, c);
	FcPatternAddCharSet(pat, FC_CHARSET, charSet);
	char *filename;
	FcPattern *matchPat;
	if(getFontFilenameWithPattern(pat, filename, matchPat) != OK)
	{
		FcCharSetDestroy(charSet);
		return false;
	}
	auto ret = font.loadIntoNextSlot((char*)filename);
	FcPatternDestroy(matchPat);
	FcCharSetDestroy(charSet);
	return ret == OK;
}
#endif

ResourceFontFreetype *ResourceFontFreetype::load()
{
	auto inst = new ResourceFontFreetype;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}
	return inst;
}

ResourceFontFreetype *ResourceFontFreetype::loadWithIoWithName(Io* io, const char *name)
{
	if(!io)
		return nullptr;
	//logMsg("loadWithIoWithName");
	auto inst = new ResourceFontFreetype;
	if(!inst)
	{
		logErr("out of memory");
		return nullptr;
	}

	//logMsg("fontData_open");
	if(inst->f[0].open(io) != OK)
	{
		logErr("error reading font");
		inst->free();
		return nullptr;
	}

	//logMsg("setting io addr %p", &i_io);

	if(inst->initWithName(name) != OK)
	{
		inst->free();
		return nullptr;
	}

	inst->usedCharSlots = 1;
	return inst;
}

ResourceFontFreetype *ResourceFontFreetype::load(Io* io)
{
	//logMsg("loadWithIo");
	return loadWithIoWithName(io, nullptr);
}

ResourceFontFreetype *ResourceFontFreetype::load(const char *name)
{
	/*ResourceFont *r = (ResourceFont*)Resource::findExisting(name);
	if(r != NULL)
		return r;*/

	//if(string_hasDotExtension(name, "ttf"))
	{
		//logMsg("suffix matches TT Font");
		auto io = IoSys::open(name, 0);
		if(!io)
		{
			logMsg("unable to open file");
			return nullptr;
		}
		return ResourceFontFreetype::loadWithIoWithName(io, name);
	}

	return nullptr;
}

CallResult ResourceFontFreetype::loadIntoSlot(Io *io, uint slot)
{
	if(f[slot].isOpen())
		f[slot].close(1);
	if(f[slot].open(io) != OK)
	{
		logErr("error reading font");
		return IO_ERROR;
	}
	usedCharSlots = slot+1;
	return OK;
}

CallResult ResourceFontFreetype::loadIntoSlot(const char *name, uint slot)
{
	auto io = IoSys::open(name, 0);
	if(!io)
	{
		logMsg("unable to open file %s", name);
		return IO_ERROR;
	}
	auto res = loadIntoSlot(io, slot);
	if(res != OK)
	{
		io->close();
		return res;
	}
	return OK;
}

CallResult ResourceFontFreetype::loadIntoNextSlot(const char *name)
{
	if(usedCharSlots == MAX_FREETYPE_SLOTS)
		return NO_FREE_ENTRIES;
	return loadIntoSlot(name, usedCharSlots);
}

void ResourceFontFreetype::free()
{
	iterateTimes(MAX_FREETYPE_SLOTS, i)
	{
		f[i].close(1);
	}
	delete this;
}

void ResourceFontFreetype::setMetrics(const FreetypeFontData &fontData, GlyphMetrics &metrics)
{
	metrics.xSize = fontData.charBitmapWidth();
	metrics.ySize = fontData.charBitmapHeight();
	metrics.xOffset = fontData.getCurrentCharBitmapLeft();
	metrics.yOffset = fontData.getCurrentCharBitmapTop();
	metrics.xAdvance = fontData.getCurrentCharBitmapXAdvance();
}

void ResourceFontFreetype::charBitmap(void *&bitmap, int &x, int &y, int &pitch)
{
	f[currCharSlot].accessCharBitmap(bitmap, x, y, pitch);
}

CallResult ResourceFontFreetype::activeChar(int idx, GlyphMetrics &metrics)
{
	//logMsg("active char: %c", idx);
	iterateTimes(usedCharSlots, i)
	{
		auto &font = f[i];
		if(!font.isOpen())
			continue;
		auto res = font.setActiveChar(idx);
		if(res != OK)
		{
			logMsg("glyph 0x%X not found in slot %d", idx, i);
			continue;
		}
		setMetrics(font, metrics);
		currCharSlot = i;
		return OK;
	}

	#ifdef CONFIG_PACKAGE_FONTCONFIG
	// try to find a font with the missing char and load into next free slot
	if(usedCharSlots != MAX_FREETYPE_SLOTS && addonSystemFontContainingChar(*this, idx))
	{
		uint newSlot = usedCharSlots-1;
		auto &font = f[newSlot];
		if(font.newSize(activeFontSizeData->settings.pixelWidth, activeFontSizeData->settings.pixelHeight,
			&activeFontSizeData->size[newSlot]) != OK)
		{
			logErr("couldn't allocate font size");
			return NOT_FOUND;
		}
		auto res = font.setActiveChar(idx);
		if(res != OK)
		{
			logMsg("glyph 0x%X still not found", idx);
			return NOT_FOUND;
		}
		setMetrics(font, metrics);
		currCharSlot = newSlot;
		return OK;
	}
	#endif

	return NOT_FOUND;
}

/*int ResourceFontFreetype::currentFaceDescender () const //+
{ return f.maxDescender(); }
int ResourceFontFreetype::currentFaceAscender () const //+
{ return f.maxAscender(); }*/

CallResult ResourceFontFreetype::newSize(const FontSettings &settings, FontSizeRef &sizeRef)
{
	auto sizeData = new FontSizeData(settings);
	if(!sizeData)
	{
		logErr("couldn't allocate size data");
		return OUT_OF_MEMORY;
	}
	// create FT_Size objects for slots in use
	iterateTimes(usedCharSlots, i)
	{
		if(!f[i].isOpen())
		{
			//sizeRef.size[i] = nullptr;
			continue;
		}
		auto res = f[i].newSize(settings.pixelWidth, settings.pixelHeight, &sizeData->size[i]);
		if(res != OK)
		{
			// TODO: cleanup already allocated sizes
			return res;
		}
	}
	sizeRef.ptr = sizeData;
	return OK;
}

CallResult ResourceFontFreetype::applySize(FontSizeRef &sizeRef)
{
	auto &sizeData = *((FontSizeData*)sizeRef.ptr);
	iterateTimes(usedCharSlots, i)
	{
		if(!sizeData.size[i])
			continue;
		auto res = f[i].applySize(sizeData.size[i]);
		if(res != OK)
		{
			return res;
		}
	}
	activeFontSizeData = (FontSizeData*)sizeRef.ptr;
	return OK;
}

void ResourceFontFreetype::freeSize(FontSizeRef &sizeRef)
{
	auto &sizeData = *((FontSizeData*)sizeRef.ptr);
	iterateTimes(usedCharSlots, i)
	{
		if(sizeData.size[i])
			f[i].freeSize(sizeData.size[i]);
	}
	if(&sizeData == activeFontSizeData)
		activeFontSizeData = nullptr;
	delete ((FontSizeData*)sizeRef.ptr);
}
