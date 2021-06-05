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

#define LOGTAG "RendererCmds"
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Program.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"
#include "utils.hh"

namespace Gfx
{

static constexpr bool useGLCache = true;

GLRendererCommands::GLRendererCommands(RendererTask &rTask, Base::Window *winPtr, Drawable drawable,
	Base::GLDisplay glDpy, const Base::GLContext &glCtx, IG::Semaphore *drawCompleteSemPtr):
	rTask{&rTask}, r{&rTask.renderer()}, drawCompleteSemPtr{drawCompleteSemPtr},
	winPtr{winPtr}, glDpy{glDpy}, glContextPtr{&glCtx}, drawable{drawable}
{
	assumeExpr(drawable);
	setCurrentDrawable(drawable);
}

void GLRendererCommands::discardTemporaryData() {}

void GLRendererCommands::bindGLArrayBuffer(GLuint vbo)
{
	if(arrayBufferIsSet && vbo == arrayBuffer)
		return;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	arrayBuffer = vbo;
	arrayBufferIsSet = true;
}

void GLRendererCommands::setCurrentDrawable(Drawable drawable)
{
	auto &glCtx = glContext();
	assert(glCtx);
	assert(Base::GLManager::currentContext() == glCtx);
	if(!Base::GLManager::hasCurrentDrawable(drawable))
	{
		glCtx.setCurrentDrawable(drawable);
	}
}

void GLRendererCommands::present(Drawable win)
{
	auto swapTime = IG::timeFuncDebug(
		[&]()
		{
			glContext().present(win);
		});
	// check if buffer swap blocks even though triple-buffering is used
	if(winPtr && r->maxSwapChainImages() > 2 && swapTime > winPtr->screen()->frameTime())
	{
		logWarn("buffer swap took %lldns", (long long)swapTime.count());
	}
}

void GLRendererCommands::doPresent()
{
	rTask->verifyCurrentContext();
	if(Config::envIsAndroid && r->support.hasSamplerObjects)
	{
		// reset sampler object at the end of frame, fixes blank screen
		// on Android SDK emulator when using mipmaps
		r->support.glBindSampler(0, 0);
	}
	discardTemporaryData();
	present(drawable);
	notifyPresentComplete();
}

void GLRendererCommands::notifyDrawComplete()
{
	if(drawCompleteSemPtr)
	{
		drawCompleteSemPtr->notify();
	}
}

void GLRendererCommands::notifyPresentComplete()
{
	if(winPtr)
	{
		winPtr->postFrameReadyToMainThread();
	}
}

void GLRendererCommands::setCachedProjectionMatrix(Mat4 mat)
{
	projectionMat = mat;
}

void RendererCommands::bindTempVertexBuffer()
{
	if(renderer().support.hasVBOFuncs)
	{
		bindGLArrayBuffer(rTask->getVBO());
	}
}

void RendererCommands::flush()
{
	rTask->verifyCurrentContext();
	glFlush();
}

void RendererCommands::present()
{
	notifyDrawComplete();
	doPresent();
}

SyncFence RendererCommands::addSyncFence()
{
	if(!renderer().support.hasSyncFences())
		return {}; // no-op
	rTask->verifyCurrentContext();
	return renderer().support.fenceSync(glDpy);
}

void RendererCommands::deleteSyncFence(SyncFence fence)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	assert(renderer().support.hasSyncFences());
	renderer().support.deleteSync(glDpy, fence.sync);
}

void RendererCommands::clientWaitSync(SyncFence fence, int flags, std::chrono::nanoseconds timeout)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	auto &r = renderer();
	assert(r.support.hasSyncFences());
	//logDMsg("waiting on sync:%p flush:%s timeout:0%llX", fence.sync, flags & 1 ? "yes" : "no", (unsigned long long)timeout);
	r.support.clientWaitSync(glDpy, fence.sync, flags, timeout.count());
	r.support.deleteSync(glDpy, fence.sync);
}

SyncFence RendererCommands::clientWaitSyncReset(SyncFence oldFence, int flags, std::chrono::nanoseconds timeout)
{
	rTask->verifyCurrentContext();
	clientWaitSync(oldFence, flags, timeout);
	return addSyncFence();
}

void RendererCommands::waitSync(SyncFence fence)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	auto &r = renderer();
	assert(r.support.hasSyncFences());
	r.support.waitSync(glDpy, fence.sync);
	r.support.deleteSync(glDpy, fence.sync);
}

void RendererCommands::setRenderTarget(Texture &texture)
{
	rTask->verifyCurrentContext();
	auto id = rTask->bindFramebuffer(texture);
	if(Config::DEBUG_BUILD && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		logErr("FBO:0x%X incomplete", id);
	}
}

void RendererCommands::setDefaultRenderTarget()
{
	rTask->verifyCurrentContext();
	glBindFramebuffer(GL_FRAMEBUFFER, rTask->defaultFBO());
}

Renderer &RendererCommands::renderer() const
{
	return *r;
}

void RendererCommands::setBlend(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
		glcEnable(GL_BLEND);
	else
		glcDisable(GL_BLEND);
}

void RendererCommands::setBlendFunc(BlendFunc s, BlendFunc d)
{
	rTask->verifyCurrentContext();
	glcBlendFunc((GLenum)s, (GLenum)d);
}

void RendererCommands::setBlendMode(uint32_t mode)
{
	rTask->verifyCurrentContext();
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

void RendererCommands::setBlendEquation(uint32_t mode)
{
	rTask->verifyCurrentContext();
	#if !defined CONFIG_GFX_OPENGL_ES \
		|| (defined CONFIG_BASE_IOS || defined __ANDROID__)
	glcBlendEquation(mode == BLEND_EQ_ADD ? GL_FUNC_ADD :
		mode == BLEND_EQ_SUB ? GL_FUNC_SUBTRACT :
		mode == BLEND_EQ_RSUB ? GL_FUNC_REVERSE_SUBTRACT :
		GL_FUNC_ADD);
	#endif
}

void RendererCommands::setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
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

void RendererCommands::setZTest(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
	{
		glcEnable(GL_DEPTH_TEST);
	}
	else
	{
		glcDisable(GL_DEPTH_TEST);
	}
}

void RendererCommands::setZBlend(bool on)
{
	rTask->verifyCurrentContext();
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
	if(!renderer().support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
}

void RendererCommands::setZBlendColor(ColorComp r, ColorComp g, ColorComp b)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		GLfloat c[4] = {r, g, b, 1.0f};
		glFogfv(GL_FOG_COLOR, c);
	}
	#endif
	if(!renderer().support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
}

void RendererCommands::clear()
{
	rTask->verifyCurrentContext();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RendererCommands::setClearColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	rTask->verifyCurrentContext();
	//GLfloat c[4] = {r, g, b, a};
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	glClearColor(r, g, b, a);
}

void RendererCommands::setColor(Color c)
{
	rTask->verifyCurrentContext();
	auto [r, g, b, a] = c;
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		glcColor4f(r, g, b, a);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// !support.useFixedFunctionPipeline
	if(vColor == c)
		return;
	vColor = c;
	glVertexAttrib4f(VATTR_COLOR, r, g, b, a);
	//logMsg("set color: %f:%f:%f:%f", (double)r, (double)g, (double)b, (double)a);
	#endif
}

void RendererCommands::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	setColor({r, g, b, a});
}

Color RendererCommands::color() const
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		return glState.colorState;
	}
	#endif
	// !support.useFixedFunctionPipeline
	return vColor;
}

void RendererCommands::setImgMode(uint32_t mode)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
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

void RendererCommands::setDither(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
		glcEnable(GL_DITHER);
	else
	{
		//logMsg("disabling dithering");
		glcDisable(GL_DITHER);
	}
}

bool RendererCommands::dither()
{
	rTask->verifyCurrentContext();
	return glcIsEnabled(GL_DITHER);
}

void RendererCommands::setSrgbFramebufferWrite(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
		glEnable(GL_FRAMEBUFFER_SRGB);
	else
		glDisable(GL_FRAMEBUFFER_SRGB);
}

void RendererCommands::setVisibleGeomFace(uint32_t sides)
{
	rTask->verifyCurrentContext();
	if(sides == BOTH_FACES)
	{
		glcDisable(GL_CULL_FACE);
	}
	else if(sides == FRONT_FACES)
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

void RendererCommands::setClipTest(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
		glcEnable(GL_SCISSOR_TEST);
	else
		glcDisable(GL_SCISSOR_TEST);
}

void RendererCommands::setClipRect(ClipRect r)
{
	rTask->verifyCurrentContext();
	//logMsg("setting Scissor %d,%d size %d,%d", r.x, r.y, r.x2, r.y2);
	glScissor(r.x, r.y, r.x2, r.y2);
}

void RendererCommands::setTexture(const Texture &t)
{
	rTask->verifyCurrentContext();
	if(renderer().support.hasSamplerObjects && !currSamplerName)
	{
		logWarn("set texture without setting a sampler first");
	}
	t.bindTex(*this);
}

void RendererCommands::setTextureSampler(const TextureSampler &sampler)
{
	if(!renderer().support.hasSamplerObjects)
		return;
	rTask->verifyCurrentContext();
	if(currSamplerName != sampler.name())
	{
		//logMsg("binding sampler object:0x%X (%s)", (int)sampler.name(), sampler.label());
		renderer().support.glBindSampler(0, sampler.name());
	}
	currSamplerName = sampler.name();
}

void RendererCommands::setCommonTextureSampler(CommonTextureSampler sampler)
{
	auto &samplerObj = renderer().commonTextureSampler(sampler);
	assert(samplerObj);
	setTextureSampler(samplerObj);
}

void RendererCommands::setViewport(Viewport v)
{
	rTask->verifyCurrentContext();
	auto inGLFormat = v.inGLFormat();
	//logMsg("set GL viewport %d:%d:%d:%d", inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	assert(inGLFormat.x2 && inGLFormat.y2);
	glViewport(inGLFormat.x, inGLFormat.y, inGLFormat.x2, inGLFormat.y2);
	currViewport = v;
}

Viewport RendererCommands::viewport() const
{
	return currViewport;
}

void RendererCommands::vertexBufferData(const void *v, uint32_t size)
{
	if(renderer().support.hasVBOFuncs)
	{
		glBufferData(GL_ARRAY_BUFFER, size, v, GL_STREAM_DRAW);
	}
}

void RendererCommands::drawPrimitives(Primitive mode, uint32_t start, uint32_t count)
{
	runGLCheckedVerbose([&]()
	{
		glDrawArrays((GLenum)mode, start, count);
	}, "glDrawArrays()");
}

void RendererCommands::drawPrimitiveElements(Primitive mode, const VertexIndex *idx, uint32_t count)
{
	runGLCheckedVerbose([&]()
	{
		glDrawElements((GLenum)mode, count, GL_UNSIGNED_SHORT, idx);
	}, "glDrawElements()");
}

// shaders

void RendererCommands::setProgram(const Program &program)
{
	setProgram(program, modelMat);
}

void RendererCommands::setProgram(const Program &program, Mat4 modelMat)
{
	rTask->verifyCurrentContext();
	if(currProgram != program)
	{
		glUseProgram(program.glProgram());
		currProgram = program;
	}
	loadTransform(modelMat);
}

void RendererCommands::setProgram(const Program &program, const Mat4 *modelMat)
{
	if(modelMat)
		setProgram(program, *modelMat);
	else
		setProgram(program);
}

void RendererCommands::setCommonProgram(CommonProgram program)
{
	setCommonProgram(program, nullptr);
}

void RendererCommands::setCommonProgram(CommonProgram program, Mat4 modelMat)
{
	setCommonProgram(program, &modelMat);
}

void RendererCommands::setCommonProgram(CommonProgram program, const Mat4 *modelMat)
{
	rTask->verifyCurrentContext();
	renderer().useCommonProgram(*this, program, modelMat);
}

void RendererCommands::uniformF(int uniformLocation, float v1, float v2)
{
	glUniform2f(uniformLocation, v1, v2);
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRendererCommands::glcMatrixMode(GLenum mode)
{ if(useGLCache) glState.matrixMode(mode); else glMatrixMode(mode); }
#endif

void GLRendererCommands::glcBindTexture(GLenum target, GLuint texture)
{ if(useGLCache) glState.bindTexture(target, texture); else glBindTexture(target, texture); }
void GLRendererCommands::glcBlendFunc(GLenum sfactor, GLenum dfactor)
{ if(useGLCache) glState.blendFunc(sfactor, dfactor); else glBlendFunc(sfactor, dfactor); }
void GLRendererCommands::glcBlendEquation(GLenum mode)
{ if(useGLCache) glState.blendEquation(mode); else glBlendEquation(mode); }
void GLRendererCommands::glcEnable(GLenum cap)
{ if(useGLCache) glState.enable(cap); else glEnable(cap); }
void GLRendererCommands::glcDisable(GLenum cap)
{ if(useGLCache) glState.disable(cap); else glDisable(cap); }

GLboolean GLRendererCommands::glcIsEnabled(GLenum cap)
{
	if(useGLCache)
		return glState.isEnabled(cap);
	else
		return glIsEnabled(cap);
}

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRendererCommands::glcEnableClientState(GLenum cap)
{ if(useGLCache) glState.enableClientState(cap); else glEnableClientState(cap); }
void GLRendererCommands::glcDisableClientState(GLenum cap)
{ if(useGLCache) glState.disableClientState(cap); else glDisableClientState(cap); }
void GLRendererCommands::glcTexEnvi(GLenum target, GLenum pname, GLint param)
{ if(useGLCache) glState.texEnvi(target, pname, param); else glTexEnvi(target, pname, param); }
void GLRendererCommands::glcTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{ if(useGLCache) glState.texEnvfv(target, pname, params); else glTexEnvfv(target, pname, params); }
void GLRendererCommands::glcColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	if(useGLCache)
		glState.color4f(red, green, blue, alpha);
	else
	{
		glColor4f(red, green, blue, alpha);
		glState.colorState[0] = red; glState.colorState[1] = green; glState.colorState[2] = blue; glState.colorState[3] = alpha; // for color()
	}
}
#endif

const Base::GLContext &GLRendererCommands::glContext() const
{
	return *glContextPtr;
}

}
