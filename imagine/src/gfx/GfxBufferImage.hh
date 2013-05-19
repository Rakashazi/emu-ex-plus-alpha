#pragma once

#include <gfx/defs.hh>
#include <pixmap/Pixmap.hh>
#include <data-type/image/GfxImageSource.hh>
#include <util/RefCount.hh>

namespace Gfx
{

class TextureDesc
{
public:
	constexpr TextureDesc() { }
	GfxTextureHandle tid = 0;
	#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	static const GLenum target = GL_TEXTURE_2D;
	#endif
	TextureCoordinate xStart = 0, xEnd = 0;
	TextureCoordinate yStart = 0, yEnd = 0;
};

struct BufferImageInterface : public TextureDesc
{
	virtual ~BufferImageInterface() { }
protected:
	virtual void write(Pixmap &p, uint hints) = 0;
	virtual void write(Pixmap &p, uint hints, uint alignment) = 0;
	virtual void replace(Pixmap &p, uint hints) = 0;
	virtual Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) = 0;
	virtual void unlock(Pixmap *pix, uint hints) = 0;
	virtual void deinit() = 0;
	friend struct TextureBufferVImpl;
};

struct TextureBufferImage:
#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
	public BufferImageInterface
#else
	public TextureDesc
#endif
{
	constexpr TextureBufferImage() { }
protected:
	void write(Pixmap &p, uint hints);
	void write(Pixmap &p, uint hints, uint alignment);
	void replace(Pixmap &p, uint hints);
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback);
	void unlock(Pixmap *pix, uint hints);
	void deinit();
	const TextureDesc &textureDesc() const { return *this; };
	TextureDesc &textureDesc() { return *this; };
	bool isInit() const { return tid != 0; }
};

struct TextureBufferVImpl
{
	constexpr TextureBufferVImpl() { }
protected:
	BufferImageInterface *impl = nullptr;
	void write(Pixmap &p, uint hints) { impl->write(p, hints); };
	void write(Pixmap &p, uint hints, uint alignment) { impl->write(p, hints, alignment); };
	void replace(Pixmap &p, uint hints) { impl->replace(p, hints); };
	Pixmap *lock(uint x, uint y, uint xlen, uint ylen, Pixmap *fallback) { return impl->lock(x, y, xlen, ylen, fallback); }
	void unlock(Pixmap *pix, uint hints) { impl->unlock(pix, hints); }
	void deinit() { impl->deinit(); delete impl; impl = 0; }
	const TextureDesc &textureDesc() const { assert(impl); return *impl; };
	TextureDesc &textureDesc() { assert(impl); return *impl; };
	bool isInit() const { return impl != nullptr; }
};

#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
	typedef TextureBufferVImpl BufferImageImpl;
#else
	typedef TextureBufferImage BufferImageImpl;
#endif

class BufferImage: public BufferImageImpl, public RefCount<BufferImage>
{
private:
	uint hints = 0;
	bool hasMipmaps_ = 0;
	void testMipmapSupport(uint x, uint y);
	bool setupTexture(Pixmap &pix, bool upload, uint internalFormat, int xWrapType, int yWrapType,
			uint usedX, uint usedY, uint hints, uint filter);
public:
	constexpr BufferImage() { }
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	uint xSize = 0, ySize = 0; // the actual x,y size of the image content
	#endif
	static const uint nearest = 0, linear = 1;
	static bool isFilterValid(uint v) { return v <= 1; }
	bool hasMipmaps();
	static const uint HINT_STREAM = BIT(0), HINT_NO_MINIFY = BIT(1);
	CallResult init(Pixmap &pix, bool upload, uint filter, uint hints, bool textured);
	CallResult init(Pixmap &pix, bool upload, uint filter = linear, uint hints = HINT_STREAM)
	{
		return init(pix, upload, filter, hints, 0);
	}
	CallResult init(GfxImageSource &img, uint filter = linear, uint hints = 0, bool textured = 0);
	static constexpr uint MAX_ASSUME_ALIGN = 8;
	static uint bestAlignment(const Pixmap &p);
	void setFilter(uint filter);
	void setRepeatMode(uint xMode, uint yMode);
	void deinit();
	void write(Pixmap &p);
	void write(Pixmap &p, uint assumeAlign);
	void replace(Pixmap &p);
	void unlock(Pixmap *p);
	void free()
	{
		logMsg("BufferImage %p has no more references", this);
		deinit();
	};

	const TextureDesc &textureDesc() const { return BufferImageImpl::textureDesc(); };
	TextureDesc &textureDesc() { return BufferImageImpl::textureDesc(); };

	operator bool() const
	{
		return isInit();
	}
};

}
