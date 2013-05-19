#pragma once

#include <gfx/Gfx.hh>
#include <gfx/GfxBufferImage.hh>
#include <gfx/GeomRect.hh>
#include <gfx/GeomQuad.hh>
#include <gfx/VertexArray.hh>

namespace Gfx
{

template<class BaseRect>
class SpriteBase : public BaseRect
{
public:
	constexpr SpriteBase() { }
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2, BufferImage *img);
	CallResult init(Coordinate x, Coordinate y, Coordinate x2, Coordinate y2)
	{
		return init(x, y, x2, y2, (BufferImage*)nullptr);
	}
	CallResult init()
	{
		return init(0, 0, 0, 0);
	}
	CallResult init(BufferImage *img)
	{
		return init(0, 0, 0, 0, img);
	}

	void deinit();

	void setImg(BufferImage *img);
	void setImg(BufferImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV);


	void draw() const;

	BufferImage *image() { return img; }

private:
	BufferImage *img = nullptr;
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	uint flags = 0;
	static constexpr uint HINT_NO_MATRIX_TRANSFORM = BIT(0);
	int screenX = 0, screenY = 0, screenX2 = 0, screenY2 = 0;
	#endif
	void setRefImg(BufferImage *img);
};

typedef SpriteBase<TexRect> Sprite;
typedef SpriteBase<ColTexQuad> ShadedSprite;

}
