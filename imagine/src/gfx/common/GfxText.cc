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
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/util/math.hh>
#include <imagine/util/ctype.hh>
#include <imagine/logger/logger.h>
#include <algorithm>
#include <bit>

namespace IG::Gfx
{

constexpr SystemLogger log{"GfxText"};

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

void Text::makeGlyphs()
{
	if(!hasText()) [[unlikely]]
		return;
	for(auto c : textStr)
	{
		face_->glyphEntry(renderer(), c);
	}
}

auto writeSpan(Renderer &r, auto quadsIt, WPt pos, std::u16string_view strView, GlyphTextureSet *face_, int spaceSize)
{
	for(auto c : strView)
	{
		if(c == '\n')
		{
			continue;
		}
		auto gly = face_->glyphEntry(r, c, false);
		if(!gly)
		{
			//log.info("no glyph for:{:X}", c);
			pos.x += spaceSize;
			continue;
		}
		auto &[glyph, metrics] = *gly;
		auto drawPos = pos.as<int16_t>() + metrics.offset.negateY();
		pos.x += metrics.xAdvance;
		ITexQuad quad
		{
			{.bounds = {drawPos, (drawPos + metrics.size)}, .textureBounds = ITexQuad::unitTexCoordRect()}
		};
		quadsIt = std::ranges::copy(quad.v, quadsIt).out;
	}
	return quadsIt;
}

bool Text::compile(TextLayoutConfig conf)
{
	if(!hasText()) [[unlikely]]
	{
		if(!face_)
			log.warn("called compile() before setting face");
		return false;
	}
	assert(quads.hasTask());
	auto &r = renderer();
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
		if(isGraph(c) && isSpace(prevC))
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
				//log.info("new line {} without text break @ char {}, {} chars in line", lines+1, charIdx, charsInLine);
				lineInfo.emplace_back(xLineSize, charsInLine);
				maxXLineSize = std::max(xLineSize, maxXLineSize);
				xLineSize = 0;
				charsInLine = 0;
			}
			else if(lineExceedsMaxSize && lineHasMultipleBlocks)
			{
				wentToNextLine = true;
				// Line has more than 1 block and is too big, needs text break
				//log.info("new line {} with text break @ char {}, {} chars in line", lines+1, charIdx, charsInLine);
				xLineSize -= textBlockSize;
				int charsInNextLine = (charIdx - textBlockIdx) + 1;
				lineInfo.emplace_back(xLineSize, charsInLine - charsInNextLine);
				maxXLineSize = std::max(xLineSize, maxXLineSize);
				xLineSize = textBlockSize;
				charsInLine = charsInNextLine;
				//log.info("break @ char {} with line starting @ {}, {} chars moved to next line, leaving {}", textBlockIdx, currLineIdx, charsInNextLine, lineInfo[lines-1].chars);
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

	// write vertex data
	WPt pos{0, nominalHeight - yLineStart};
	quads.reset({.size = size_t(charIdx)});
	auto mappedVerts = quads.map();
	if(lines > 1)
	{
		auto s = textStr.data();
		auto spansPtr = &textStr[sizeBeforeLineSpans];
		auto vertsIt = mappedVerts.begin();
		auto startingXPos = [&](auto xLineSize)
		{
			switch(conf.alignment)
			{
				case TextAlignment::left: return 0;
				case TextAlignment::center: return (xSize - xLineSize) / 2;
				case TextAlignment::right: return xSize - xLineSize;
			}
			std::unreachable();
		};
		for([[maybe_unused]] auto i : iotaCount(lines))
		{
			auto [xLineSize, charsToDraw] = LineSpan::decode({spansPtr, LineSpan::encodedChar16Size});
			spansPtr += LineSpan::encodedChar16Size;
			pos.x = startingXPos(xLineSize);
			//log.info("line:{} chars:{} ", i, charsToDraw);
			vertsIt = writeSpan(r, vertsIt, pos, std::u16string_view{s, charsToDraw}, face_, spaceSize);
			s += charsToDraw;
			pos.y += nominalHeight;
		}
	}
	else
	{
		writeSpan(r, mappedVerts.begin(), pos, std::u16string_view{textStr}, face_, spaceSize);
	}
	return true;
}

static int drawSpan(RendererCommands &cmds, std::u16string_view strView, int spriteOffset, GlyphTextureSet *face_)
{
	auto &renderer = cmds.renderer();
	auto &basicEffect = cmds.basicEffect();
	for(auto c : strView)
	{
		if(c == '\n')
		{
			continue;
		}
		auto gly = face_->glyphEntry(renderer, c, false);
		if(!gly)
		{
			//log.info("no glyph for {:X}", c);
			continue;
		}
		auto &[glyph, metrics] = *gly;
		basicEffect.drawSprite(cmds, spriteOffset++, glyph);
	}
	return spriteOffset;
}

void Text::draw(RendererCommands &cmds, WPt pos, _2DOrigin o, Color c) const
{
	cmds.setColor(c);
	draw(cmds, pos, o);
}

void Text::draw(RendererCommands &cmds, WPt pos, _2DOrigin o) const
{
	if(!hasText()) [[unlikely]]
		return;
	cmds.set(BlendMode::ALPHA);
	pos.x = o.adjustX(pos.x, xSize, LT2DO);
	if(o.onBottom())
		pos.y -= ySize;
	else if(o.onYCenter())
		pos.y -= ySize / 2;
	//log.info("drawing text @ {},{}, size:{},{}", xPos, yPos, xSize, ySize);
	cmds.basicEffect().setModelView(cmds, Mat4::makeTranslate({pos.x, pos.y, 0}));
	cmds.setVertexArray(quads);
	auto lines = currentLines();
	if(lines > 1)
	{
		auto s = textStr.data();
		auto spansPtr = &textStr[sizeBeforeLineSpans];
		int spriteOffset = 0;
		for([[maybe_unused]] auto i : iotaCount(lines))
		{
			auto [xLineSize, charsToDraw] = LineSpan::decode({spansPtr, LineSpan::encodedChar16Size});
			spansPtr += LineSpan::encodedChar16Size;
			//log.info("line:{} chars:{}", i, charsToDraw);
			spriteOffset = drawSpan(cmds, std::u16string_view{s, charsToDraw}, spriteOffset, face_);
			s += charsToDraw;
		}
	}
	else
	{
		drawSpan(cmds, std::u16string_view{textStr}, 0, face_);
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

Renderer &Text::renderer() { return quads.renderer(); }

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
