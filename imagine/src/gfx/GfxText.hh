#pragma once

#include <gfx/Gfx.hh>

#if defined(CONFIG_RESOURCE_FACE)

#include "GfxSprite.hh"
#include <util/2DOrigin.h>
#include <resource2/face/ResourceFace.hh>

extern GfxSprite gfxText_spr;

class GfxText
{
public:
	constexpr GfxText(): face(0), spaceSize(0), nominalHeight(0), yLineStart(0), xSize(0), ySize(0),
	maxLineSize(0), str(0), slen(0), maxLines(0) { }

	void init();
	void init(const char *str)
	{
		init();
		setString(str);
	}

	void init(const char *str, ResourceFace *face)
	{
		init();
		setString(str);
		setFace(face);
	}

	void init(ResourceFace *face)
	{
		init();
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

	ResourceFace *face;
	GC spaceSize;
	GC nominalHeight;
	GC yLineStart;
	GC xSize, ySize;
	GC maxLineSize;
	const char *str;
	uint slen;
	ushort maxLines;
};

#endif
