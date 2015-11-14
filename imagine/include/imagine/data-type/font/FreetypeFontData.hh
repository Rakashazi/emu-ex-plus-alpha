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

#include <imagine/config/defs.hh>
#include <imagine/io/IO.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/data-type/font/FontData.hh>

#include <ft2build.h>
#include FT_FREETYPE_H

class FreetypeFontData
{
public:
	constexpr FreetypeFontData() {}
	CallResult open(GenericIO file);
	void close(bool closeIo);
	IG::Pixmap accessCharBitmap() const;
	int getCurrentCharBitmapXAdvance() const;
	int getCurrentCharBitmapTop() const;
	int getCurrentCharBitmapLeft() const;
	CallResult setActiveChar(int c);
	int charBitmapWidth() const;
	int charBitmapHeight() const;
	int maxDescender() const;
	int maxAscender() const;
	CallResult newSize(int x, int y, FT_Size *sizeRef);
	CallResult applySize(FT_Size sizeRef);
	void freeSize(FT_Size sizeRef);
	bool isOpen();

private:
	static FT_Library library;
	FT_Face face = nullptr;
	FT_StreamRec streamRec {0};
	FT_Bitmap convBitmap {0};

	CallResult setSizes(int x, int y);
};
