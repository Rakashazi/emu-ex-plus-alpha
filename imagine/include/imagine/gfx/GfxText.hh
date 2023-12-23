#pragma once

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

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/string/utf16.hh>
#include <limits>
#include <concepts>

namespace IG::Gfx
{

class GlyphTextureSet;

enum class TextAlignment
{
	left, center, right
};

struct TextLayoutConfig
{
	static constexpr auto noMaxLines = std::numeric_limits<int>::max();
	static constexpr auto noMaxLineSize = std::numeric_limits<int>::max();

	int maxLineSize = noMaxLineSize;
	int maxLines = noMaxLines;
	TextAlignment alignment{};
};

class Text
{
public:
	Text() = default;
	Text(RendererTask &task, GlyphTextureSet *face): Text{task, UTF16String{}, face} {}
	Text(RendererTask &task, UTF16Convertible auto &&str, GlyphTextureSet *face = nullptr):
		textStr{IG_forward(str)}, face_{face}, quads{task, {.size = 1}} {}

	void resetString(UTF16Convertible auto &&str)
	{
		textStr = IG_forward(str);
		sizeBeforeLineSpans = {};
	}

	void resetString() { resetString(UTF16String{}); }
	void setFace(GlyphTextureSet *face) { face_ = face; }
	GlyphTextureSet *face() const { return face_; }
	void makeGlyphs();
	bool compile(TextLayoutConfig conf = {});
	void draw(RendererCommands &, WPt pos, _2DOrigin, Color) const;
	void draw(RendererCommands &, WPt pos, _2DOrigin) const;
	WSize pixelSize() const { return {xSize, ySize}; }
	int width() const { return xSize; }
	int height() const { return ySize; }
	auto nominalHeight() const { return metrics.nominalHeight; }
	int fullHeight() const { return ySize + (nominalHeight() / 2); }
	auto spaceWidth() const { return metrics.spaceSize; }
	uint16_t currentLines() const;
	size_t stringSize() const;
	bool isVisible() const;
	std::u16string_view stringView() const;
	std::u16string string() const;
	Renderer &renderer();

protected:
	struct LineSpan
	{
		int xSize;
		uint16_t chars;
		static constexpr size_t encodedChar16Size = (sizeof(xSize) / 2) + (sizeof(chars) / 2);

		constexpr LineSpan(int xSize, uint16_t chars):
			xSize{xSize}, chars{chars} {}
		void encodeTo(std::u16string &);
		static LineSpan decode(std::u16string_view);
	};

	UTF16String textStr;
	GlyphTextureSet *face_{};
	size_t sizeBeforeLineSpans{}; // encoded LineSpans in textStr start after this offset
	int xSize{};
	int ySize{};
	GlyphSetMetrics metrics;
	ITexQuads quads;

	bool hasText() const;
};

}
