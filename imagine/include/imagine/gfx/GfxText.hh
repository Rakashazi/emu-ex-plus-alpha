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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/string/utf16.hh>
#include <vector>
#include <limits>

namespace IG::Gfx
{

class Renderer;
class RendererCommands;
class GlyphTextureSet;
struct TexVertex;
class ProjectionPlane;

class Text
{
public:
	static constexpr uint16_t NO_MAX_LINES = std::numeric_limits<uint16_t>::max();
	static constexpr float NO_MAX_LINE_SIZE = std::numeric_limits<float>::max();

	Text() = default;
	Text(GlyphTextureSet *face);
	Text(IG::utf16String str, GlyphTextureSet *face = nullptr);
	void setString(IG::utf16String);
	void setFace(GlyphTextureSet *face);
	void makeGlyphs(Renderer &r);
	bool compile(Renderer &r, ProjectionPlane projP);
	void draw(RendererCommands &cmds, float xPos, float yPos, _2DOrigin o, ProjectionPlane projP) const;
	void draw(RendererCommands &cmds, GP p, _2DOrigin o, ProjectionPlane projP) const;
	void setMaxLineSize(float size);
	void setMaxLines(uint16_t lines);
	float width() const;
	float height() const;
	float fullHeight() const;
	float nominalHeight() const;
	float spaceWidth() const;
	GlyphTextureSet *face() const;
	uint16_t currentLines() const;
	unsigned stringSize() const;
	bool isVisible() const;
	std::u16string_view stringView() const;
	std::u16string string() const;

protected:
	struct LineSpan
	{
		constexpr LineSpan(float size, uint32_t chars):
			size{size}, chars{chars}
		{}
		float size;
		uint32_t chars;
	};

	std::u16string textStr{};
	GlyphTextureSet *face_{};
	std::vector<LineSpan> lineInfo{};
	float spaceSize = 0;
	float nominalHeight_ = 0;
	float yLineStart = 0;
	float xSize = 0;
	float ySize = 0;
	float maxLineSize = NO_MAX_LINE_SIZE;
	uint16_t lines = 0;
	uint16_t maxLines = NO_MAX_LINES;

	void drawSpan(RendererCommands &cmds, float xPos, float yPos, ProjectionPlane projP, std::u16string_view strView, std::array<TexVertex, 4> &vArr) const;
	bool hasText() const;
};

}
