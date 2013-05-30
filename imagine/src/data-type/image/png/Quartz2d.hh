#pragma once

#include <engine-globals.h>
#include <pixmap/Pixmap.hh>
#include <data-type/image/GfxImageSource.hh>
#include <io/sys.hh>
#include <fs/sys.hh>
#include <CoreGraphics/CGImage.h>

class PixelFormatDesc;

class Quartz2dImage
{
public:
	constexpr Quartz2dImage() {}
	CallResult load(const char *name);
	CallResult readImage(void *buffer, uint bufferLinePadBytes, const PixelFormatDesc &outFormat);
	static CallResult writeImage(const Pixmap &pix, const char *name);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat();

private:
	CGImageRef img = nullptr;
};

class PngFile : public GfxImageSource
{
public:
	PngFile() {}
	~PngFile()
	{
		deinit();
	}
	CallResult load(const char *name);
	CallResult loadAsset(const char *name);
	void deinit();
	CallResult getImage(Pixmap &dest) override;
	uint width() override { return png.width(); }
	uint height() override { return png.height(); }
	const PixelFormatDesc *pixelFormat() override { return png.pixelFormat(); }

private:
	Quartz2dImage png;
};
