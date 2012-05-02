#pragma once

#include <engine-globals.h>

#include <io/sys.hh>
#ifdef CONFIG_IO_MMAP_GENERIC
	#include <io/mmap/generic/IoMmapGeneric.hh>
#endif
#include <resource2/image/ResourceImage.h>
#include <data-type/image/libpng/reader.h>

class ResourceImagePng : public ResourceImage
{
public:
	static ResourceImage * load(Io* io);
	static ResourceImage * load(const char * name);
	#ifdef CONFIG_IO_MMAP_GENERIC
	static ResourceImage * load(const uchar *buffer, size_t bytes)
	{
		return load(IoMmapGeneric::open(buffer, bytes));
	}
	#endif
	static ResourceImage * loadAsset(const char * name)
	{
		return load(openAppAssetIo(name));
	}
	void free();
	CallResult getImage(Pixmap* dest);
	int alphaChannelType();
	uint width();
	uint height();
	const PixelFormatDesc *pixelFormat() const;
	float aspectRatio();
private:
	static ResourceImage * loadWithIoWithName (Io* io, const char *name);
	Io* io;
	Png png;
	const PixelFormatDesc *format;
};
