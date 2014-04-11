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
#include <imagine/resource/font/glyphTable.h>
#include <imagine/data-type/font/FontData.hh>
#include <imagine/util/operators.hh>
#include <assert.h>

class FontSettings : public NotEquals<FontSettings>
{
public:
	int pixelWidth = 0, pixelHeight = 0;

	constexpr FontSettings() {}
	constexpr FontSettings(int pixelWidth, int pixelHeight) : pixelWidth(pixelWidth), pixelHeight(pixelHeight) {}
	constexpr FontSettings(int pixelHeight) : FontSettings(0, pixelHeight) {}

	bool areValid() const
	{
		return pixelHeight || pixelWidth;
	}

	void process()
	{
		assert(areValid());
		if(!pixelHeight)
			pixelHeight = pixelWidth;
		if(!pixelWidth)
			pixelWidth = pixelHeight;
	}

	bool operator==(const FontSettings& other) const
	{
		return pixelHeight == other.pixelHeight
			&& pixelWidth == other.pixelWidth;
	}
};

class ResourceFace;

class ResourceFont
{
public:
	constexpr ResourceFont() {}
	virtual ~ResourceFont() {}
	CallResult initWithName(const char * name);
	int minUsablePixels() const;
	virtual void free() = 0;
	virtual IG::Pixmap charBitmap() = 0;
	virtual void unlockCharBitmap(IG::Pixmap &pix) {}
	virtual CallResult activeChar(int idx, GlyphMetrics &metrics) = 0;
	//virtual int currentFaceDescender() const = 0;
	//virtual int currentFaceAscender() const = 0;
	virtual CallResult newSize(const FontSettings &settings, FontSizeRef &sizeRef) = 0;
	virtual CallResult applySize(FontSizeRef &sizeData) = 0;
	virtual void freeSize(FontSizeRef &sizeData) = 0;
};
