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
#include <imagine/logger/logger.h>

namespace Gfx
{

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
void SpriteBase<BaseRect>::setUVBounds(GTexCRect uvBounds)
{
	if(!uvBounds.xSize())
		logWarn("setting Empty UV bounds");
	//logMsg("setting UV bounds:%f:%f:%f:%f", (double)uvBounds.x, (double)uvBounds.y, (double)uvBounds.x2, (double)uvBounds.y2);
	BaseRect::setUV(uvBounds);
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw(RendererCommands &cmds) const
{
	if(img) [[likely]]
	{
		cmds.setTexture(*img);
		BaseRect::draw(cmds);
	}
}

template<class BaseRect>
bool SpriteBase<BaseRect>::compileDefaultProgram(uint32_t mode)
{
	if(img)
		return img->compileDefaultProgram(mode);
	else
		return false;
}

template<class BaseRect>
bool SpriteBase<BaseRect>::compileDefaultProgramOneShot(uint32_t mode)
{
	if(img)
		return img->compileDefaultProgramOneShot(mode);
	else
		return false;
}

template<class BaseRect>
void SpriteBase<BaseRect>::setCommonProgram(RendererCommands &cmds, uint32_t mode, const Mat4 *modelMat) const
{
	if(img)
		img->useDefaultProgram(cmds, mode, modelMat);
}

template<class BaseRect>
void SpriteBase<BaseRect>::setCommonProgram(RendererCommands &cmds, uint32_t mode, Mat4 modelMat) const
{
	setCommonProgram(cmds, mode, &modelMat);
}

template<class BaseRect>
const Texture *SpriteBase<BaseRect>::image() const
{
	return img;
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

}
