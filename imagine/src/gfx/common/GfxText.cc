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

#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/util/math/int.hh>
#include <imagine/util/ctype.hh>
#include <imagine/logger/logger.h>
#include <algorithm>
#include <bit>

namespace IG::Gfx
{

static int xSizeOfChar(Renderer &r, GlyphTextureSet *face_, int c, int spaceX)
{
	assert(c != '\0');
	if(c == ' ')
		return spaceX;
	if(c == '\n')
		return 0;
	auto gly = face_->glyphEntry(r, c);
	if(gly)
		return gly->metrics.xAdvance;
	else
		return 0;
}

void Text::makeGlyphs(Renderer &r)
{
	if(!hasText()) [[unlikely]]
		return;
	for(auto c : textStr)
	{
		face_->glyphEntry(r, c);
	}
}

bool Text::compile(Renderer &r, TextLayoutConfig conf)
{
	if(!hasText()) [[unlikely]]
		return false;
	//logMsg("compiling text %s", str);
	if(sizeBeforeLineSpans)
	{
		textStr.resize(stringSize());
		sizeBeforeLineSpans = {};
	}
	metrics = face_->metrics();
	auto [nominalHeight, spaceSize, yLineStart] = metrics;
	int lines = 1;
	std::vector<LineSpan> lineInfo;
	int xLineSize = 0, maxXLineSize = 0;
	int prevC = 0;
	int textBlockSize = 0;
	int textBlockIdx = 0, currLineIdx = 0;
	int charIdx = 0, charsInLine = 0;
	for(auto c : textStr)
	{
		auto cSize = xSizeOfChar(r, face_, c, spaceSize);
		charsInLine++;

		// Is this the start of a text block?
		if(isgraph(c) && isspace(prevC))
		{
			textBlockIdx = charIdx;
			textBlockSize = 0;
		}
		xLineSize += cSize;
		textBlockSize += cSize;
		bool lineHasMultipleBlocks = textBlockIdx != currLineIdx;
		bool lineExceedsMaxSize = xLineSize > conf.maxLineSize;
		if(lines < conf.maxLines)
		{
			bool wentToNextLine = false;
			// Go to next line?
			if(c == '\n' || (lineExceedsMaxSize && !lineHasMultipleBlocks))
			{
				wentToNextLine = true;
				// Don't break text
				//logMsg("new line %d without text break @ char %d, %d chars in line", lines+1, charIdx, charsInLine);
				lineInfo.emplace_back(xLineSize, charsInLine);
				maxXLineSize = std::max(xLineSize, maxXLineSize);
				xLineSize = 0;
				charsInLine = 0;
			}
			else if(lineExceedsMaxSize && lineHasMultipleBlocks)
			{
				wentToNextLine = true;
				// Line has more than 1 block and is too big, needs text break
				//logMsg("new line %d with text break @ char %d, %d chars in line", lines+1, charIdx, charsInLine);
				xLineSize -= textBlockSize;
				int charsInNextLine = (charIdx - textBlockIdx) + 1;
				lineInfo.emplace_back(xLineSize, charsInLine - charsInNextLine);
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
	if(lines > 1) // Encode LineSpan metadata (1 line case doesn't use per-line info)
	{
		sizeBeforeLineSpans = textStr.size();
		for(auto &span : lineInfo)
		{
			LineSpan{span.xSize, span.chars}.encodeTo(textStr);
		}
		LineSpan{xLineSize, (uint16_t)charsInLine}.encodeTo(textStr);
	}
	maxXLineSize = std::max(xLineSize, maxXLineSize);
	xSize = maxXLineSize;
	ySize = nominalHeight * lines;
	return true;
}

static void drawSpan(RendererCommands &cmds, WP pos,
	std::u16string_view strView, TexQuad &vArr, GlyphTextureSet *face_, int spaceSize)
{
	for(auto c : strView)
	{
		if(c == '\n')
		{
			continue;
		}
		auto gly = face_->glyphEntry(cmds.renderer(), c, false);
		if(!gly)
		{
			//logMsg("no glyph for %X", c);
			pos.x += spaceSize;
			continue;
		}
		auto &[glyph, metrics] = *gly;
		auto x = pos.x + metrics.xOffset;
		auto y = pos.y - metrics.yOffset;
		pos.x += metrics.xAdvance;
		vArr = {{{float(x), float(y)}, {float(x + metrics.xSize), float(y + metrics.ySize)}}, glyph};
		cmds.vertexBufferData(vArr.data(), sizeof(vArr));
		cmds.setTexture(glyph);
		cmds.drawPrimitives(Primitive::TRIANGLE_STRIP, 0, 4);
	}
}

void Text::draw(RendererCommands &cmds, WP pos, _2DOrigin o) const
{
	if(!hasText()) [[unlikely]]
		return;
	cmds.set(BlendMode::ALPHA);
	cmds.bindTempVertexBuffer();
	TexQuad vArr;
	cmds.setVertexAttribs(vArr.data());
	pos.x = o.adjustX(pos.x, xSize, LT2DO);
	if(o.onBottom())
		pos.y -= ySize;
	else if(o.onYCenter())
		pos.y -= ySize / 2;
	//logMsg("drawing text @ %d,%d, size:%d,%d", xPos, yPos, xSize, ySize);
	auto [nominalHeight, spaceSize, yLineStart] = metrics;
	pos.y += nominalHeight - yLineStart;
	auto xOrig = pos.x;
	auto startingXPos =
		[&](auto xLineSize)
		{
			return LT2DO.adjustX(xOrig, xSize - xLineSize, o);
		};
	auto lines = currentLines();
	if(lines > 1)
	{
		auto s = textStr.data();
		auto spansPtr = &textStr[sizeBeforeLineSpans];
		for(auto i : iotaCount(lines))
		{
			auto [xLineSize, charsToDraw] = LineSpan::decode({spansPtr, LineSpan::encodedChar16Size});
			spansPtr += LineSpan::encodedChar16Size;
			pos.x = startingXPos(xLineSize);
			//logMsg("line:%d chars:%d ", i, charsToDraw);
			drawSpan(cmds, pos, std::u16string_view{s, charsToDraw}, vArr, face_, spaceSize);
			s += charsToDraw;
			pos.y += nominalHeight;
		}
	}
	else
	{
		auto xLineSize = xSize;
		pos.x = startingXPos(xLineSize);
		drawSpan(cmds, pos, std::u16string_view{textStr}, vArr, face_, spaceSize);
	}
}

uint16_t Text::currentLines() const
{
	if(sizeBeforeLineSpans)
	{
		// count of LineSpans stored at the end of textStr
		return (textStr.size() - sizeBeforeLineSpans) / LineSpan::encodedChar16Size;
	}
	else
	{
		return 1;
	}
}

size_t Text::stringSize() const
{
	if(sizeBeforeLineSpans)
	{
		assumeExpr(sizeBeforeLineSpans < textStr.size());
		return sizeBeforeLineSpans;
	}
	else
	{
		return textStr.size();
	}
}

bool Text::isVisible() const
{
	return stringSize();
}

std::u16string_view Text::stringView() const
{
	return {textStr.data(), stringSize()};
}

std::u16string Text::string() const
{
	return std::u16string{stringView()};
}

bool Text::hasText() const
{
	return face_ && stringSize();
}

void Text::LineSpan::encodeTo(std::u16string &outStr)
{
	auto newSize = outStr.size() + 3;
	outStr.resize_and_overwrite(newSize, [&](char16_t *buf, size_t)
	{
		buf[newSize - 3] = chars;
		buf[newSize - 2] = char16_t(xSize & 0xFFFF);
		buf[newSize - 1] = char16_t(xSize >> 16);
		return newSize;
	});
}

Text::LineSpan Text::LineSpan::decode(std::u16string_view str)
{
	assumeExpr(str.size() >= 3);
	auto xSize = str[1] | (str[2] << 16);
	return {xSize, str[0]};
}

}
