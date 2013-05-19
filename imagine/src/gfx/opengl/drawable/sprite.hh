#pragma once
#include <gfx/GfxSprite.hh>

/*
void TexVertexQuad::draw() const
{
	if(useVBOFuncs)
	{
		//logMsg("binding VBO id %d", vArr.ref);
		glState_bindBuffer(GL_ARRAY_BUFFER, vArr.ref);
		TexVertex::draw(0, TRIANGLE_STRIP, 4);
	}
	else
		TexVertex::draw(v, TRIANGLE_STRIP, 4);
}
*/

namespace Gfx
{

template<class BaseRect>
CallResult SpriteBase<BaseRect>::init(GC x, GC y, GC x2, GC y2, BufferImage *img)
{
	BaseRect::init(x, y, x2, y2);

	//logMsg("set sprite img %p", img);
	setImg(img);

	return OK;
}

#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
static void setupCropRect(BufferImage *img)
{
	assert(useDrawTex);
	logMsg("setting GL_TEXTURE_CROP_RECT_OES %d,%d", img->xSize, img->ySize);
	GLint coords[] = {0, (int)img->ySize, (int)img->xSize, -(int)img->ySize};
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	GLenum target = GL_TEXTURE_2D;
	#else
	GLenum target = img->textureDesc().target;
	#endif
	Gfx::setActiveTexture(img->textureDesc().tid, target);
	glTexParameteriv(target, GL_TEXTURE_CROP_RECT_OES, coords);
}
#endif

template<class BaseRect>
void SpriteBase<BaseRect>::setRefImg(BufferImage *newImg)
{
	if(!newImg)
	{
		if(img)
			img->freeRef();
		img = nullptr;
		return;
	}

	newImg->ref();
	if(img)
		img->freeRef();
	img = newImg;
}

template<class BaseRect>
void SpriteBase<BaseRect>::setImg(BufferImage *newImg)
{
	setRefImg(newImg);
	if(!newImg)
		return;

	::mapImg(BaseRect::v, newImg->textureDesc());
	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	if(flags & HINT_NO_MATRIX_TRANSFORM && useDrawTex)
		setupCropRect(img);
	#endif
}

template<class BaseRect>
void SpriteBase<BaseRect>::setImg(BufferImage *newImg, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
{
	setRefImg(newImg);

	::mapImg(BaseRect::v, leftTexU, topTexV, rightTexU, bottomTexV);
}

template<class BaseRect>
void SpriteBase<BaseRect>::deinit()
{
	setRefImg(nullptr);
	BaseRect::deinit();
}

template<class BaseRect>
void SpriteBase<BaseRect>::draw() const
{
	Gfx::setActiveTexture(img->textureDesc().tid, img->textureDesc().target);

	#if defined CONFIG_BASE_ANDROID && defined CONFIG_GFX_OPENGL_USE_DRAW_TEXTURE
	if(flags & HINT_NO_MATRIX_TRANSFORM && useDrawTex && projAngleM.isComplete())
	{
		glDrawTexiOES(screenX, screenY, 1, screenX2, screenY2);
	}
	else
	#endif
	{
		BaseRect::draw();
	}
}

template class SpriteBase<TexRect>;
template class SpriteBase<ColTexQuad>;

}
