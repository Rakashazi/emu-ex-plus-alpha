#pragma once

namespace Gfx
{

class BufferImageInterface
{
public:
	virtual ~BufferImageInterface() {}
	virtual void write(IG::Pixmap &p, uint hints) = 0;
	virtual void write(IG::Pixmap &p, uint hints, uint alignment) = 0;
	virtual void replace(IG::Pixmap &p, uint hints) = 0;
	virtual IG::Pixmap *lock(uint x, uint y, uint xlen, uint ylen, IG::Pixmap *fallback) = 0;
	virtual void unlock(IG::Pixmap *pix, uint hints) = 0;
	virtual void deinit() = 0;
	virtual const TextureDesc &textureDesc() const = 0;
	virtual TextureDesc &textureDesc() = 0;
};

class TextureBufferImage
#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
: public BufferImageInterface
#endif
{
public:
	constexpr TextureBufferImage() {}
	void setSwizzleForFormat(const PixelFormatDesc &format);

protected:
	TextureDesc desc;
	void write(IG::Pixmap &p, uint hints);
	void write(IG::Pixmap &p, uint hints, uint alignment);
	void replace(IG::Pixmap &p, uint hints);
	IG::Pixmap *lock(uint x, uint y, uint xlen, uint ylen, IG::Pixmap *fallback);
	void unlock(IG::Pixmap *pix, uint hints);
	void deinit();
	const TextureDesc &textureDesc() const { return desc; };
	TextureDesc &textureDesc() { return desc; };
	bool isInit() const { return desc.tid != 0; }
};

class TextureBufferVImpl
{
public:
	constexpr TextureBufferVImpl() {}

protected:
	BufferImageInterface *impl = nullptr;
	void write(IG::Pixmap &p, uint hints) { impl->write(p, hints); }
	void write(IG::Pixmap &p, uint hints, uint alignment) { impl->write(p, hints, alignment); }
	void replace(IG::Pixmap &p, uint hints) { impl->replace(p, hints); }
	IG::Pixmap *lock(uint x, uint y, uint xlen, uint ylen, IG::Pixmap *fallback) { return impl->lock(x, y, xlen, ylen, fallback); }
	void unlock(IG::Pixmap *pix, uint hints) { impl->unlock(pix, hints); }
	void deinit() { impl->deinit(); delete impl; impl = nullptr; }
	const TextureDesc &textureDesc() const { return impl->textureDesc(); }
	TextureDesc &textureDesc() { return impl->textureDesc(); }
	bool isInit() const { return impl; }
};

#ifdef CONFIG_GFX_OPENGL_BUFFER_IMAGE_MULTI_IMPL
using BufferImageImpl = TextureBufferVImpl;
#else
using BufferImageImpl = TextureBufferImage;
#endif

}
