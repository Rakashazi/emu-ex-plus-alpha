#pragma once

#include <engine-globals.h>
#include <resource2/Resource.h>
#include <pixmap/Pixmap.hh>
#include <gfx/GfxBufferImage.hh>
#include <util/RefCount.hh>

enum { IMAGE_ALPHA_CHANNEL_NONE, IMAGE_ALPHA_CHANNEL_USED, IMAGE_ALPHA_CHANNEL_IS_INTENSITY };
//enum { IMAGE_COLOR_GRADATION_DEFAULT, IMAGE_COLOR_GRADATION_LOW, IMAGE_COLOR_GRADATION_HIGH };
//enum { IMAGE_MASKED_PIXELS_USED, IMAGE_MASKED_PIXELS_NOT_USED };
//enum { IMAGE_CLASS_UNKNOWN, IMAGE_CLASS_TEXTURE, IMAGE_CLASS_NON_REPEATING, IMAGE_CLASS_NR = IMAGE_CLASS_NON_REPEATING, IMAGE_CLASS_TILED };
//enum { IMAGE_EDGES_UNKNOWN, IMAGE_EDGES_HARD, IMAGE_EDGES_SOFT };

class ResourceImage : /*public Resource,*/ public GfxUsableImage, public RefCount<ResourceImage>
{
public:
	constexpr ResourceImage() { }
	virtual ~ResourceImage() { }

	void init();
	//CallResult init(const char * name);
	static ResourceImage * load(const char * name);

	virtual void free() = 0;
	void freeSafe() { if(this) free(); }
	virtual CallResult getImage(Pixmap* dest) = 0;
	virtual int alphaChannelType() = 0;
	virtual uint width() = 0;
	virtual uint height() = 0;
	virtual const PixelFormatDesc *pixelFormat() const = 0;
	virtual float aspectRatio() = 0;
	float aR() { return aspectRatio(); };

	uchar isGrayScale() const { return pixelFormat()->isGrayscale(); }
	uchar bitsPP() const { return pixelFormat()->bitsPerPixel; }

	void setupPixmap(Pixmap &p)
	{
		p.init(0, pixelFormat(), width(), height(), 0);
	}

	GfxBufferImage gfxD;
	void deinit() { freeRef(); }
};
