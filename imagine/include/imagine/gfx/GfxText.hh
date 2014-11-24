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

#include <imagine/engine-globals.h>
#include <imagine/gfx/Gfx.hh>

#if defined(CONFIG_RESOURCE_FACE)

#include <float.h>
#include "GfxSprite.hh"
#include <imagine/util/2DOrigin.h>
#include <imagine/util/basicString.h>
#include <imagine/resource/face/ResourceFace.hh>

namespace Gfx
{

class Text
{
public:
	static constexpr ushort NO_MAX_LINES = 0-1;
	static constexpr GC NO_MAX_LINE_SIZE = FLT_MAX;

	struct LineInfo
	{
		GC size;
		uint chars;
	};

	ResourceFace *face{};
	GC spaceSize = 0;
	GC nominalHeight = 0;
	GC yLineStart = 0;
	GC xSize = 0, ySize = 0;
	GC maxLineSize = NO_MAX_LINE_SIZE;
	const char *str{};
	uint chars = 0;
	ushort lines = 0;
	ushort maxLines = NO_MAX_LINES;
	LineInfo *lineInfo{};

	constexpr Text() {}
	constexpr Text(const char *str): str(str) {}

	void init(const char *str)
	{
		setString(str);
	}

	void init(const char *str, ResourceFace *face)
	{
		setString(str);
		setFace(face);
	}

	void init(ResourceFace *face)
	{
		setFace(face);
	}

	void initCompiled(const char *str, ResourceFace *face, const ProjectionPlane &projP)
	{
		init(str, face);
		compile(projP);
	}
	void deinit();
	void setString(const char *str);
	void setFace(ResourceFace *face);
	void compile(const ProjectionPlane &projP);

	void draw(GC xPos, GC yPos, _2DOrigin o, const ProjectionPlane &projP) const;
	void draw(GP p, _2DOrigin o, const ProjectionPlane &projP) const
	{
		draw(p.x, p.y, o, projP);
	}
};

}

#endif
