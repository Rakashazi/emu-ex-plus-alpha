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

#define thisModuleName "gfx:opengl"
#include <engine-globals.h>
#include <gfx/Gfx.hh>
#include <logger/interface.h>
#include <mem/interface.h>
#include <base/Base.hh>
#include <util/number.h>

uint gfx_frameTime = 0, gfx_frameTimeRel = 0;

#ifdef CONFIG_GFX_OPENGL_GLEW_STATIC
	#define Pixmap _X11Pixmap
	#include <util/glew/glew.c>
	#undef Pixmap
#endif

#if defined(__APPLE__) && ! defined (CONFIG_BASE_IOS)
	#include <OpenGL/OpenGL.h>
#endif

#include "glStateCache.h"
#include <util/Matrix4x4.hh>
#include <util/Motion.hh>
#include "utils.h"
#include <gfx/common/space.h>

GLStateCache glState;
static int animateOrientationChange = !Config::envIsWebOS3;
TimedMotion<GC> projAngleM;

#include "settings.h"
#include "transforms.hh"

#include "geometry.hh"
#include "texture.hh"

#include "startup-shutdown.h"
#include "commit.hh"

namespace Gfx
{

void setActiveTexture(GfxTextureHandle texture, uint type)
{
	#if !defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
	type = GL_TEXTURE_2D;
	#endif
	if(texture != 0)
	{
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		//GLenum otherTarget = type == GL_TEXTURE_EXTERNAL_OES ? GL_TEXTURE_2D : GL_TEXTURE_EXTERNAL_OES;
		//glcDisable(otherTarget);
		if(type == GL_TEXTURE_2D)
			glcDisable(GL_TEXTURE_EXTERNAL_OES);
		// enabling GL_TEXTURE_EXTERNAL_OES overrides GL_TEXTURE_2D so no need to
		// handle disabling GL_TEXTURE_2D
		#endif
		glcBindTexture(type, texture);
		glcEnable(type);
	}
	else
	{
		glcDisable(GL_TEXTURE_2D);
		#if defined(CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES)
		glcDisable(GL_TEXTURE_EXTERNAL_OES);
		#endif
		// TODO: binding texture 0 causes implicit glDisable(GL_TEXTURE_2D) ?
	}
}

void setZTest(bool on)
{
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
		clearZBufferBit = GL_DEPTH_BUFFER_BIT;
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
		clearZBufferBit = 0;
	}
}

void setBlendMode(uint mode)
{
	switch(mode)
	{
		bcase BLEND_MODE_OFF:
			glcDisable(GL_BLEND);
		bcase BLEND_MODE_ALPHA:
			glcBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); // general blending
			//glcBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // for premultiplied alpha
			glcEnable(GL_BLEND);
		bcase BLEND_MODE_INTENSITY:
			glcBlendFunc(GL_SRC_ALPHA,GL_ONE); // for text
			glcEnable(GL_BLEND);
	}
}

void setImgMode(uint mode)
{
	switch(mode)
	{
		bcase IMG_MODE_REPLACE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		bcase IMG_MODE_MODULATE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		bcase IMG_MODE_ADD: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		bcase IMG_MODE_BLEND: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
	}
}

void setBlendEquation(uint mode)
{
#if !defined CONFIG_GFX_OPENGL \
	|| (defined CONFIG_BASE_IOS || defined CONFIG_BASE_ANDROID || defined CONFIG_BASE_PS3)
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
			mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
			mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
			GL_FUNC_ADD);
#endif
}

void setImgBlendColor(GColor r, GColor g, GColor b, GColor a)
{
	GLfloat col[4] = { r, g, b, a } ;
	glcTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
}

void setZBlend(uchar on)
{
	if(on)
	{
		#ifndef CONFIG_GFX_OPENGL_ES
			glFogi(GL_FOG_MODE, GL_LINEAR);
		#else
			glFogf(GL_FOG_MODE, GL_LINEAR);
		#endif
		glFogf(GL_FOG_DENSITY, 0.1f);
		glHint(GL_FOG_HINT, GL_DONT_CARE);
		glFogf(GL_FOG_START, zRange/2.0);
		glFogf(GL_FOG_END, zRange);
		glcEnable(GL_FOG);
	}
	else
	{
		glcDisable(GL_FOG);
	}
}

void setZBlendColor(GColor r, GColor g, GColor b)
{
	GLfloat c[4] = {r, g, b, 1.0f};
	glFogfv(GL_FOG_COLOR, c);
}

void setColor(GColor r, GColor g, GColor b, GColor a)
{
	glcColor4f(r, g, b, a);
}

uint color()
{
	return ColorFormat.build(glState.colorState[0], glState.colorState[1], glState.colorState[2], glState.colorState[3]);
}

void setVisibleGeomFace(uint faces)
{
	if(faces == BOTH_FACES)
	{
		glcDisable(GL_CULL_FACE);
	}
	else if(faces == FRONT_FACES)
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // our order is reversed from OpenGL
	}
	else
	{
		glcEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

}

void pointerPos(int x, int y, int *xOut, int *yOut);

namespace Gfx
{

void setClipRect(bool on)
{
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void setClipRectBounds(int x, int y, int w, int h)
{
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	switch(rotateView)
	{
		bcase VIEW_ROTATE_0:
			y = (Base::window().rect.ySize() - y) - h;
		bcase VIEW_ROTATE_90:
			y = (Base::window().rect.xSize() - y) - h;
			IG::swap(x, y);
			IG::swap(w, h);
		bcase VIEW_ROTATE_270:
			y += Base::window().rect.x;
			IG::swap(x, y);
			IG::swap(w, h);
		bcase VIEW_ROTATE_180:
			y += Base::window().rect.x;
	}
	#else
	y = (Base::window().rect.ySize() - y) - h;
	#endif
	//logMsg("setting Scissor %d,%d size %d,%d", x, y, w, h);
	glScissor(x, y, w, h);
}

void setClear(bool on)
{
	// always clear screen on android since not doing so seems to have a performance hit
	#if !defined(CONFIG_GFX_OPENGL_ES)
		clearColorBufferBit = on ? GL_COLOR_BUFFER_BIT : 0;
	#endif
}

void setClearColor(GColor r, GColor g, GColor b, GColor a)
{
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	// TODO: add glClearColor to the state cache
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

}
