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

#include <imagine/logger/logger.h>
#include <imagine/gfx/GfxText.hh>
#include <imagine/util/math/int.hh>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace Gfx
{

Text::~Text()
{
	if(lineInfo)
	{
		std::free(lineInfo);
	}
}

void Text::setString(const char *str)
{
	assert(str);
	this->str = str;
}

void Text::setFace(GlyphTextureSet *face)
{
	assert(face);
	this->face = face;
}

static GC xSizeOfChar(Renderer &r, GlyphTextureSet *face, int c, GC spaceX, const ProjectionPlane &projP)
{
	assert(c != '\0');
	if(c == ' ')
		return spaceX;
	if(c == '\n')
		return 0;

	GlyphEntry *gly = face->glyphEntry(r, c);
	if(gly != NULL)
		return projP.unprojectXSize(gly->metrics.xAdvance);
	else
		return 0;
}

void Text::makeGlyphs(Renderer &r)
{
	if(unlikely(!face || !str))
		return;
	const char *s = str;
	uint32_t c = 0;
	while(!(bool)string_convertCharCode(&s, c))
	{
		face->glyphEntry(r, c);
	}
}

void Text::compile(Renderer &r, const ProjectionPlane &projP)
{
	assert(face);
	assert(str);
	r.makeCommonTextureSampler(CommonTextureSampler::NO_MIP_CLAMP);
	//logMsg("compiling text %s", str);
	
	// TODO: move calc into Face class
	GlyphEntry *mGly = face->glyphEntry(r, 'M');
	GlyphEntry *gGly = face->glyphEntry(r, 'g');
	
	if(!mGly || !gGly)
	{
		logErr("error reading measurement glyphs to compile text");
		return;
	}

	yLineStart = projP.alignYToPixel(projP.unprojectYSize(gGly->metrics.ySize - gGly->metrics.yOffset));
	
	int spaceSizeI = mGly->metrics.xSize/2;
	spaceSize = projP.unprojectXSize(spaceSizeI);
	uint32_t nominalHeightPixels = mGly->metrics.ySize + gGly->metrics.ySize/2;
	nominalHeight = projP.alignYToPixel(projP.unprojectYSize(IG::makeEvenRoundedUp(nominalHeightPixels)));
	//int maxLineSizeI = Gfx::toIXSize(maxLineSize);
	//logMsg("max line size %f", maxLineSize);
	
	lines = 1;
	GC xLineSize = 0, maxXLineSize = 0;
	const char *s = str;
	uint32_t c = 0, prevC = 0;
	GC textBlockSize = 0;
	uint32_t textBlockIdx = 0, currLineIdx = 0;
	uint32_t charIdx = 0, charsInLine = 0;
	while(!(bool)string_convertCharCode(&s, c))
	{
		auto cSize = xSizeOfChar(r, face, c, spaceSize, projP);
		charsInLine++;

		// Is this the start of a text block?
		if(isgraph(c) && isspace(prevC))
		{
			textBlockIdx = charIdx;
			textBlockSize = 0;
		}
		xLineSize += cSize;
		textBlockSize += cSize;
		if(lines < maxLines)
		{
			bool wentToNextLine = 0;
			// Go to next line?
			if(c == '\n')
			{
				wentToNextLine = 1;
				lineInfo = (LineInfo*)std::realloc(lineInfo, sizeof(LineInfo)*(lines+1));
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
				lineInfo = (LineInfo*)std::realloc(lineInfo, sizeof(LineInfo)*(lines+1));
				assert(lineInfo);
				// Line has more than 1 block and is too big, needs text break
				//logMsg("new line %d with text break @ char %d, %d chars in line", lines+1, charIdx, charsInLine);
				xLineSize -= textBlockSize;
				uint32_t charsInNextLine = (charIdx - textBlockIdx) + 1;
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
	ySize = nominalHeight * (GC)lines;
}

void Text::draw(RendererCommands &cmds, GC xPos, GC yPos, _2DOrigin o, const ProjectionPlane &projP) const
{
	using namespace Gfx;
	assert(face && str);
	//o = LT2DO;
	//logMsg("drawing with origin: %s,%s", o.toString(o.x), o.toString(o.y));
	cmds.setBlendMode(BLEND_MODE_ALPHA);
	cmds.setCommonTextureSampler(CommonTextureSampler::NO_MIP_CLAMP);
	std::array<TexVertex, 4> vArr;
	cmds.bindTempVertexBuffer();
	TexVertex::bindAttribs(cmds, vArr.data());
	_2DOrigin align = o;
	xPos = o.adjustX(xPos, xSize, LT2DO);
	//logMsg("aligned to %f, converted to %d", Gfx::alignYToPixel(yPos), toIYPos(Gfx::alignYToPixel(yPos)));
	yPos = o.adjustY(yPos, projP.alignYToPixel(ySize/2_gc), ySize, LT2DO);
	if(IG::isOdd(projP.viewport.height()))
		yPos = projP.alignYToPixel(yPos);
	yPos -= nominalHeight - yLineStart;
	GC xOrig = xPos;
	
	//logMsg("drawing text @ %f,%f: str", xPos, yPos, str);
	auto xViewLimit = projP.wHalf();
	const char *s = str;
	uint32_t totalCharsDrawn = 0;
	if(lines > 1)
	{
		assert(lineInfo);
	}
	iterateTimes(lines, l)
	{
		// Get line info (1 line case doesn't use per-line info)
		GC xLineSize = lines > 1 ? lineInfo[l].size : xSize;
		uint32_t charsToDraw = lines > 1 ? lineInfo[l].chars : chars;
		xPos = projP.alignXToPixel(LT2DO.adjustX(xOrig, xSize-xLineSize, align));
		//logMsg("line %d, %d chars", l, charsToDraw);
		iterateTimes(charsToDraw, i)
		{
			uint32_t c;
			if(auto err = string_convertCharCode(&s, c);
				(bool)err)
			{
				logWarn("failed char conversion while drawing line %d, char %d, result %d", l, i, (int)err);
				return;
			}

			if(c == '\n')
			{
				continue;
			}

			GlyphEntry *gly = face->glyphEntry(cmds.renderer(), c, false);
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
			GC xSize = projP.unprojectXSize(gly->metrics.xSize);

			auto x = xPos + projP.unprojectXSize(gly->metrics.xOffset);
			auto y = yPos - projP.unprojectYSize(gly->metrics.ySize - gly->metrics.yOffset);
			auto &glyph = gly->glyph();
			vArr = makeTexVertArray({x, y, x + xSize, y + projP.unprojectYSize(gly->metrics.ySize)}, glyph);
			cmds.vertexBufferData(vArr.data(), sizeof(vArr));
			cmds.setTexture(glyph);
			//logMsg("drawing");
			cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
			xPos += projP.unprojectXSize(gly->metrics.xAdvance);
		}
		yPos -= nominalHeight;
		yPos = projP.alignYToPixel(yPos);
		totalCharsDrawn += charsToDraw;
	}
	if(totalCharsDrawn < chars)
	{
		logWarn("only rendered %d/%d chars", totalCharsDrawn, chars);
	}
}

GC Text::fullHeight() const
{
	using namespace Gfx;
	return ySize + (nominalHeight * .5_gc);
}

}
