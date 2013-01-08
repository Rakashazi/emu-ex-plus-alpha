#pragma once

#include <gfx/Gfx.hh>
#include <gfx/GfxBufferImage.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GeomQuad.hh>
#include <gfx/VertexArray.hh>
#if defined(CONFIG_RESOURCE_IMAGE)
#include <resource2/image/ResourceImage.h>
#endif

namespace Gfx
{

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase() { }
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, BufferImage *img);
	CallResult init(Coordinate x = 0, Coordinate y = 0, Coordinate x2 = 0, Coordinate y2 = 0)
	{
		return init(x, y, x2, y2, (BufferImage*)nullptr);
	}
	CallResult init(BufferImage *img)
	{
		return init(0, 0, 0, 0, img);
	}

	#if defined(CONFIG_RESOURCE_IMAGE)
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, ResourceImage *img)
	{
		return init(x, y, x2, y2, img ? &img->gfxD : (BufferImage*)nullptr);
	}
	CallResult init(ResourceImage *img)
	{
		return init(img ? &img->gfxD : (BufferImage*)nullptr);
	}
	#endif

	void deinit();

	void setImg(BufferImage *img);
	void setImg(BufferImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);

	#if defined(CONFIG_RESOURCE_IMAGE)
	void setImg(ResourceImage *img)
	{
		setImg(img ? &img->gfxD : (BufferImage*)nullptr);
	}
	void setImg(ResourceImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
	{
		setImg(img ? &img->gfxD : (BufferImage*)nullptr, leftTexU, topTexV, rightTexU, bottomTexV);
	}
	#endif


	void draw() const;

	void deinitAndFreeImg()
	{
		deinit();
		if(img)
		{
			img->deinit();
			img = nullptr;
		}
	}

	BufferImage *img = nullptr;
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	uint flags = 0;
	static constexpr uint HINT_NO_MATRIX_TRANSFORM = BIT(0);
	int screenX = 0, screenY = 0, screenX2 = 0, screenY2 = 0;
	#endif
};

typedef SpriteBase<TexRect> Sprite;
typedef SpriteBase<ColTexQuad> ShadedSprite;

}
