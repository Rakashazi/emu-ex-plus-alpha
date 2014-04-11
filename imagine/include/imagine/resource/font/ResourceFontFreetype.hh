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
#include <imagine/data-type/font/FreetypeFontData.hh>
#include <imagine/resource/font/ResourceFont.h>
#include <imagine/io/Io.hh>

class ResourceFontFreetype : public ResourceFont
{
public:
	ResourceFontFreetype() {}
	static ResourceFontFreetype *load();
	static ResourceFontFreetype *load(Io *io);
	static ResourceFontFreetype *load(const char *name);
	static ResourceFontFreetype *loadAsset(const char *name);
	void free() override;
	IG::Pixmap charBitmap() override;
	CallResult activeChar(int idx, GlyphMetrics &metrics) override;
	//int currentFaceDescender() const override;
	//int currentFaceAscender() const override;
	CallResult newSize(const FontSettings &settings, FontSizeRef &sizeRef) override;
	CallResult applySize(FontSizeRef &sizeRef) override;
	void freeSize(FontSizeRef &sizeRef) override;
	CallResult loadIntoSlot(Io *io, uint slot);
	CallResult loadIntoSlot(const char *name, uint slot);
	CallResult loadIntoNextSlot(const char *name);

private:
	static constexpr uint MAX_FREETYPE_SLOTS = Config::envIsLinux ? 4 : 2;
	struct FontSizeData
	{
		constexpr FontSizeData(FontSettings settings): settings(settings) {}
		FontSettings settings;
		FT_Size size[MAX_FREETYPE_SLOTS] {nullptr};
	};
	FreetypeFontData f[MAX_FREETYPE_SLOTS];
	uint16 currCharSlot = 0;
	uint16 usedCharSlots = 0;
	FontSizeData *activeFontSizeData = nullptr;

	static ResourceFontFreetype *loadWithIoWithName (Io *io, const char *name);
	void setMetrics(const FreetypeFontData &fontData, GlyphMetrics &metrics);
};
