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
#include <vector>
#include <limits>

namespace Gfx
{

class Renderer;
class RendererCommands;
class GlyphTextureSet;
class TexVertex;
class ProjectionPlane;

class Text
{
public:
	static constexpr uint16_t NO_MAX_LINES = std::numeric_limits<uint16_t>::max();
	static constexpr GC NO_MAX_LINE_SIZE = std::numeric_limits<GC>::max();

	Text();
	Text(GlyphTextureSet *face);
	Text(const char *str, GlyphTextureSet *face = nullptr);
	Text(TextString str, GlyphTextureSet *face = nullptr);
	void setString(const char *str);
	void setString(const Text &o);
	void setString(TextString v);
	void setFace(GlyphTextureSet *face);
	void makeGlyphs(Renderer &r);
	bool compile(Renderer &r, ProjectionPlane projP);
	void draw(RendererCommands &cmds, GC xPos, GC yPos, _2DOrigin o, ProjectionPlane projP) const;
	void draw(RendererCommands &cmds, GP p, _2DOrigin o, ProjectionPlane projP) const;
	void setMaxLineSize(GC size);
	void setMaxLines(uint16_t lines);
	GC width() const;
	GC height() const;
	GC fullHeight() const;
	GC nominalHeight() const;
	GC spaceWidth() const;
	GlyphTextureSet *face() const;
	uint16_t currentLines() const;
	unsigned stringSize() const;
	bool isVisible() const;
	TextStringView stringView() const;

protected:
	struct LineSpan
	{
		constexpr LineSpan(GC size, uint32_t chars):
			size{size}, chars{chars}
		{}
		GC size;
		uint32_t chars;
	};

	TextString textStr{};
	GlyphTextureSet *face_{};
	std::vector<LineSpan> lineInfo{};
	GC spaceSize = 0;
	GC nominalHeight_ = 0;
	GC yLineStart = 0;
	GC xSize = 0;
	GC ySize = 0;
	GC maxLineSize = NO_MAX_LINE_SIZE;
	uint16_t lines = 0;
	uint16_t maxLines = NO_MAX_LINES;

	void drawSpan(RendererCommands &cmds, GC xPos, GC yPos, ProjectionPlane projP, TextStringView strView, std::array<TexVertex, 4> &vArr) const;
	bool hasText() const;
};

}
