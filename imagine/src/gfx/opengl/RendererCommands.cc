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
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/TextureSampler.hh>
#include <imagine/gfx/GeomQuad.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/base/Viewport.hh>
#include <imagine/logger/logger.h>
#include "internalDefs.hh"
#include "utils.hh"

namespace IG::Gfx
{

static constexpr bool useGLCache = true;

GLRendererCommands::GLRendererCommands(RendererTask &rTask, Window *winPtr, Drawable drawable,
	Rect2<int> viewport, GLDisplay glDpy, const GLContext &glCtx, std::binary_semaphore *drawCompleteSemPtr):
	rTask{&rTask}, r{&rTask.renderer()}, drawCompleteSemPtr{drawCompleteSemPtr},
	winPtr{winPtr}, glDpy{glDpy}, glContextPtr{&glCtx}, drawable{drawable},
	winViewport{viewport}
{
	assumeExpr(drawable);
	if(setCurrentDrawable(drawable) && viewport.x2)
	{
		setViewport(viewport);
	}
}

void GLRendererCommands::bindGLArrayBuffer(GLuint vbo)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

bool GLRendererCommands::setCurrentDrawable(Drawable drawable)
{
	auto &glCtx = glContext();
	assert(glCtx);
	assert(GLManager::currentContext() == glCtx);
	if(!GLManager::hasCurrentDrawable(drawable))
	{
		glCtx.setCurrentDrawable(drawable);
		return true;
	}
	return false;
}

void GLRendererCommands::present(Drawable win)
{
	auto swapTime = IG::timeFuncDebug(
		[&]()
		{
			glContext().present(win);
		});
	// check if buffer swap blocks even though triple-buffering is used
	if(Config::DEBUG_BUILD && winPtr && r->maxSwapChainImages() > 2 && swapTime > winPtr->screen()->frameTime())
	{
		logWarn("buffer swap took %lldns", (long long)swapTime.count());
	}
}

void GLRendererCommands::doPresent()
{
	rTask->verifyCurrentContext();
	present(drawable);
	notifyPresentComplete();
}

void GLRendererCommands::notifyDrawComplete()
{
	if(drawCompleteSemPtr)
	{
		drawCompleteSemPtr->release();
	}
}

void GLRendererCommands::notifyPresentComplete()
{
	if(winPtr)
	{
		winPtr->postFrameReadyToMainThread();
	}
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

void RendererCommands::present(SteadyClockTimePoint t)
{
	if constexpr(Config::GL_PLATFORM_EGL)
		r->glManager.setPresentationTime(drawable, t);
	present();
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

void RendererCommands::setBlendMode(BlendMode mode)
{
	if(mode == BlendMode::OFF)
	{
		setBlend(false);
		return;
	}
	auto [srcFunc, destFunc] = [&]() -> std::pair<BlendFunc, BlendFunc>
	{
		switch(mode)
		{
			case BlendMode::ALPHA: return {BlendFunc::SRC_ALPHA, BlendFunc::ONE_MINUS_SRC_ALPHA};
			case BlendMode::PREMULT_ALPHA: return{BlendFunc::ONE, BlendFunc::ONE_MINUS_SRC_ALPHA};
			case BlendMode::INTENSITY: return{BlendFunc::SRC_ALPHA, BlendFunc::ONE};
			case BlendMode::OFF: break;
		}
		bug_unreachable("invalid blend mode:%d", std::to_underlying(mode));
	}();
	setBlendFunc(srcFunc, destFunc);
	setBlend(true);
}

void RendererCommands::setBlendEquation(BlendEquation mode)
{
	rTask->verifyCurrentContext();
	auto glMode = [&]()
	{
		switch(mode)
		{
			case BlendEquation::ADD: return GL_FUNC_ADD;
			case BlendEquation::SUB: return GL_FUNC_SUBTRACT;
			case BlendEquation::RSUB: return GL_FUNC_REVERSE_SUBTRACT;
		}
		bug_unreachable("invalid BlendEquation:%d", std::to_underlying(mode));
	}();
	glcBlendEquation(glMode);
}

void RendererCommands::setZTest(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
}

void RendererCommands::clear()
{
	rTask->verifyCurrentContext();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RendererCommands::setClearColor(Color4F c)
{
	rTask->verifyCurrentContext();
	//logMsg("setting clear color %f %f %f %f", (float)r, (float)g, (float)b, (float)a);
	glClearColor(c.r, c.g, c.b, c.a);
}

void RendererCommands::setColor(Color4F c)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		glcColor4f(c.r, c.g, c.b, c.a);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	// !support.useFixedFunctionPipeline
	if(vColor == c)
		return;
	vColor = c;
	glVertexAttrib4f(VATTR_COLOR, c.r, c.g, c.b, c.a);
	//logMsg("set color: %f:%f:%f:%f", (double)r, (double)g, (double)b, (double)a);
	#endif
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

void RendererCommands::setImgMode(EnvMode mode)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		switch(mode)
		{
			case EnvMode::REPLACE: return glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			case EnvMode::MODULATE: return glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			case EnvMode::ADD: return glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
			case EnvMode::BLEND: return glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
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
		glEnable(GL_DITHER);
	else
	{
		//logMsg("disabling dithering");
		glDisable(GL_DITHER);
	}
}

void RendererCommands::setSrgbFramebufferWrite(bool on)
{
	rTask->verifyCurrentContext();
	if(on)
		glEnable(GL_FRAMEBUFFER_SRGB);
	else
		glDisable(GL_FRAMEBUFFER_SRGB);
}

void RendererCommands::setVisibleGeomFace(Faces sides)
{
	rTask->verifyCurrentContext();
	if(sides == Faces::BOTH)
	{
		glDisable(GL_CULL_FACE);
	}
	else if(sides == Faces::FRONT)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // our order is reversed from OpenGL
	}
	else
	{
		glEnable(GL_CULL_FACE);
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
	set(t.binding());
}

void RendererCommands::set(TextureBinding binding)
{
	rTask->verifyCurrentContext();
	if(!binding.name) [[unlikely]]
	{
		logWarn("binding default texture");
	}
	glBindTexture(binding.target, binding.name);
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

void GLRendererCommands::setViewport(Rect2<int> v)
{
	rTask->verifyCurrentContext();
	//logMsg("set GL viewport %d:%d:%d:%d", v.x, v.y, v.x2, v.y2);
	assert(v.x2 && v.y2);
	glViewport(v.x, v.y, v.x2, v.y2);
}

void RendererCommands::setViewport(Viewport v)
{
	GLRendererCommands::setViewport(v.asYUpRelRect());
}

void RendererCommands::restoreViewport()
{
	GLRendererCommands::setViewport(winViewport);
}

void RendererCommands::vertexBufferData(const void *v, size_t size)
{
	if(renderer().support.hasVBOFuncs)
	{
		glBufferData(GL_ARRAY_BUFFER, size, v, GL_STREAM_DRAW);
	}
}

constexpr GLenum asGLType(AttribType type)
{
	switch(type)
	{
		case AttribType::UByte: return GL_UNSIGNED_BYTE;
		case AttribType::Short: return GL_SHORT;
		case AttribType::UShort: return GL_UNSIGNED_SHORT;
		case AttribType::Float: return GL_FLOAT;
	}
	bug_unreachable("invalid AttribType");
}

constexpr bool shouldNormalize(AttribType type, bool normalize) { return type != AttribType::Float && normalize; }

void RendererCommands::drawPrimitives(Primitive mode, int start, int count)
{
	runGLCheckedVerbose([&]()
	{
		glDrawArrays((GLenum)mode, start, count);
	}, "glDrawArrays()");
}

void RendererCommands::drawPrimitiveElements(Primitive mode, std::span<const VertexIndex> idxs)
{
	runGLCheckedVerbose([&]()
	{
		glDrawElements((GLenum)mode, idxs.size(), asGLType(attribType<VertexIndex>), idxs.data());
	}, "glDrawElements()");
}

void RendererCommands::drawRect(WRect bounds)
{
	IQuad::draw(*this, bounds.as<int16_t>());
}

bool GLRendererCommands::hasVBOFuncs() const { return r->support.hasVBOFuncs; }

bool GLRendererCommands::useFixedFunctionPipeline() const { return r->support.useFixedFunctionPipeline; }

#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
void GLRendererCommands::setupVertexArrayPointers(const char *v, int stride,
	AttribDesc textureAttrib, AttribDesc colorAttrib, AttribDesc posAttrib)
{
	if(hasVBOFuncs() && v) // turn off VBO when rendering from memory
	{
		//logMsg("un-binding VBO");
		bindGLArrayBuffer(0);
	}
	glcEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(posAttrib.size, asGLType(posAttrib.type), stride, v + posAttrib.offset);
	if(textureAttrib.size)
	{
		glcEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(textureAttrib.size, asGLType(textureAttrib.type), stride, v + textureAttrib.offset);
	}
	else
		glcDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(colorAttrib.size)
	{
		glcEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(colorAttrib.size, asGLType(colorAttrib.type), stride, v + colorAttrib.offset);
		glState.colorState[0] = -1; //invalidate glColor state cache
	}
	else
		glcDisableClientState(GL_COLOR_ARRAY);
}
#endif

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void GLRendererCommands::setupShaderVertexArrayPointers(const char *v, int stride, VertexLayoutFlags enabledLayout,
	AttribDesc textureAttrib, AttribDesc colorAttrib, AttribDesc posAttrib)
{
	if(currentEnabledVertexLayout != enabledLayout)
	{
		if(textureAttrib.size)
			glEnableVertexAttribArray(VATTR_TEX_UV);
		else
			glDisableVertexAttribArray(VATTR_TEX_UV);
		if(colorAttrib.size)
			glEnableVertexAttribArray(VATTR_COLOR);
		else
			glDisableVertexAttribArray(VATTR_COLOR);
		currentEnabledVertexLayout = enabledLayout;
	}
	glVertexAttribPointer(VATTR_POS, posAttrib.size, asGLType(posAttrib.type),
		posAttrib.normalize, stride, v + posAttrib.offset);
	if(textureAttrib.size)
	{
		glVertexAttribPointer(VATTR_TEX_UV, textureAttrib.size, asGLType(textureAttrib.type),
			textureAttrib.normalize, stride, v + textureAttrib.offset);
	}
	if(colorAttrib.size)
	{
		glVertexAttribPointer(VATTR_COLOR, colorAttrib.size, asGLType(colorAttrib.type),
			colorAttrib.normalize, stride, v + colorAttrib.offset);
	}
}
#endif

// shaders

void RendererCommands::setProgram(NativeProgram program)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	rTask->verifyCurrentContext();
	if(currProgram != program)
	{
		glUseProgram(program);
		currProgram = program;
	}
	#endif
}

void RendererCommands::setProgram(const Program &program)
{
	setProgram(program.glProgram());
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void RendererCommands::uniform(int loc, float v1){ glUniform1f(loc, v1); }
void RendererCommands::uniform(int loc, float v1, float v2){ glUniform2f(loc, v1, v2); }
void RendererCommands::uniform(int loc, float v1, float v2, float v3){ glUniform3f(loc, v1, v2, v3); }
void RendererCommands::uniform(int loc, float v1, float v2, float v3, float v4){ glUniform4f(loc, v1, v2, v3, v4); }
void RendererCommands::uniform(int loc, int v1){ glUniform1i(loc, v1); }
void RendererCommands::uniform(int loc, int v1, int v2){ glUniform2i(loc, v1, v2); }
void RendererCommands::uniform(int loc, int v1, int v2, int v3){ glUniform3i(loc, v1, v2, v3); }
void RendererCommands::uniform(int loc, int v1, int v2, int v3, int v4){ glUniform4i(loc, v1, v2, v3, v4); }

void RendererCommands::uniform(int loc, Mat4 mat)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}
#endif

BasicEffect &RendererCommands::basicEffect() { return renderer().basicEffect(); }

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

const GLContext &GLRendererCommands::glContext() const
{
	return *glContextPtr;
}

}
