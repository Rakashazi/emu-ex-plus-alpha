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
#include <imagine/gfx/RendererTask.hh>
#include <imagine/logger/logger.h>
#include "private.hh"

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

#ifndef GL_TIMEOUT_IGNORED
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#endif

namespace Gfx
{

RendererCommands::RendererCommands(RendererDrawTask &rTask, Drawable drawable):
	rTask{&rTask}, r{&rTask.renderer()}, drawable{drawable}
{
	assumeExpr(drawable);
	rTask.setCurrentDrawable(drawable);
	if(Config::DEBUG_BUILD && defaultToFullErrorChecks)
	{
		setDebugOutput(true);
	}
}

RendererCommands::RendererCommands(RendererCommands &&o)
{
	*this = std::move(o);
}

RendererCommands &RendererCommands::operator=(RendererCommands &&o)
{
	RendererCommandsImpl::operator=(o);
	rTask = std::exchange(o.rTask, {});
	r = std::exchange(o.r, {});
	drawable = std::exchange(o.drawable, {});
	return *this;
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

void RendererCommands::bindTempVertexBuffer()
{
	if(renderer().support.hasVBOFuncs)
	{
		bindGLArrayBuffer(rTask->getVBO());
	}
}

void RendererCommands::present()
{
	rTask->notifySemaphore();
	rTask->verifyCurrentContext();
	if(Config::envIsAndroid && renderer().support.hasSamplerObjects)
	{
		// reset sampler object at the end of frame, fixes blank screen
		// on Android SDK emulator when using mipmaps
		renderer().support.glBindSampler(0, 0);
	}
	discardTemporaryData();
	rTask->present(drawable);
}

SyncFence RendererCommands::addSyncFence()
{
	if(!renderer().useSeparateDrawContext)
		return {}; // no-op
	assumeExpr(renderer().support.hasSyncFences());
	rTask->verifyCurrentContext();
	return renderer().support.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

SyncFence RendererCommands::replaceSyncFence(SyncFence fence)
{
	deleteSyncFence(fence);
	return addSyncFence();
}

void RendererCommands::deleteSyncFence(SyncFence fence)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	assert(renderer().support.hasSyncFences());
	renderer().support.glDeleteSync(fence.sync);
}

void RendererCommands::clientWaitSync(SyncFence fence, uint64_t timeout)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	assert(renderer().support.hasSyncFences());
	renderer().support.glClientWaitSync(fence.sync, 0, timeout);
	renderer().support.glDeleteSync(fence.sync);
}

void RendererCommands::waitSync(SyncFence fence)
{
	if(!fence.sync)
		return;
	rTask->verifyCurrentContext();
	assert(renderer().support.hasSyncFences());
	renderer().support.glWaitSync(fence.sync, 0, GL_TIMEOUT_IGNORED);
	renderer().support.glDeleteSync(fence.sync);
}

void RendererCommands::setDrawable(Drawable drawable)
{
	rTask->verifyCurrentContext();
	rTask->setCurrentDrawable(drawable);
	this->drawable = drawable;
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
	glBindFramebuffer(GL_FRAMEBUFFER, rTask->defaultFramebuffer());
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

void RendererCommands::setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
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

std::array<ColorComp, 4> RendererCommands::color() const
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
	if(unlikely(!currSampler))
	{
		logErr("set texture without setting a sampler first");
		return;
	}
	t.bindTex(*this, *currSampler);
}

void RendererCommands::setTextureSampler(const TextureSampler &sampler)
{
	rTask->verifyCurrentContext();
	if(renderer().support.hasSamplerObjects && (!currSampler || currSampler->name() != sampler.name()))
	{
		//logMsg("binding sampler object:0x%X (%s)", (int)sampler.name(), sampler.label());
		renderer().support.glBindSampler(0, sampler.name());
	}
	currSampler = &sampler;
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

Viewport RendererCommands::viewport()
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

void RendererCommands::setProgram(Program &program)
{
	setProgram(program, modelMat);
}

void RendererCommands::setProgram(Program &program, Mat4 modelMat)
{
	rTask->verifyCurrentContext();
	if(currProgram != &program)
	{
		glUseProgram(program.program());
		currProgram = &program;
	}
	loadTransform(modelMat);
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
	switch(program)
	{
		bcase CommonProgram::TEX_REPLACE: renderer().texReplaceProgram.use(*this, modelMat);
		bcase CommonProgram::TEX_ALPHA_REPLACE: renderer().texAlphaReplaceProgram.use(*this, modelMat);
		#ifdef __ANDROID__
		bcase CommonProgram::TEX_EXTERNAL_REPLACE: renderer().texExternalProgram.use(*this, modelMat);
		#endif
		bcase CommonProgram::TEX: renderer().texProgram.use(*this, modelMat);
		bcase CommonProgram::TEX_ALPHA: renderer().texAlphaProgram.use(*this, modelMat);
		#ifdef __ANDROID__
		bcase CommonProgram::TEX_EXTERNAL: renderer().texExternalProgram.use(*this, modelMat);
		#endif
		bcase CommonProgram::NO_TEX: renderer().noTexProgram.use(*this, modelMat);
		bdefault: bug_unreachable("program:%d", (int)program);
	}
}

void RendererCommands::uniformF(int uniformLocation, float v1, float v2)
{
	glUniform2f(uniformLocation, v1, v2);
}

void RendererCommands::setDebugOutput(bool on)
{
	if constexpr(!Config::DEBUG_BUILD)
		return;
	if(!renderer().support.hasDebugOutput || renderer().drawContextDebug == on)
	{
		return;
	}
	logMsg("draw context debug output:%s", on ? "on" : "off");
	setGLDebugOutput(renderer().support, on);
	renderer().drawContextDebug = on;
}

}
