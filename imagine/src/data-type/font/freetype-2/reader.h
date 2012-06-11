#pragma once

#include <engine-globals.h>
#include <io/Io.hh>

#include <ft2build.h>
#include FT_FREETYPE_H

class FontData
{
public:
	constexpr FontData() { }
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
	FT_Library library = nullptr;
	FT_Face face = nullptr;
	FT_StreamRec streamRec = {0};
	FT_Open_Args openS = {0};

	CallResult setSizes(int x, int y);
};
