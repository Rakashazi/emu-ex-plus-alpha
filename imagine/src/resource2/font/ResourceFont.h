#pragma once
#include <engine-globals.h>
#include <resource2/font/common/glyphTable.h>
#include <data-type/font/FontData.hh>
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

class ResourceFace;

class ResourceFont
{
public:
	virtual ~ResourceFont() { }

	CallResult initWithName(const char * name);
	//ResourceImageGlyph *createRenderable(int c, ResourceFace *face, GlyphEntry *entry);
	int minUsablePixels() const;

	virtual void free() = 0;
	virtual void charBitmap(void* &bitmap, int &x, int &y, int &pitch) = 0;
	virtual void unlockCharBitmap(void *data) { }
	virtual CallResult activeChar(int idx, GlyphMetrics &metrics) = 0;
	//virtual int currentFaceDescender() const = 0;
	//virtual int currentFaceAscender() const = 0;
	virtual CallResult newSize(const FontSettings &settings, FontSizeRef &sizeRef) = 0;
	virtual CallResult applySize(FontSizeRef &sizeData) = 0;
	virtual void freeSize(FontSizeRef &sizeData) = 0;
};
