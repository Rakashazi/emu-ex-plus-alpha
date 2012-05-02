#pragma once

#include <gfx/Gfx.hh>
#include <gfx/GfxBufferImage.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GeomQuad.hh>
#include <gfx/VertexArray.hh>
#if defined(CONFIG_RESOURCE_IMAGE)
#include <resource2/image/ResourceImage.h>
#endif

template<class BaseRect>
class GfxSpriteBase : public BaseRect
{
public:
	constexpr GfxSpriteBase(): img(0) { }
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, GfxBufferImage *img);
	CallResult init(Coordinate x = 0, Coordinate y = 0, Coordinate x2 = 0, Coordinate y2 = 0)
	{
		return init(x, y, x2, y2, (GfxBufferImage*)nullptr);
	}
	CallResult init(GfxBufferImage *img)
	{
		return init(0, 0, 0, 0, img);
	}

	#if defined(CONFIG_RESOURCE_IMAGE)
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, ResourceImage *img)
	{
		return init(x, y, x2, y2, img ? &img->gfxD : (GfxBufferImage*)nullptr);
	}
	CallResult init(ResourceImage *img)
	{
		return init(img ? &img->gfxD : (GfxBufferImage*)nullptr);
	}
	#endif

	void deinit();

	void setImg(GfxBufferImage *img);
	void setImg(GfxBufferImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);

	#if defined(CONFIG_RESOURCE_IMAGE)
	void setImg(ResourceImage *img)
	{
		setImg(img ? &img->gfxD : (GfxBufferImage*)nullptr);
	}
	void setImg(ResourceImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
	{
		setImg(img ? &img->gfxD : (GfxBufferImage*)nullptr, leftTexU, topTexV, rightTexU, bottomTexV);
	}
	#endif


	void draw(uint manageBlend = 1) const;

	void deinitAndFreeImg()
	{
		deinit();
		if(img)
		{
			img->deinit();
			img = 0;
		}
	}

	GfxBufferImage *img;
};

typedef GfxSpriteBase<GfxTexRect> GfxSprite;
typedef GfxSpriteBase<GfxColTexQuad> GfxShadedSprite;
