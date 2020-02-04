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
#include <float.h>
#include <imagine/util/2DOrigin.h>
#include <imagine/util/string.h>

namespace Gfx
{

class Renderer;
class RendererCommands;
class GlyphTextureSet;
class ProjectionPlane;

class Text
{
public:
	static constexpr uint16_t NO_MAX_LINES = 0-1;
	static constexpr GC NO_MAX_LINE_SIZE = FLT_MAX;

	struct LineInfo
	{
		GC size;
		uint32_t chars;
	};

	GlyphTextureSet *face{};
	GC spaceSize = 0;
	GC nominalHeight = 0;
	GC yLineStart = 0;
	GC xSize = 0, ySize = 0;
	GC maxLineSize = NO_MAX_LINE_SIZE;
	const char *str{};
	uint32_t chars = 0;
	uint16_t lines = 0;
	uint16_t maxLines = NO_MAX_LINES;
	LineInfo *lineInfo{};

	constexpr Text() {}
	constexpr Text(const char *str): str{str} {}
	constexpr Text(const char *str, GlyphTextureSet *face): face{face}, str{str} {}
	~Text();
	void setString(const char *str);
	void setFace(GlyphTextureSet *face);
	void makeGlyphs(Renderer &r);
	void compile(Renderer &r, const ProjectionPlane &projP);
	void draw(RendererCommands &cmds, GC xPos, GC yPos, _2DOrigin o, const ProjectionPlane &projP) const;
	void draw(RendererCommands &cmds, GP p, _2DOrigin o, const ProjectionPlane &projP) const
	{
		draw(cmds, p.x, p.y, o, projP);
	}
	GC fullHeight() const;
};

}
