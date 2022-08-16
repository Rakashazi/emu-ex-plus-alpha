/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gfx/GfxSprite.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/logger/logger.h>

namespace IG::Gfx
{

template<class BaseRect>
SpriteBase<BaseRect>::SpriteBase(GCRect pos, TextureSpan span):
	BaseRect{pos, span.uvBounds()},
	texBinding{span.texture() ? span.texture()->binding() : TextureBinding{}} {}

template<class BaseRect>
void SpriteBase<BaseRect>::set(const Texture *tex)
{
	if(tex)
		texBinding = tex->binding();
	else
		texBinding = {};
}

template<class BaseRect>
void SpriteBase<BaseRect>::set(TextureSpan span, Rotation r)
{
	set(span.texture());
	setUVBounds(span.uvBounds(), r);
}

template<class BaseRect>
void SpriteBase<BaseRect>::setUVBounds(FRect uvBounds, Rotation r)
{
	if(!uvBounds.xSize())
		logWarn("setting Empty UV bounds");
	//logMsg("setting UV bounds:%f:%f:%f:%f", (double)uvBounds.x, (double)uvBounds.y, (double)uvBounds.x2, (double)uvBounds.y2);
	BaseRect::setUV(uvBounds, r);
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw(RendererCommands &cmds) const
{
	if(!texBinding.name) [[unlikely]]
		return;
	cmds.set(texBinding);
	BaseRect::draw(cmds);
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw(RendererCommands &cmds, BasicEffect &prog) const
{
	if(!texBinding.name) [[unlikely]]
		return;
	prog.enableTexture(cmds, texBinding);
	BaseRect::draw(cmds);
}

std::array<TexVertex, 4> makeTexVertArray(GCRect pos, TextureSpan img)
{
	std::array<TexVertex, 4> arr{};
	arr = mapQuadPos(arr, pos);
	auto uvBounds = img.uvBounds();
	arr = mapQuadUV(arr, uvBounds);
	return arr;
}

template class SpriteBase<TexRect>;
template class SpriteBase<ColTexQuad>;

TextureSpan::operator bool() const
{
	return tex && (bool)*tex;
}

}
