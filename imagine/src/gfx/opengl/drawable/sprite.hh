#pragma once
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/RendererCommands.hh>


namespace Gfx
{

template<class BaseRect>
void SpriteBase<BaseRect>::init(GCRect pos, TextureSpan span)
{
	BaseRect::init(pos.x, pos.y, pos.x2, pos.y2);
	setImg(span);
}

template<class BaseRect>
void SpriteBase<BaseRect>::setImg(const Texture *newImg)
{
	img = newImg;
}

template<class BaseRect>
void SpriteBase<BaseRect>::setImg(TextureSpan span)
{
	setImg(span.texture());
	setUVBounds(span.uvBounds());
}

template<class BaseRect>
void SpriteBase<BaseRect>::setUVBounds(IG::Rect2<GTexC> uvBounds)
{
	if(!uvBounds.xSize())
		logWarn("setting Empty UV bounds");
	//logMsg("setting UV bounds:%f:%f:%f:%f", (double)uvBounds.x, (double)uvBounds.y, (double)uvBounds.x2, (double)uvBounds.y2);
	mapImg(BaseRect::v, uvBounds.x, uvBounds.y, uvBounds.x2, uvBounds.y2);
}

template<class BaseRect>
void SpriteBase<BaseRect>::deinit()
{
	BaseRect::deinit();
	setImg(nullptr);
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw(RendererCommands &cmds) const
{
	if(likely(img))
	{
		cmds.setTexture(*img);
		BaseRect::draw(cmds);
	}
}

std::array<TexVertex, 4> makeTexVertArray(GCRect pos, TextureSpan img)
{
	std::array<TexVertex, 4> arr{};
	setPos(arr, pos.x, pos.y, pos.x2, pos.y2);
	auto uvBounds = img.uvBounds();
	mapImg(arr, uvBounds.x, uvBounds.y, uvBounds.x2, uvBounds.y2);
	return arr;
}

template class SpriteBase<TexRect>;
template class SpriteBase<ColTexQuad>;

}
