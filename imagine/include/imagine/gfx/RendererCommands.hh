#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/SyncFence.hh>
#include <imagine/gfx/Mat4.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRendererCommands.hh>
#endif

namespace Gfx
{

class Renderer;
class RendererDrawTask;
class Program;
class Texture;

enum class Primitive
{
	TRIANGLE = TRIANGLE_IMPL,
	TRIANGLE_STRIP = TRIANGLE_STRIP_IMPL
};

enum class BlendFunc
{
	ZERO = ZERO_IMPL,
	ONE = ONE_IMPL,
	SRC_COLOR = SRC_COLOR_IMPL,
	ONE_MINUS_SRC_COLOR = ONE_MINUS_SRC_COLOR_IMPL,
	DST_COLOR = DST_COLOR_IMPL,
	ONE_MINUS_DST_COLOR = ONE_MINUS_DST_COLOR_IMPL,
	SRC_ALPHA = SRC_ALPHA_IMPL,
	ONE_MINUS_SRC_ALPHA = ONE_MINUS_SRC_ALPHA_IMPL,
	DST_ALPHA = DST_ALPHA_IMPL,
	ONE_MINUS_DST_ALPHA = ONE_MINUS_DST_ALPHA_IMPL,
	CONSTANT_COLOR = CONSTANT_COLOR_IMPL,
	ONE_MINUS_CONSTANT_COLOR = ONE_MINUS_CONSTANT_COLOR_IMPL,
	CONSTANT_ALPHA = CONSTANT_ALPHA_IMPL,
	ONE_MINUS_CONSTANT_ALPHA = ONE_MINUS_CONSTANT_ALPHA_IMPL,
};

class RendererCommands : public RendererCommandsImpl
{
public:
	RendererCommands(RendererDrawTask &rTaskCtx, Drawable drawable);
	RendererCommands(RendererCommands &&o);
	RendererCommands &operator=(RendererCommands &&o);
	void present();
	void setDrawable(Drawable win);
	void setRenderTarget(Texture &t);
	void setDefaultRenderTarget();
	void setDebugOutput(bool on);
	Renderer &renderer() const;

	// render states

	void setBlend(bool on);
	void setBlendFunc(BlendFunc s, BlendFunc d);
	void setBlendMode(uint32_t mode);
	void setBlendEquation(uint32_t mode);
	void setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a);
	void setZTest(bool on);
	void setZBlend(bool on);
	void setZBlendColor(ColorComp r, ColorComp g, ColorComp b);
	void setClearColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a = 1.);
	void setColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a = 1.);
	void setColor(ColorComp i) { setColor(i, i, i, 1.); }
	void setColor(GfxColorEnum colConst)
	{
		switch(colConst)
		{
			bcase COLOR_WHITE: setColor(1., 1., 1.);
			bcase COLOR_BLACK: setColor(0., 0., 0.);
		}
	}
	std::array<ColorComp, 4> color() const;
	void setImgMode(uint32_t mode);
	void setDither(bool on);
	bool dither();
	void setVisibleGeomFace(uint32_t sides);
	void setClipTest(bool on);
	void setClipRect(ClipRect b);
	void setTexture(const Texture &t);
	void setTextureSampler(const TextureSampler &sampler);
	void setCommonTextureSampler(CommonTextureSampler sampler);
	void setViewport(Viewport v);
	Viewport viewport();
	void vertexBufferData(const void *v, uint32_t size);
	void bindTempVertexBuffer();

	// transforms

	void setTransformTarget(TransformTargetEnum target);
	void loadTransform(Mat4 mat);
	void loadTranslate(GC x, GC y, GC z);
	void loadIdentTransform();
	void setProjectionMatrix(const Mat4 &mat);

	// shaders

	void setProgram(Program &program);
	void setProgram(Program &program, Mat4 modelMat);
	void setCommonProgram(CommonProgram program);
	void setCommonProgram(CommonProgram program, Mat4 modelMat);
	void uniformF(int uniformLocation, float v1, float v2);

	// synchronization
	SyncFence addSyncFence();
	SyncFence replaceSyncFence(SyncFence fence);
	void deleteSyncFence(SyncFence fence);
	void clientWaitSync(SyncFence fence, uint64_t timeoutNS = SyncFence::IGNORE_TIMEOUT);
	void waitSync(SyncFence fence);

	// rendering

	void clear();
	void drawPrimitives(Primitive mode, uint32_t start, uint32_t count);
	void drawPrimitiveElements(Primitive mode, const VertexIndex *idx, uint32_t count);

private:
	RendererDrawTask *rTask{};
	Renderer *r{};
	Drawable drawable{};

	void setCommonProgram(CommonProgram program, const Mat4 *modelMat);
};

}
