#pragma once

#include <engine-globals.h>
#include <pixmap/Pixmap.hh>
#include <data-type/image/GfxImageSource.hh>
#include <io/sys.hh>
#include <fs/sys.hh>

#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

class PixelFormatDesc;

class Png
{
public:
	constexpr Png() {}
	CallResult readHeader(Io *stream);
	CallResult readImage(Io *stream, void* buffer, uint bufferLinePadBytes, const PixelFormatDesc &outFormat);
	bool hasAlphaChannel();
	bool isGrayscale();
	void freeImageData();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat();

private:
	png_structp png = nullptr;
	png_infop info = nullptr;
	//png_infop end;
	void setTransforms(const PixelFormatDesc &outFormat, png_infop transInfo);
	static bool supportUncommonConv;
};

class PngFile : public GfxImageSource
{
public:
	PngFile() {}
	~PngFile()
	{
		deinit();
	}
	CallResult load(Io *io);
	CallResult load(const char *name);
	CallResult loadAsset(const char *name)
	{
		return load(openAppAssetIo(name));
	}
	void deinit();
	CallResult getImage(Pixmap &dest) override;
	uint width() override { return png.width(); }
	uint height() override { return png.height(); }
	const PixelFormatDesc *pixelFormat() override { return png.pixelFormat(); }

private:
	Png png;
	Io *io = nullptr;
};
