#pragma once

#include <gfx/defs.hh>
#include <pixmap/Pixmap.hh>

#if defined(CONFIG_RESOURCE_IMAGE)
class ResourceImage;
#endif

class GfxTextureDesc
{
public:
	constexpr GfxTextureDesc() { }
	GfxTextureHandle tid = 0;
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	static const GLenum target = GL_TEXTURE_2D;
	#endif
	TextureCoordinate xStart = 0, xEnd = 0;
	TextureCoordinate yStart = 0, yEnd = 0;
};

class GfxUsableImage
{
public:
	constexpr GfxUsableImage() { }
	virtual void deinit() = 0;
};

struct GfxBufferImageInterface : public GfxTextureDesc
{
	virtual ~GfxBufferImageInterface() { }
	virtual void write(Pixmap &p, uint hints) = 0;
	virtual void replace(Pixmap &p, uint hints) = 0;
	virtual Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) = 0;
	virtual void unlock(Pixmap *pix, uint hints) = 0;
	virtual void deinit() = 0;
};

struct TextureGfxBufferImage:
#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
	public GfxBufferImageInterface
#else
	public GfxTextureDesc
#endif
{
	constexpr TextureGfxBufferImage() { }
	void write(Pixmap &p, uint hints);
	void replace(Pixmap &p, uint hints);
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback);
	void unlock(Pixmap *pix, uint hints);
	void deinit();
	const GfxTextureDesc &textureDesc() const { return *this; };
	GfxTextureDesc &textureDesc() { return *this; };
	fbool isInit() { return tid != 0; }
};

struct TextureGfxBufferVImpl
{
	constexpr TextureGfxBufferVImpl() { }
	GfxBufferImageInterface *impl = nullptr;
	void write(Pixmap &p, uint hints) { impl->write(p, hints); };
	void replace(Pixmap &p, uint hints) { impl->replace(p, hints); };
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) { return impl->lock(x, y, xlen, ylen, fallback); }
	void unlock(Pixmap *pix, uint hints) { impl->unlock(pix, hints); }
	void deinit() { impl->deinit(); delete impl; impl = 0; }
	const GfxTextureDesc &textureDesc() const { assert(impl); return *impl; };
	GfxTextureDesc &textureDesc() { assert(impl); return *impl; };
	fbool isInit() { return impl != nullptr; }
};

#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
	typedef TextureGfxBufferVImpl GfxBufferImageImpl;
#else
	typedef TextureGfxBufferImage GfxBufferImageImpl;
#endif

class GfxBufferImage: public GfxBufferImageImpl
{
private:
	uint hints = 0;
	fbool hasMipmaps_ = 0;
	GfxUsableImage *backingImg = nullptr;
	void testMipmapSupport(uint x, uint y);
	fbool setupTexture(Pixmap &pix, bool upload, uint internalFormat, int xWrapType, int yWrapType,
			uint usedX, uint usedY, uint hints, uint filter);
public:
	constexpr GfxBufferImage() { }
	static const uint nearest = 0, linear = 1;
	static bool isFilterValid(uint v) { return v <= 1; }
	bool hasMipmaps();
	static const uint HINT_STREAM = BIT(0), HINT_NO_MINIFY = BIT(1);
	CallResult init(Pixmap &pix, bool upload, uint filter, uint hints, bool textured);
	CallResult init(Pixmap &pix, bool upload, uint filter = linear, uint hints = HINT_STREAM)
	{
		return init(pix, upload, filter, hints, 0);
	}
	#if defined(CONFIG_RESOURCE_IMAGE)
	CallResult init(ResourceImage &img, uint filter = linear, uint hints = 0, bool textured = 0);
	//CallResult subInit(ResourceImage &img, int x, int y, int xSize, int ySize);
	#endif

	void removeBacker() { backingImg = 0; }
	void setFilter(uint filter);
	void setRepeatMode(uint xMode, uint yMode);
	void deinit();
	void write(Pixmap &p);
	void replace(Pixmap &p);
	void unlock(Pixmap *p);
};
