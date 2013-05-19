#pragma once

#if defined(CONFIG_RESOURCE_FACE)
	#include <resource2/face/ResourceFace.hh>
#endif

#include <cctype>
#include <gfx/GfxText.hh>
#include <util/strings.h>
#include <algorithm>

namespace Gfx
{

void Text::deinit()
{
	spaceSize = 0;
	nominalHeight = 0;
	yLineStart = 0;
	xSize = ySize = 0;
	chars = 0;
	lines = 0;
	if(lineInfo)
	{
		mem_free(lineInfo);
		lineInfo = nullptr;
	}
}

void Text::setString(const char *str)
{
	assert(str);
	this->str = str;
}

void Text::setFace(ResourceFace *face)
{
	assert(face);
	this->face = face;
}

static GC xSizeOfChar(ResourceFace *face, int c, GC spaceX)
{
	assert(c != '\0');
	if(c == ' ')
		return spaceX;
	if(c == '\n')
		return 0;

	GlyphEntry *gly = face->glyphEntry(c);
	if(gly != NULL)
		return Gfx::iXSize(gly->metrics.xAdvance);
	else
		return 0;
}

void Text::compile()
{
	assert(face);
	assert(str);
	//logMsg("compiling text %s", str);
	
	// TODO: move calc into Face class
	GlyphEntry *mGly = face->glyphEntry('M');
	GlyphEntry *gGly = face->glyphEntry('g');
	
	if(!mGly || !gGly)
	{
		logErr("error reading measurement glyphs to compile text");
		return;
	}

	yLineStart = Gfx::alignYToPixel(Gfx::iYSize(gGly->metrics.ySize - gGly->metrics.yOffset));
	
	int spaceSizeI = mGly->metrics.xSize/2;
	spaceSize = Gfx::iXSize(spaceSizeI);
	nominalHeight = Gfx::alignYToPixel(Gfx::iYSize(mGly->metrics.ySize) + Gfx::iYSize(gGly->metrics.ySize/2));
	//int maxLineSizeI = Gfx::toIXSize(maxLineSize);
	//logMsg("max line size %f", maxLineSize);
	
	lines = 1;
	GC xLineSize = 0, maxXLineSize = 0;
	const char *s = str;
	uint c = 0, prevC = 0;
	GC textBlockSize = 0;
	uint textBlockIdx = 0, currLineIdx = 0;
	uint charIdx = 0, charsInLine = 0;
	CallResult res;
	while((res = string_convertCharCode(&s, c)) == OK)
	{
		auto cSize = xSizeOfChar(face, c, spaceSize);
		charsInLine++;

		// Is this the start of a text block?
		if(isgraph(c) && isspace(prevC))
		{
			textBlockIdx = charIdx;
			textBlockSize = 0;
		}
		xLineSize += cSize;
		textBlockSize += cSize;
#ifndef CONFIG_BASE_PS3
		if(lines < maxLines)
		{
			bool wentToNextLine = 0;
			// Go to next line?
			if(c == '\n')
			{
				wentToNextLine = 1;
				lineInfo = (LineInfo*)mem_realloc(lineInfo, sizeof(LineInfo)*(lines+1));
				assert(lineInfo);
				// Don't break text
				//logMsg("new line %d without text break @ char %d, %d chars in line", lines+1, charIdx, charsInLine);
				lineInfo[lines-1].size = xLineSize;
				lineInfo[lines-1].chars = charsInLine;
				maxXLineSize = std::max(xLineSize, maxXLineSize);
				xLineSize = 0;
				charsInLine = 0;
			}
			else if(xLineSize > maxLineSize && textBlockIdx != currLineIdx)
			{
				wentToNextLine = 1;
				lineInfo = (LineInfo*)mem_realloc(lineInfo, sizeof(LineInfo)*(lines+1));
				assert(lineInfo);
				// Line has more than 1 block and is too big, needs text break
				//logMsg("new line %d with text break @ char %d, %d chars in line", lines+1, charIdx, charsInLine);
				xLineSize -= textBlockSize;
				uint charsInNextLine = (charIdx - textBlockIdx) + 1;
				lineInfo[lines-1].size = xLineSize;
				lineInfo[lines-1].chars = charsInLine - charsInNextLine;
				maxXLineSize = std::max(xLineSize, maxXLineSize);
				xLineSize = textBlockSize;
				charsInLine = charsInNextLine;
				//logMsg("break @ char %d with line starting @ %d, %d chars moved to next line, leaving %d", textBlockIdx, currLineIdx, charsInNextLine, lineInfo[lines-1].chars);
			}

			if(wentToNextLine)
			{
				textBlockIdx = currLineIdx = charIdx+1;
				textBlockSize = 0;
				lines++;
			}
		}
#endif
		charIdx++;
		prevC = c;
	}
	chars = charIdx;
	if(lines > 1) // Add info of last line (1 line case doesn't use per-line info)
	{
		lineInfo[lines-1].size = xLineSize;
		lineInfo[lines-1].chars = charsInLine;
	}
	maxXLineSize = std::max(xLineSize, maxXLineSize);
	xSize = maxXLineSize;
	ySize = Gfx::alignYToPixel(nominalHeight * (GC)lines);
}

void Text::draw(GC xPos, GC yPos, _2DOrigin o, _2DOrigin align) const
{
	using namespace Gfx;
	assert(face != NULL && str != NULL);

	resetTransforms();
	setBlendMode(BLEND_MODE_INTENSITY);
	Sprite spr;
	spr.init(0, 0, 1, 1);

	xPos = o.adjustX(xPos, xSize, LT2DO);
	//logMsg("aligned to %f, converted to %d", Gfx::alignYToPixel(yPos), toIYPos(Gfx::alignYToPixel(yPos)));
	yPos = Gfx::alignYToPixel(o.adjustY(yPos, ySize, LT2DO));
	yPos -= nominalHeight - yLineStart;
	GC xOrig = xPos;
	
	//logMsg("drawing text @ %f,%f: str", xPos, yPos, str);
	auto xViewLimit = proj.wHalf();
	const char *s = str;
	uint totalCharsDrawn = 0;
	if(lines > 1)
	{
		assert(lineInfo);
	}
	iterateTimes(lines, l)
	{
		// Get line info (1 line case doesn't use per-line info)
		GC xLineSize = lines > 1 ? lineInfo[l].size : xSize;
		uint charsToDraw = lines > 1 ? lineInfo[l].chars : chars;
		xPos = alignXToPixel(LT2DO.adjustX(xOrig, xSize-xLineSize, align));
		//logMsg("line %d, %d chars", l, charsToDraw);
		iterateTimes(charsToDraw, i)
		{
			uint c;
			auto res = string_convertCharCode(&s, c);
			if(res != OK)
			{
				logWarn("failed char conversion while drawing line %d, char %d, result %d", l, i, res);
				spr.setImg(nullptr);
				return;
			}

			if(c == '\n')
			{
				continue;
			}

			GlyphEntry *gly = face->glyphEntry(c);
			if(!gly)
			{
				//logMsg("no glyph for %X", c);
				xPos += spaceSize;
				continue;
			}
			if(xPos >= xViewLimit)
			{
				//logMsg("skipped %c, off right screen edge", s[i]);
				continue;
			}
			GC xSize = iXSize(gly->metrics.xSize);

			spr.setImg(&gly->glyph);
			auto x = xPos + iXSize(gly->metrics.xOffset);
			auto y = yPos - iYSize(gly->metrics.ySize - gly->metrics.yOffset);
			spr.setPos(x, y, x + xSize, y + iYSize(gly->metrics.ySize));
			//logMsg("drawing");
			spr.draw();
			xPos += Gfx::iXSize(gly->metrics.xAdvance);
		}
		yPos -= nominalHeight;
		totalCharsDrawn += charsToDraw;
	}
	assert(totalCharsDrawn <= chars);
	spr.setImg(nullptr);
}

}
