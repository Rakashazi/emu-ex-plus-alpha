#pragma once

#include <engine-globals.h>

class ResourceFace;
#include <resource2/image/ResourceImage.h>
#include <resource2/font/common/glyphTable.h>

class ResourceImageGlyph : public ResourceImage
{
public:
	static ResourceImageGlyph *createWithFace(ResourceFace * face, GlyphEntry * idx);
	void free();
	CallResult getImage(Pixmap* dest);
	int alphaChannelType();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat() const;
	float aspectRatio();
private:
	ResourceFace *face = nullptr;
	GlyphEntry *idx = nullptr;
};
