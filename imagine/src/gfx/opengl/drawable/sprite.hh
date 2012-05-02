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

template<class BaseRect>
CallResult GfxSpriteBase<BaseRect>::init(GC x, GC y, GC x2, GC y2, GfxBufferImage *img)
{
	BaseRect::init(x, y, x2, y2);

	if(img)
	{
		//logMsg("set sprite img %p", img);
		setImg(img);
	}

	return OK;
}

template<class BaseRect>
void GfxSpriteBase<BaseRect>::setImg(GfxBufferImage *img)
{
	var_selfs(img);
	::mapImg(BaseRect::v, img ? &img->textureDesc() : 0);
}

template<class BaseRect>
void GfxSpriteBase<BaseRect>::setImg(GfxBufferImage *img, GTexC leftTexU, GTexC topTexV, GTexC rightTexU, GTexC bottomTexV)
{
	var_selfs(img);
	::mapImg(BaseRect::v, leftTexU, topTexV, rightTexU, bottomTexV);
}

template<class BaseRect>
void GfxSpriteBase<BaseRect>::deinit()
{
	BaseRect::deinit();
}

template<class BaseRect>
void GfxSpriteBase<BaseRect>::draw(uint manageBlend) const
{
	assert(!manageBlend);

	//gfx_setActiveTexture(img->tid);
	Gfx::setActiveTexture(img->textureDesc().tid, img->textureDesc().target);

	/*#if defined(CONFIG_GFX_OPENGL_ES) && !defined(CONFIG_BASE_PS3)
	if(useDrawTex && projAngleM.isComplete())
	{
		int xSize = gfx_toIXSize(v[2].x - v[0].x);
		int ySize = gfx_toIYSize(v[1].y - v[0].y);
		glDrawTexiOES(gfx_viewPixelWidth()/2 - (xSize/2),
				gfx_viewPixelHeight()/2 - (ySize/2),
				1, xSize, ySize);
	}
	else
	#endif*/
	{
		BaseRect::draw();
	}
}

template class GfxSpriteBase<GfxTexRect>;
template class GfxSpriteBase<GfxColTexQuad>;
