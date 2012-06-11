#pragma once
#include <engine-globals.h>
#include <resource2/font/common/glyphTable.h>
#include <resource2/Resource.h>
#include <assert.h>

class FontSettings
{
public:
	constexpr FontSettings() { }
	constexpr FontSettings(int pixelWidth, int pixelHeight) : pixelWidth(pixelWidth), pixelHeight(pixelHeight) { }
	constexpr FontSettings(int pixelHeight) : FontSettings(0, pixelHeight) { }

	int areValid()
	{
		return pixelHeight != 0 || pixelWidth != 0;
	}

	void process()
	{
		assert(areValid());
		if(pixelHeight == 0)
			pixelHeight = pixelWidth;
		if(pixelWidth == 0)
			pixelWidth = pixelHeight;
	}

	bool operator==(const FontSettings& other) const
	{
		if(pixelHeight == other.pixelHeight
			&& pixelWidth == other.pixelWidth)
			return 1;
		else
			return 0;
	}

	bool operator!=(const FontSettings& other) const
	{
		return !(*this == other);
	}

	int pixelWidth = 0, pixelHeight = 0;
};

#ifdef CONFIG_RESOURCE_FONT_FREETYPE
	#include <data-type/font/freetype-2/reader.h>
	typedef FT_Size FaceSizeData;
#endif

class ResourceFace;
#include <resource2/image/glyph/ResourceImageGlyph.h>

class ResourceFont
{
public:
	virtual ~ResourceFont() { }

	CallResult initWithName(const char * name);

	virtual void free() = 0;
	virtual int currentCharYOffset() const = 0;
	virtual int currentCharXOffset() const = 0;
	virtual int currentCharXAdvance() const = 0;
	virtual void charBitmap(uchar** bitmap, int* x, int* y, int* pitch) const = 0;
	virtual CallResult activeChar(int idx) = 0;
	virtual int currentCharXSize() const = 0;
	virtual int currentCharYSize() const = 0;
	virtual int currentFaceDescender() const = 0;
	virtual int currentFaceAscender() const = 0;
	virtual int minUsablePixels() const = 0;
	virtual ResourceImageGlyph *createRenderable(int c, ResourceFace *face, GlyphEntry *entry) = 0;
	virtual CallResult newSize(FontSettings* settings, FaceSizeData* sizeDataAddr) = 0;
	virtual CallResult applySize(FaceSizeData sizeData) = 0;
	virtual void freeSize(FaceSizeData sizeData) = 0;
};
