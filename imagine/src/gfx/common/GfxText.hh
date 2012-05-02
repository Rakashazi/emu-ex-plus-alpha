#pragma once

#if defined(CONFIG_RESOURCE_FACE)
	#include <resource2/face/ResourceFace.hh>
	#include <resource2/image/ResourceImage.h>
#endif

#include <ctype.h>
#include <gfx/GfxText.hh>

static bool textIsInit = 0;
GfxSprite gfxText_spr;

void GfxText::init()
{
	if(!textIsInit)
	{
		gfxText_spr.init(0, 0, 1, 1);
		textIsInit = 1;
	}
	face = NULL;
	str = NULL;
	slen = 0;
	//inst->spr.img = NULL;
	spaceSize = 0;
	nominalHeight = 0;
	yLineStart = 0;
	maxLines = 0;
	maxLineSize = 0;
}

void GfxText::deinit()
{
	/*if(inst->spr.img != NULL)
	{
		gfxSprite_deinit(&inst->spr);
	}*/
}

void GfxText::setString(const char *str)
{
	assert(str);
	this->str = str;
	slen = strlen(str);
}

void GfxText::setFace(ResourceFace *face)
{
	this->face = face;
}

/*static bool isStartOfTextUnit(char prev, char c)
{
	assert(c != 0);
	if(prev == 0 ||
		isblank(c) ||
		(isblank(prev) && isalnum(c)))
	{
		logMsg("%c is start of text unit", c);
		return 1;
	}
	return 0;
}*/

static GC xSizeOfChar(ResourceFace *face, char c, GC spaceX)
{
	assert(c != '\0');
	if(c == ' ')
		return spaceX;
	if(c == '\n')
		return 0;

	GlyphEntry *gly = face->glyphEntry(c);
	if(gly != NULL)
		return Gfx::iXSize(gly->xAdvance);
	else
		return 0;
}

static GC xSizeOfTextUnit(ResourceFace *face, const char *s, GC spaceX)
{
	if(s[0] == ' ')
		return spaceX;
	if(s[0] == '\n' || s[0] == '\0')
		return 0;

	GC xSize = 0;
	GlyphEntry *gly = face->glyphEntry(s[0]);
	if(gly != NULL)
		xSize = Gfx::iXSize(gly->xAdvance);

	int i = 1;
	while(isgraph(s[i]))
	{
		xSize += xSizeOfChar(face, s[i], spaceX);
		i++;
	}
	return xSize;
}

static int charsInNextTextUnit(const char *s)
{
	if(s[0] == '\0')
		return 0;
	if(isspace(s[0]))
		return 1;

	int  i = 0;
	while(isgraph(s[i]))
	{
		i++;
	}

	return i == 0 ? 1 : i;
}

static uint charsInLine(ResourceFace *face, const char *s, GC spaceX, GC xMaxSize, GC *xSize)
{
	GC size = 0;
	int chars, total = 0;
	while((chars = charsInNextTextUnit(s)) > 0)
	{
		if(s[0] == '\n')
		{
			total++;
			break;
		}

		GC unitSize = xSizeOfTextUnit(face, s, spaceX);
		if(total != 0 && xMaxSize != (GC)0 && unitSize + size > xMaxSize)
			break;

		size += unitSize;
		s += chars;
		total += chars;
	}

	if(xSize != NULL)
		*xSize = size;
	return total;
}

void GfxText::compile()
{
	assert(face);
	assert(str);
	
	// TODO: move calc into Face class
	GlyphEntry *mGly = face->glyphEntry('M');
	GlyphEntry *gGly = face->glyphEntry('g');
	
	if(mGly == NULL || gGly == NULL)
	{
		logErr("error reading measurement glyphs to compile text");
		return;
	}

	yLineStart = Gfx::alignYToPixel(Gfx::iYSize(gGly->ySize - gGly->yOffset));
	
	int spaceSizeI = mGly->xSize/2;
	spaceSize = Gfx::iXSize(spaceSizeI);
	nominalHeight = Gfx::alignYToPixel(Gfx::iYSize(mGly->ySize) + Gfx::iYSize(gGly->ySize/2));
	int maxLineSizeI = Gfx::toIXSize(maxLineSize);
	//logMsg("max line size %f", maxLineSize);
	
	int lines = 0;
	// calc x text size
	//int xLineSize = 0, xSizeI = 0;
	GC xLineSize = 0, maxXLineSize = 0;
	const char *s = str; uint charsToHandle = 0;
	while(s[0] != '\0' && (charsToHandle = charsInLine(face, s, spaceSize, maxLineSize, &xLineSize)) > 0)
	{
		lines++;
		maxXLineSize = IG::max(xLineSize, maxXLineSize);
		s += charsToHandle;
	}
	/*iterateTimes(slen, i)
	{
		GlyphEntry *gly = face->glyphEntry(str[i]);
		if(gly == NULL)
		{
			if(str[i] == '\n')
			{
				lines++;
				xSizeI = IG::max(xLineSize, xSizeI);
				//logMsg("new line, prev line was %d pixels", xLineSize);
				xLineSize = 0;
			}
			else
				xLineSize += spaceSizeI;
			continue;
		}
		
		xLineSize += gly->xAdvance;
	}*/
	//logMsg("x size of %d pixels", max(xLineSize, xSize));
	//xSize = gfx_iXSize(IG::max(xLineSize, xSizeI));
	xSize = maxXLineSize;
	ySize = Gfx::alignYToPixel(nominalHeight * (GC)lines);
}

void GfxText::draw(GC xPos, GC yPos, _2DOrigin o, _2DOrigin align) const
{
	using namespace Gfx;
	assert(face != NULL && str != NULL);
	xPos = o.adjustX(xPos, xSize, LT2DO);
	yPos = o.adjustY(yPos, ySize, LT2DO);
	//xPos = floorMult(xPos, xPerI);
	//yPos = floorMult(yPos, yPerI);
	//xPos = floor(xPos);
	//yPos = floor(yPos);
	yPos -= nominalHeight - yLineStart;
	GC xOrig = xPos;
	
	//logMsg("printing %s", inst->str);
	//logMsg("drawing text at %f,%f", xPos, yPos);
	uchar line = 1;
	GC xLineSize = 0;//, xMaxSize = 0;
	uint charsToHandle = 0;
	const char *s = str;
	setBlendMode(BLEND_MODE_INTENSITY);
	while(s[0] != '\0' && (charsToHandle = charsInLine(face, s, spaceSize, maxLineSize, &xLineSize)) > 0)
	{
		bool setXPos = 0;
		if(s == str)
			setXPos = 1;
		else if(line != maxLines) // break lines on loop 2+
		{
			//logMsg("breaking line");
			line++;
			yPos -= nominalHeight;
			setXPos = 1;
		}

		if(setXPos)
		{
			xPos = alignXToPixel(LT2DO.adjustX(xOrig, xSize-xLineSize, align));
		}

		GC xViewLimit = proj.wHalf;
		iterateTimes(charsToHandle, i)
		{
			if(s[i] == '\n')
			{
				if(line == maxLines)
				{
					// convert newline to space if max lines reached
					xPos += spaceSize;
					continue;
				}
				else
					break;
			}
			GlyphEntry *gly = face->glyphEntry(s[i]);

			if(gly == NULL)
			{
				xPos += spaceSize;
				continue;
			}
			if(xPos >= xViewLimit)
			{
				//logMsg("skipped %c, off right screen edge", s[i]);
				continue;
			}
			GC xSize = iXSize(gly->xSize);

			gfxText_spr.setImg(gly->glyph);
			loadTranslate(xPos + iXSize(gly->xOffset), yPos - iYSize(gly->ySize - gly->yOffset));
			applyScale(xSize, iYSize(gly->ySize));
			xPos += Gfx::iXSize(gly->xAdvance);

			//logMsg("drawing");
			gfxText_spr.draw(0);
		}

		s += charsToHandle;
	}
}
