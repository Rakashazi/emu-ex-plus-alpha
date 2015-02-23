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

#define LOGTAG "GfxOpenGL"
#include <imagine/gfx/Gfx.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/number.h>
#include "GLStateCache.hh"
#include <imagine/util/Interpolator.hh>
#include "private.hh"
#include "utils.h"

namespace Gfx
{

Base::GLContext gfxContext;
Base::Window *currWin{};
GLStateCache glState;
TimedInterpolator<Gfx::GC> projAngleM;
bool checkGLErrors = Config::DEBUG_BUILD;
bool checkGLErrorsVerbose = false;
static constexpr bool multipleContextsPerThread = Config::envIsAndroid;

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
bool useFixedFunctionPipeline = true;
#endif
static ColorComp vColor[4]{}; // color when using shader pipeline
static ColorComp texEnvColor[4]{}; // color when using shader pipeline
static Base::Timer releaseShaderCompilerTimer;

TextureRef newTex()
{
	GLuint ref;
	glGenTextures(1, &ref);
	//logMsg("created texture:0x%X", ref);
	return ref;
}

void deleteTex(TextureRef texRef)
{
	//logMsg("deleting texture:0x%X", texRef);
	glcDeleteTextures(1, &texRef);
}

void setActiveTexture(TextureRef texture, uint target)
{
	bool setGLState = useFixedFunctionPipeline;
	if(setGLState && target != GL_TEXTURE_2D)
		bug_exit("cannot manage different texture target state");
	if(texture)
	{
		glcBindTexture(target, texture);
		if(setGLState)
			glcEnable(GL_TEXTURE_2D);
	}
	else
	{
		if(setGLState)
			glcDisable(GL_TEXTURE_2D);
	}
}

void setZTest(bool on)
{
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
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
			glcBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glcEnable(GL_BLEND);
	}
}

void setBlendEquation(uint mode)
{
#if !defined CONFIG_GFX_OPENGL_ES \
	|| (defined CONFIG_BASE_IOS || defined __ANDROID__)
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
			mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
			mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
			GL_FUNC_ADD);
#endif
}

void setImgMode(uint mode)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		switch(mode)
		{
			bcase IMG_MODE_REPLACE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			bcase IMG_MODE_MODULATE: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			bcase IMG_MODE_ADD: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			bcase IMG_MODE_BLEND: glcTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
		}
		return;
	}
	#endif
	// TODO
}

void setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		GLfloat col[4] {r, g, b, a};
		glcTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
		return;
	}
	#endif
	texEnvColor[0] = r;
	texEnvColor[1] = g;
	texEnvColor[2] = b;
	texEnvColor[3] = a;
}

void setZBlend(uchar on)
{
//	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
//	if(useFixedFunctionPipeline)
//	{
//		if(on)
//		{
//			#ifndef CONFIG_GFX_OPENGL_ES
//			glFogi(GL_FOG_MODE, GL_LINEAR);
//			#else
//			glFogf(GL_FOG_MODE, GL_LINEAR);
//			#endif
//			glFogf(GL_FOG_DENSITY, 0.1f);
//			glHint(GL_FOG_HINT, GL_DONT_CARE);
//			glFogf(GL_FOG_START, proj.zRange/2.0);
//			glFogf(GL_FOG_END, proj.zRange);
//			glcEnable(GL_FOG);
//		}
//		else
//		{
//			glcDisable(GL_FOG);
//		}
//	}
//	#endif
	if(!useFixedFunctionPipeline)
	{
		bug_exit("TODO");
	}
}

void setZBlendColor(ColorComp r, ColorComp g, ColorComp b)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		GLfloat c[4] = {r, g, b, 1.0f};
		glFogfv(GL_FOG_COLOR, c);
	}
	#endif
	if(!useFixedFunctionPipeline)
	{
		bug_exit("TODO");
	}
}

void setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		glcColor4f(r, g, b, a);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// !useFixedFunctionPipeline
	if(vColor[0] == r && vColor[1] == g && vColor[2] == b && vColor[3] == a)
		return;
	vColor[0] = r;
	vColor[1] = g;
	vColor[2] = b;
	vColor[3] = a;
	glVertexAttrib4f(VATTR_COLOR, r, g, b, a);
	//logMsg("set color: %f:%f:%f:%f", (double)r, (double)g, (double)b, (double)a);
	#endif
}

uint color()
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		return ColorFormat.build(glState.colorState[0], glState.colorState[1], glState.colorState[2], glState.colorState[3]);
	}
	#endif
	// !useFixedFunctionPipeline
	return ColorFormat.build((float)vColor[0], (float)vColor[1], (float)vColor[2], (float)vColor[3]);
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

void setClipRect(bool on)
{
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void setClipRectBounds(const Base::Window &win, int x, int y, int w, int h)
{
	//logMsg("scissor before transform %d,%d size %d,%d", x, y, w, h);
	// translate from view to window coordinates
	if(!Config::SYSTEM_ROTATES_WINDOWS)
	{
		using namespace Base;
		switch(win.softOrientation())
		{
			bcase VIEW_ROTATE_0:
				//x += win.viewport.rect.x;
				y = win.height() - (y + h);
			bcase VIEW_ROTATE_90:
				//x += win.viewport.rect.y;
				//y = win.width() - (y + h /*+ (win.w - win.viewport.rect.x2)*/);
				std::swap(x, y);
				std::swap(w, h);
				x = (win.realWidth() - x) - w;
				y = (win.realHeight() - y) - h;
			bcase VIEW_ROTATE_270:
				//x += win.viewport.rect.y;
				//y += win.viewport.rect.x;
				std::swap(x, y);
				std::swap(w, h);
			bcase VIEW_ROTATE_180:
				x = (win.realWidth() - x) - w;
				//y = win.height() - (y + h);
				//std::swap(x, y);
				//std::swap(w, h);
				//x += win.viewport.rect.x;
				//y += win.height() - win.viewport.bounds().y2;
		}
	}
	else
	{
		//x += win.viewport.rect.x;
		y = win.height() - (y + h /*+ win.viewport.rect.y*/);
	}
	//logMsg("setting Scissor %d,%d size %d,%d", x, y, w, h);
	glScissor(x, y, w, h);
}

void setClearColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	// TODO: add glClearColor to the state cache
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

void setDither(bool on)
{
	if(on)
		glcEnable(GL_DITHER);
	else
	{
		logMsg("disabling dithering");
		glcDisable(GL_DITHER);
	}
}

bool dither()
{
	return glcIsEnabled(GL_DITHER);
}

void releaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	glReleaseShaderCompiler();
	#endif
}

void autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!releaseShaderCompilerTimer)
	{
		releaseShaderCompilerTimer.callbackAfterMSec(
			[]()
			{
				logMsg("automatically releasing shader compiler");
				glReleaseShaderCompiler();
			}, 1);
	}
	#endif
}

void clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

bool bind()
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current())
	{
		logMsg("restoring context");
		gfxContext.setCurrent(gfxContext, currWin);
		return true;
	}
	return false;
}

bool setCurrentWindow(Base::Window *win)
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current())
	{
		logMsg("restoring context");
		currWin = win;
		gfxContext.setCurrent(gfxContext, win);
		return true;
	}
	else
	{
		if(win == currWin)
			return false;
		currWin = win;
		gfxContext.setDrawable(win, gfxContext);
		return true;
	}
}

bool updateCurrentWindow(Base::Window &win, Base::Window::DrawParams params, Viewport viewport, Mat4 projMat)
{
	if(setCurrentWindow(&win) || params.wasResized())
	{
		setViewport(viewport);
		setProjectionMatrix(projMat);
		return true;
	}
	return false;
}

void presentWindow(Base::Window &win)
{
	gfxContext.present(win, gfxContext);
}

void setCorrectnessChecks(bool on)
{
	if(on)
	{
		logWarn("enabling verification of OpenGL state");
	}
	GLStateCache::verifyState = on;
	checkGLErrors = on ? true : Config::DEBUG_BUILD;
	checkGLErrorsVerbose = on;
}

}
