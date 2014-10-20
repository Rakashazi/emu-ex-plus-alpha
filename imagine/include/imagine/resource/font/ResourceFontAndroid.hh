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
#include <imagine/resource/font/ResourceFont.h>
#include <jni.h>

class ResourceFontAndroid : public ResourceFont
{
public:
	constexpr ResourceFontAndroid() {}
	static ResourceFont *loadSystem();
	void free() override;
	IG::Pixmap charBitmap() override;
	CallResult activeChar(int idx, GlyphMetrics &metrics) override;
	void unlockCharBitmap(IG::Pixmap &pix) override;
	//int currentFaceDescender() const override;
	//int currentFaceAscender() const override;
	CallResult newSize(const FontSettings &settings, FontSizeRef &sizeRef) override;
	CallResult applySize(FontSizeRef &sizeRef) override;
	void freeSize(FontSizeRef &sizeRef) override;

private:
	jobject renderer = nullptr, lockedBitmap = nullptr;
};
