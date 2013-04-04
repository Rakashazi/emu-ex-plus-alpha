#pragma once

#include <gfx/Gfx.hh>

#if defined(CONFIG_RESOURCE_FACE)

#include <float.h>
#include "GfxSprite.hh"
#include <util/2DOrigin.h>
#include <util/basicString.h>
#include <resource2/face/ResourceFace.hh>

#ifdef CONFIG_BASE_PS3
	#undef FLT_MAX
	#define FLT_MAX 1E+37
#endif

namespace Gfx
{

class Text
{
public:
	constexpr Text() { }
	constexpr Text(const char *str): str(str) { }

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

	void initCompiled(const char *str, ResourceFace *face)
	{
		init(str, face);
		compile();
	}
	void deinit();
	void setString(const char *str);
	void setFace(ResourceFace *face);
	void compile();
	void draw(GC xPos, GC yPos, _2DOrigin o, _2DOrigin align) const;
	void draw(GC xPos, GC yPos, _2DOrigin o) const
	{
		draw(xPos, yPos, o, LT2DO);
	}

	static constexpr ushort NO_MAX_LINES = 0-1;
	static constexpr GC NO_MAX_LINE_SIZE = FLT_MAX;

	struct LineInfo
	{
		GC size;
		uint chars;
	};

	ResourceFace *face = nullptr;
	GC spaceSize = 0;
	GC nominalHeight = 0;
	GC yLineStart = 0;
	GC xSize = 0, ySize = 0;
	GC maxLineSize = NO_MAX_LINE_SIZE;
	const char *str = nullptr;
	uint chars = 0;
	ushort lines = 0;
	ushort maxLines = NO_MAX_LINES;
	LineInfo *lineInfo = nullptr;
};

}

#endif
