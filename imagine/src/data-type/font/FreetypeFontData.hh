#pragma once

#include <engine-globals.h>
#include <io/Io.hh>
#include <data-type/font/FontData.hh>

#include <ft2build.h>
#include FT_FREETYPE_H

class FreetypeFontData
{
public:
	constexpr FreetypeFontData() { }
	CallResult open(Io *file);
	void close(bool closeIo);
	void accessCharBitmap(void *&bitmap, int &x, int &y, int &pitch) const;
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

	CallResult setSizes(int x, int y);
};
