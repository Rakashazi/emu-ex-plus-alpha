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
#include <imagine/gfx/RenderTarget.hh>
#include <imagine/logger/logger.h>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/util/Interpolator.hh>
#include "private.hh"
#include "utils.h"

#ifndef GL_BACK_LEFT
#define GL_BACK_LEFT				0x0402
#endif

#ifndef GL_BACK_RIGHT
#define GL_BACK_RIGHT				0x0403
#endif

namespace Gfx
{

bool checkGLErrors = Config::DEBUG_BUILD;
bool checkGLErrorsVerbose = false;
static constexpr bool multipleContextsPerThread = Config::envIsAndroid;
static constexpr bool useGLCache = true;

void GLRenderer::verifyCurrentContext()
{
	if(!Config::DEBUG_BUILD)
		return;
	auto currentCtx = Base::GLContext::current(glDpy);
	if(unlikely(gfxContext != currentCtx))
	{
		bug_unreachable("expected GL context:%p but current is:%p", gfxContext.nativeObject(), currentCtx.nativeObject());
	}
}

TextureRef GLRenderer::newTex()
{
	GLuint ref;
	glGenTextures(1, &ref);
	//logMsg("created texture:0x%X", ref);
	return ref;
}

void GLRenderer::deleteTex(TextureRef texRef)
{
	//logMsg("deleting texture:0x%X", texRef);
	glcDeleteTextures(1, &texRef);
}

void Renderer::setZTest(bool on)
{
	verifyCurrentContext();
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
	}
}

void Renderer::setBlend(bool on)
{
	verifyCurrentContext();
	if(on)
		glcEnable(GL_BLEND);
	else
		glcDisable(GL_BLEND);
}

void Renderer::setBlendFunc(BlendFunc s, BlendFunc d)
{
	verifyCurrentContext();
	glcBlendFunc((GLenum)s, (GLenum)d);
}

void Renderer::setBlendMode(uint mode)
{
	verifyCurrentContext();
	switch(mode)
	{
		bcase BLEND_MODE_OFF:
			setBlend(false);
		bcase BLEND_MODE_ALPHA:
			setBlendFunc(BlendFunc::SRC_ALPHA, BlendFunc::ONE_MINUS_SRC_ALPHA); // general blending
			//setBlendFunc(BlendFunc::ONE, BlendFunc::ONE_MINUS_SRC_ALPHA); // for premultiplied alpha
			setBlend(true);
		bcase BLEND_MODE_INTENSITY:
			setBlendFunc(BlendFunc::SRC_ALPHA, BlendFunc::ONE);
			setBlend(true);
	}
}

void Renderer::setBlendEquation(uint mode)
{
	verifyCurrentContext();
#if !defined CONFIG_GFX_OPENGL_ES \
	|| (defined CONFIG_BASE_IOS || defined __ANDROID__)
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
			mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
			mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
			GL_FUNC_ADD);
#endif
}

void Renderer::setImgMode(uint mode)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
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

void Renderer::setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
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

void Renderer::setZBlend(bool on)
{
	verifyCurrentContext();
//	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
//	if(support.useFixedFunctionPipeline)
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
	if(!support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
}

void Renderer::setZBlendColor(ColorComp r, ColorComp g, ColorComp b)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		GLfloat c[4] = {r, g, b, 1.0f};
		glFogfv(GL_FOG_COLOR, c);
	}
	#endif
	if(!support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
}

void Renderer::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		glcColor4f(r, g, b, a);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// !support.useFixedFunctionPipeline
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

uint Renderer::color()
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		return ColorFormat.build(glState.colorState[0],
			glState.colorState[1],
			glState.colorState[2],
			glState.colorState[3]);
	}
	#endif
	// !support.useFixedFunctionPipeline
	return ColorFormat.build((float)vColor[0], (float)vColor[1], (float)vColor[2], (float)vColor[3]);
}

void Renderer::setVisibleGeomFace(uint faces)
{
	verifyCurrentContext();
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

void Renderer::setClipRect(bool on)
{
	verifyCurrentContext();
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void Renderer::setClipRectBounds(const Base::Window &win, int x, int y, int w, int h)
{
	verifyCurrentContext();
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

void Renderer::setClearColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	verifyCurrentContext();
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	// TODO: add glClearColor to the state cache
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

void Renderer::setDither(bool on)
{
	verifyCurrentContext();
	if(on)
		glcEnable(GL_DITHER);
	else
	{
		logMsg("disabling dithering");
		glcDisable(GL_DITHER);
	}
}

bool Renderer::dither()
{
	verifyCurrentContext();
	return glcIsEnabled(GL_DITHER);
}

void Renderer::releaseShaderCompiler()
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	glReleaseShaderCompiler();
	#endif
}

void Renderer::autoReleaseShaderCompiler()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!releaseShaderCompilerTimer)
	{
		releaseShaderCompilerTimer.callbackAfterMSec(
			[this]()
			{
				logMsg("automatically releasing shader compiler");
				verifyCurrentContext();
				glReleaseShaderCompiler();
			}, 1, {});
	}
	#endif
}

void GLRenderer::discardTemporaryData()
{
	discardTexturePBO();
}

void Renderer::clear()
{
	verifyCurrentContext();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::bind()
{
	gfxContext.setCurrent(glDpy, gfxContext, currWin);
}

void Renderer::unbind()
{
	gfxContext.setCurrent(glDpy, {}, {});
}

bool Renderer::restoreBind()
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current(glDpy))
	{
		logMsg("restoring context");
		gfxContext.setCurrent(glDpy, gfxContext, currWin);
		return true;
	}
	return false;
}

void Renderer::updateDrawableForSurfaceChange(Drawable &drawable, Base::Window::SurfaceChange change)
{
	if(change.destroyed())
	{
		deinitDrawable(drawable);
	}
	else if(change.reset())
	{
		if(currWin == drawable)
			currWin = {};
	}
}

bool Renderer::setCurrentDrawable(Drawable win)
{
	if(multipleContextsPerThread && gfxContext != Base::GLContext::current(glDpy))
	{
		logMsg("restoring context");
		currWin = win;
		gfxContext.setCurrent(glDpy, gfxContext, win);
		return true;
	}
	else
	{
		if(win == currWin)
			return false;
		gfxContext.setDrawable(glDpy, win, gfxContext);
		if(support.shouldSpecifyDrawReadBuffers && win)
		{
			//logMsg("specifying draw/read buffers");
			verifyCurrentContext();
			const GLenum back = Config::Gfx::OPENGL_ES_MAJOR_VERSION ? GL_BACK : GL_BACK_LEFT;
			support.glDrawBuffers(1, &back);
			support.glReadBuffer(GL_BACK);
		}
		currWin = win;
		return true;
	}
}

bool Renderer::updateCurrentDrawable(Drawable &drawable, Base::Window &win, Base::Window::DrawParams params, Viewport viewport, Mat4 projMat)
{
	if(!drawable)
	{
		std::error_code ec{};
		drawable = glDpy.makeDrawable(win, gfxBufferConfig, ec);
		if(ec)
		{
			logErr("Error creating GL drawable");
		}
	}
	if(setCurrentDrawable(drawable) || params.wasResized())
	{
		setViewport(viewport);
		setProjectionMatrix(projMat);
		if(win == Base::mainWindow())
		{
			if(!Config::SYSTEM_ROTATES_WINDOWS)
			{
				setProjectionMatrixRotation(orientationToGC(win.softOrientation()));
				Base::setOnDeviceOrientationChanged(
					[this, &win](uint newO)
					{
						auto oldWinO = win.softOrientation();
						if(win.requestOrientationChange(newO))
						{
							animateProjectionMatrixRotation(orientationToGC(oldWinO), orientationToGC(newO));
						}
					});
			}
			else if(Config::SYSTEM_ROTATES_WINDOWS && !Base::Window::systemAnimatesRotation())
			{
				Base::setOnSystemOrientationChanged(
					[this](uint oldO, uint newO) // TODO: parameters need proper type definitions in API
					{
						const Angle orientationDiffTable[4][4]
						{
							{0, angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90)},
							{angleFromDegree(-90), 0, angleFromDegree(90), angleFromDegree(-180)},
							{angleFromDegree(-180), angleFromDegree(-90), 0, angleFromDegree(90)},
							{angleFromDegree(90), angleFromDegree(-180), angleFromDegree(-90), 0},
						};
						auto rotAngle = orientationDiffTable[oldO][newO];
						logMsg("animating from %d degrees", (int)angleToDegree(rotAngle));
						animateProjectionMatrixRotation(rotAngle, 0.);
					});
			}
		}
		return true;
	}
	return false;
}

void Renderer::deinitDrawable(Drawable &drawable)
{
	if(currWin == drawable)
		currWin = {};
	glDpy.deleteDrawable(drawable);
}

void Renderer::presentDrawable(Drawable win)
{
	verifyCurrentContext();
	discardTemporaryData();
	gfxContext.present(glDpy, win, gfxContext);
}

void Renderer::finishPresentDrawable(Drawable win)
{
	gfxContext.finishPresent(glDpy, win);
}

void Renderer::finish()
{
	verifyCurrentContext();
	setCurrentDrawable({});
	if(Config::envIsIOS)
	{
		glFinish();
	}
}

void Renderer::setRenderTarget(const RenderTarget &target)
{
	verifyCurrentContext();
	auto id = target.id();
	if(!id) // default frame buffer
	{
		#if defined __APPLE__ && TARGET_OS_IPHONE
		Base::GLContext::setDrawable(glDpy, currWin);
		#else
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		#endif
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, id);
		if(Config::DEBUG_BUILD && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			logErr("FBO:0x%X incomplete", id);
		}
	}
}

void Renderer::setCorrectnessChecks(bool on)
{
	if(on)
	{
		logWarn("enabling verification of OpenGL state");
	}
	GLStateCache::verifyState = on;
	checkGLErrors = on ? true : Config::DEBUG_BUILD;
	checkGLErrorsVerbose = on;
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRenderer::glcMatrixMode(GLenum mode)
{ if(useGLCache) glState.matrixMode(mode); else glMatrixMode(mode); }
#endif

void GLRenderer::glcBindTexture(GLenum target, GLuint texture)
{ if(useGLCache) glState.bindTexture(target, texture); else glBindTexture(target, texture); }
void GLRenderer::glcDeleteTextures(GLsizei n, const GLuint *textures)
{ if(useGLCache) glState.deleteTextures(n, textures); else glDeleteTextures(n, textures); }
void GLRenderer::glcBlendFunc(GLenum sfactor, GLenum dfactor)
{ if(useGLCache) glState.blendFunc(sfactor, dfactor); else glBlendFunc(sfactor, dfactor); }
void GLRenderer::glcBlendEquation(GLenum mode)
{ if(useGLCache) glState.blendEquation(mode); else glBlendEquation(mode); }
void GLRenderer::glcEnable(GLenum cap)
{ if(useGLCache) glState.enable(cap); else glEnable(cap); }
void GLRenderer::glcDisable(GLenum cap)
{ if(useGLCache) glState.disable(cap); else glDisable(cap); }

GLboolean GLRenderer::glcIsEnabled(GLenum cap)
{
	if(useGLCache)
		return glState.isEnabled(cap);
	else
		return glIsEnabled(cap);
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRenderer::glcEnableClientState(GLenum cap)
{ if(useGLCache) glState.enableClientState(cap); else glEnableClientState(cap); }
void GLRenderer::glcDisableClientState(GLenum cap)
{ if(useGLCache) glState.disableClientState(cap); else glDisableClientState(cap); }
void GLRenderer::glcTexEnvi(GLenum target, GLenum pname, GLint param)
{ if(useGLCache) glState.texEnvi(target, pname, param); else glTexEnvi(target, pname, param); }
void GLRenderer::glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{ if(useGLCache) glState.texEnvfv(target, pname, params); else glTexEnvfv(target, pname, params); }
void GLRenderer::glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if(useGLCache)
		glState.color4f(red, green, blue, alpha);
	else
	{
		glColor4f(red, green, blue, alpha);
		glState.colorState[0] = red; glState.colorState[1] = green; glState.colorState[2] = blue; glState.colorState[3] = alpha; // for color()
	}
}
void GLRenderer::glcTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.texCoordPointer(size, type, stride, pointer); else glTexCoordPointer(size, type, stride, pointer); }
void GLRenderer::glcColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.colorPointer(size, type, stride, pointer); else glColorPointer(size, type, stride, pointer); }
void GLRenderer::glcVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.vertexPointer(size, type, stride, pointer); else glVertexPointer(size, type, stride, pointer); }
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void GLRenderer::glcVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{ if(useGLCache) glState.vertexAttribPointer(index, size, type, normalized, stride, pointer); else glVertexAttribPointer(index, size, type, normalized, stride, pointer); }
#endif

void GLRenderer::glcBindBuffer(GLenum target, GLuint buffer)
{ if(useGLCache) glState.bindBuffer(target, buffer); else glBindBuffer(target, buffer); }
void GLRenderer::glcDeleteBuffers(GLsizei n, const GLuint *buffers)
{ if(useGLCache) glState.deleteBuffers(n, buffers); else glDeleteBuffers(n, buffers); }
void GLRenderer::glcPixelStorei(GLenum pname, GLint param)
{ if(useGLCache) glState.pixelStorei(pname, param); else glPixelStorei(pname, param); }


}
