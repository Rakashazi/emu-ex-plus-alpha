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

#define thisModuleName "res:face"

#include <resource2/image/glyph/ResourceImageGlyph.h>
#include <util/strings.h>
#include "ResourceFace.hh"

#ifdef CONFIG_RESOURCE_FONT_FREETYPE

#include <resource2/font/freetype/ResourceFontFreetype.h>

#endif

void ResourceFace::initGlyphTable ()
{
	//logMsg("initGlyphTable");
	uint tableEntries = numDrawableAsciiChars;
	iterateTimes(tableEntries, i)
	{
		glyphTable[i].glyph = NULL;
	}
}

ResourceFace *ResourceFace::load(const char *path, FontSettings *set)
{
	ResourceFont *font = NULL;
	#ifdef CONFIG_RESOURCE_FONT_FREETYPE
	font = ResourceFontFreetype::load(path);
	#endif
	if(font == NULL)
		return NULL;

	return create(font, set);
}

ResourceFace *ResourceFace::load(Io* io, FontSettings *set)
{
	ResourceFont *font = NULL;
	#ifdef CONFIG_RESOURCE_FONT_FREETYPE
	font = ResourceFontFreetype::load(io);
	#endif
	if(font == NULL)
		return NULL;

	return create(font, set);
}

ResourceFace *ResourceFace::create(ResourceFace *face, FontSettings *set)
{
	return create(face->font, set);
}

ResourceFace *ResourceFace::create(ResourceFont *font, FontSettings *set)
{
	ResourceFace *inst = new ResourceFace;
	if(inst == NULL)
	{
		logErr("out of memory");
		return NULL;
	}

	logMsg("allocating glyph table, %d entries", numDrawableAsciiChars);
	inst->glyphTable = (GlyphEntry*)mem_alloc(sizeof(GlyphEntry) * numDrawableAsciiChars);
	if(!inst->glyphTable)
	{
		delete inst;
		logErr("out of memory");
		return 0;
	}

	//inst->init();
	inst->font = font;
	inst->initGlyphTable();

	if(set)
	{
		inst->settings = *set;
		inst->settings.process();
		font->newSize(&inst->settings, &inst->faceSize);
	}
	else
	{
		inst->settings.pixelHeight = 0;
		inst->settings.pixelWidth = 0;;
	}

	return inst;
}

void ResourceFace::free ()
{
	font->freeSize(faceSize);
	iterateTimes(numDrawableAsciiChars, i)
	{
		glyphTable[i].glyph->freeSafe();
	}
	mem_free(glyphTable);
	delete this;
}

uint ResourceFace::nominalHeight() const
{
	return nominalHeight_;
}

void ResourceFace::calcNominalHeight()
{
	//logMsg("calcNominalHeight");
	GlyphEntry *mGly = glyphEntry('M');
	GlyphEntry *gGly = glyphEntry('g');

	assert(mGly != NULL && gGly != NULL);

	nominalHeight_ = mGly->ySize + (gGly->ySize/2);
}

CallResult ResourceFace::applySettings (FontSettings set)
{
	set.process();
	if(set.pixelWidth < font->minUsablePixels())
		set.pixelWidth = font->minUsablePixels();
	if(set.pixelHeight < font->minUsablePixels())
		set.pixelHeight = font->minUsablePixels();

	if(set != settings)
	{
		if(settings.areValid())
		{
			logMsg("flushing glyph cache");
			font->freeSize(faceSize);
			iterateTimes(numDrawableAsciiChars, i)
			{
				glyphTable[i].glyph->freeSafe();
			}
		}

		settings = set;
		font->newSize(&settings, &faceSize);
		initGlyphTable();
		calcNominalHeight();
		return OK;
	}
	else
		return RESOURCE_FACE_SETTINGS_UNCHANGED;
}

int ResourceFace::maxDescender () const { font->applySize(faceSize); return font->currentFaceDescender(); }
int ResourceFace::maxAscender () const { font->applySize(faceSize); return font->currentFaceAscender(); }

CallResult ResourceFace::writeCurrentChar (Pixmap *out)
{
	uchar *bitmap = NULL;
	int bX = 0, bY = 0, bPitch;
	font->charBitmap(&bitmap, &bX, &bY, &bPitch);
	//logDMsg("copying char %dx%d, pitch %d", bX, bY, bPitch);
	//assert(bX == bPitch);

	/*if(bX <= 64)
	{
		iterateTimes(bY, yPos)
		{
			char line[3*64 + 1];
			uint linePos = 0;
			iterateTimes(bPitch, xPos)
			{
				sprintf(&line[linePos], "%02X ", bitmap[bPitch * yPos + xPos]);
				linePos += 3;
			}
			line[linePos] = 0;
			logMsg("%s", line);
		}
	}*/

	assert(bX != 0 && bY != 0 && bitmap != NULL);
	Pixmap src;
	src.init(bitmap, &PixelFormatI8, bX, bY, bPitch - bX);
	src.copy(0, 0, 0, 0, out, 0, 0);
	//memset ( out->data, 0xFF, 16 ); // test by filling with white
	return OK;
}

// make sure resourceFont_applySize() has been called before entering this function
CallResult ResourceFace::cacheChar (int c, int tableIdx)
{
	doOrReturn(font->activeChar(c));
	//logMsg("setting up table entry %d", tableIdx);
	glyphTable[tableIdx].xSize = font->currentCharXSize();
	glyphTable[tableIdx].ySize = font->currentCharYSize();
	glyphTable[tableIdx].xOffset = font->currentCharXOffset();
	glyphTable[tableIdx].yOffset = font->currentCharYOffset();
	glyphTable[tableIdx].xAdvance = font->currentCharXAdvance();
	glyphTable[tableIdx].glyph = font->createRenderable(c, this, &glyphTable[tableIdx]);
	return OK;
}

static CallResult mapCharToTable(int c, int *tableIdx)
{
	if(charIsDrawableAscii(c))
	{
		*tableIdx = c - firstDrawableAsciiChar;
		return OK;
	}
	else return INVALID_PARAMETER;
}

CallResult ResourceFace::precache (const char *string)
{
	assert(settings.areValid());
	font->applySize(faceSize);
	for(int i = 0, c = string[i]; c != 0; c = string[++i])
	{
		int tableIdx;
		if(mapCharToTable(c, &tableIdx) != OK)
		{
			//logMsg( "%c not a known drawable character, skipping", c);
			continue;
		}
		if(glyphTable[tableIdx].glyph != NULL)
		{
			//logMsg( "%c already cached", c);
			continue;
		}

		logMsg( "precaching char %c", c);
		doOrReturn(cacheChar(c, tableIdx));
	}
	return OK;
}

CallResult ResourceFace::getGlyph (ResourceImageGlyph **glyphAddr, int c)
{
	assert(settings.areValid());
	int tableIdx;
	doOrReturn(mapCharToTable(c, &tableIdx));
	if(glyphTable[tableIdx].glyph == NULL)
	{
		//logMsg("char %c not in table, caching now", c);
		font->applySize(faceSize);
		doOrReturn(cacheChar(c, tableIdx));
	}

	*glyphAddr = glyphTable[tableIdx].glyph;

	return OK;
}

void ResourceFace::lookupCharBounds (int c, int *width, int *height, int *yLineOffset, int *xOffset, int *xAdvance)
{
	int tableIdx = 0;
	if(mapCharToTable(c, &tableIdx) == OK)
	{
		*width = glyphTable[tableIdx].xSize;
		*height = glyphTable[tableIdx].ySize;
		*yLineOffset = glyphTable[tableIdx].yOffset;
		*xOffset = glyphTable[tableIdx].xOffset;
		*xAdvance = glyphTable[tableIdx].xAdvance;
	}
}

GlyphEntry *ResourceFace::glyphEntry (int c)
{
	assert(settings.areValid());
	int tableIdx;
	if(mapCharToTable(c, &tableIdx) != OK)
		return NULL;
	if(glyphTable[tableIdx].glyph == NULL)
	{
		//logMsg("char %c not in table, caching now", c);
		font->applySize(faceSize);
		if(cacheChar(c, tableIdx))
			return NULL;
	}

	return &glyphTable[tableIdx];
}

#undef thisModuleName
