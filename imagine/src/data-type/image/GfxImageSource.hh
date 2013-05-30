#pragma once

class GfxImageSource
{
public:
	constexpr GfxImageSource() { }
	virtual ~GfxImageSource() { }
	virtual CallResult getImage(Pixmap &dest) = 0;
	virtual uint width() = 0;
	virtual uint height() = 0;
	virtual const PixelFormatDesc *pixelFormat() = 0;
	bool isGrayscale() { return pixelFormat()->isGrayscale(); }
	bool bitsPP() { return pixelFormat()->bitsPerPixel; }
};
