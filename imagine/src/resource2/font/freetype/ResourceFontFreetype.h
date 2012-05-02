#pragma once

#include <engine-globals.h>
#include <data-type/font/freetype-2/reader.h>
#include <resource2/font/ResourceFont.h>
#include <io/sys.hh>

class ResourceFontFreetype : public ResourceFont
{
public:
	static ResourceFont * load(Io* io);
	static ResourceFont * load(const char * name);
	static ResourceFont * loadAsset(const char * name)
	{
		return load(openAppAssetIo(name));
	}

	void free();
	int currentCharYOffset() const;
	int currentCharXOffset() const;
	int currentCharXAdvance() const;
	void charBitmap(uchar** bitmap, int* x, int* y, int* pitch) const;
	CallResult activeChar(int idx);
	int currentCharXSize() const;
	int currentCharYSize() const;
	int currentFaceDescender() const;
	int currentFaceAscender() const;
	int minUsablePixels() const;
	ResourceImageGlyph *createRenderable(int c, ResourceFace *face, GlyphEntry *entry);
	CallResult newSize(FontSettings* settings, FaceSizeData* sizeDataAddr);
	CallResult applySize(FaceSizeData sizeData);
	void freeSize(FaceSizeData sizeData);

private:
	Io* io;
	FontData f;
	static ResourceFont *loadWithIoWithName (Io* io, const char *name);
};

#define pResourceFontFreetype_LoadWithEmbedFile(name) PP_resourceLoadWithEmbedFile(name, ResourceFontFreetype::load)
