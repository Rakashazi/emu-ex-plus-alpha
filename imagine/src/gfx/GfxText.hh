#pragma once

#include <gfx/Gfx.hh>

#if defined(CONFIG_RESOURCE_FACE)

#include "GfxSprite.hh"
#include <util/2DOrigin.h>
#include <util/basicString.h>
#include <resource2/face/ResourceFace.hh>

class GfxText
{
public:
	constexpr GfxText() { }
	constexpr GfxText(const char *str): str(str), slen(string_len(str)) { }
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

	ResourceFace *face = nullptr;
	GC spaceSize = 0;
	GC nominalHeight = 0;
	GC yLineStart = 0;
	GC xSize = 0, ySize = 0;
	GC maxLineSize = 0;
	const char *str = nullptr;
	uint slen = 0;
	ushort maxLines = 0;
};

#endif
