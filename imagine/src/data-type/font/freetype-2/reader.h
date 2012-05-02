#pragma once

#include <engine-globals.h>
#include <io/Io.hh>

#include <ft2build.h>
#include FT_FREETYPE_H

class FontData
{
public:
	CallResult open(Io *file);
	void close();
	//CallResult setSizes(int x, int y);
	void accessCharBitmap(uchar **bitmap, int *x, int *y, int *pitch) const;
	int getCurrentCharBitmapXAdvance() const;
	int getCurrentCharBitmapTop() const;
	int getCurrentCharBitmapLeft() const;
	CallResult setActiveChar(uchar c);
	int charBitmapWidth() const;
	int charBitmapHeight() const;
	int maxDescender() const;
	int maxAscender() const;
	CallResult newSize(int x, int y, FT_Size *sizeDataAddr);
	CallResult applySize(FT_Size sizeData);
	void freeSize(FT_Size sizeData);
private:
	FT_Library library;
	FT_Face face;
	FT_StreamRec streamRec;
	FT_Open_Args openS;

	CallResult setSizes(int x, int y);
};
