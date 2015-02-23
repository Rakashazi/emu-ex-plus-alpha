#pragma once
#include <imagine/gfx/GfxSprite.hh>


namespace Gfx
{

template<class BaseRect>
CallResult SpriteBase<BaseRect>::init(GCRect pos, Texture *img, IG::Rect2<GTexC> uvBounds)
{
	BaseRect::init(pos.x, pos.y, pos.x2, pos.y2);
	setImg(img);
	setUVBounds(uvBounds);
	return OK;
}

template<class BaseRect>
void SpriteBase<BaseRect>::setImg(Texture *newImg)
{
	img = newImg;
}

template<class BaseRect>
void SpriteBase<BaseRect>::setUVBounds(IG::Rect2<GTexC> uvBounds)
{
	if(uvBounds.xSize())
	{
		//logMsg("setting UV bounds:%f:%f:%f:%f", (double)uvBounds.x, (double)uvBounds.y, (double)uvBounds.x2, (double)uvBounds.y2);
		mapImg(BaseRect::v, uvBounds.x, uvBounds.y, uvBounds.x2, uvBounds.y2);
	}
	else
	{
		mapImg(BaseRect::v, 0., 0., 1., 1.);
	}
}

template<class BaseRect>
void SpriteBase<BaseRect>::deinit()
{
	BaseRect::deinit();
	setImg(nullptr);
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw() const
{
	if(likely(img))
	{
		img->bind();
		BaseRect::draw();
	}
}

template class SpriteBase<TexRect>;
template class SpriteBase<ColTexQuad>;

}
