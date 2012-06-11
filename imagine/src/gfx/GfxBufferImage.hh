#pragma once

#include <gfx/defs.hh>
#include <pixmap/Pixmap.hh>

#if defined(CONFIG_RESOURCE_IMAGE)
class ResourceImage;
#endif

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	bool gfx_androidDirectTextureSupported();
	bool gfx_androidDirectTextureSupportWhitelisted();
	int gfx_androidDirectTextureError();
	void gfx_setAndroidDirectTexture(bool on);

	enum
	{
		ANDROID_DT_SUCCESS,
		ANDROID_DT_ERR_NO_EGL_IMG_EXT,
		ANDROID_DT_ERR_NO_LIBEGL, ANDROID_DT_ERR_NO_CREATE_IMG, ANDROID_DT_ERR_NO_DESTROY_IMG,
		ANDROID_DT_ERR_NO_GET_DISPLAY, ANDROID_DT_ERR_GET_DISPLAY,
		ANDROID_DT_ERR_NO_LIBHARDWARE, ANDROID_DT_ERR_NO_HW_GET_MODULE,
		ANDROID_DT_ERR_NO_GRALLOC_MODULE, ANDROID_DT_ERR_NO_ALLOC_DEVICE,

		ANDROID_DT_ERR_TEST_ALLOC, ANDROID_DT_ERR_TEST_LOCK,
		ANDROID_DT_ERR_TEST_UNLOCK, ANDROID_DT_ERR_TEST_IMG_CREATE,
		ANDROID_DT_ERR_TEST_TARGET_TEXTURE,

		ANDROID_DT_ERR_FORCE_DISABLE
	};

	const char *gfx_androidDirectTextureErrorString(int err);
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
	virtual Pixmap *lock(uint x, uint y, uint xlen, uint ylen) = 0;
	virtual void unlock() = 0;
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
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen);
	void unlock();
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
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen) { return impl->lock(x, y, xlen, ylen); }
	void unlock() { impl->unlock(); }
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
	CallResult init(ResourceImage &img, uint filter = linear, uint hints = 0);
	//CallResult subInit(ResourceImage &img, int x, int y, int xSize, int ySize);
	#endif

	void removeBacker() { backingImg = 0; }
	void setFilter(uint filter);
	void deinit();
	//const GfxTextureDesc &textureDesc() const { return *implPtr(); };

	void write(Pixmap &p);
	void replace(Pixmap &p);
	/*Pixmap *lock(uint x, uint y, uint xlen, uint ylen);
	void unlock();
	void deinit();*/
};
