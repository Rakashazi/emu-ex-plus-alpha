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
#include <imagine/util/bits.h>
#include <imagine/util/rectangle2.h>
#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Viewport.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/base/Window.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/time/Time.hh>
#include <array>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRenderer.hh>
#endif

class GfxImageSource;

namespace Gfx
{

class Renderer;
class RendererCommands;

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

class Program : public ProgramImpl
{
public:
	constexpr Program() {}
	bool init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit(Renderer &r);
	bool link(Renderer &r);
	int uniformLocation(Renderer &r, const char *uniformName);
};

class SyncFence : public SyncFenceImpl
{
public:
	using SyncFenceImpl::SyncFenceImpl;
	explicit operator bool() const;
};

class DrawableHolder : public DrawableHolderImpl
{
public:
	constexpr DrawableHolder() {}
};

class RendererTask : public RendererTaskImpl
{
public:
	enum class AsyncMode
	{
		NONE, PRESENT, FULL
	};

	enum class FenceMode
	{
		NONE, RESOURCE
	};

	class DrawParams
	{
	public:
		constexpr DrawParams() {}

		void setAsyncMode(AsyncMode mode)
		{
			asyncMode_ = mode;
		}

		AsyncMode asyncMode() const { return asyncMode_; }

		void setFenceMode(FenceMode mode)
		{
			fenceMode_ = mode;
		}

		FenceMode fenceMode() const { return fenceMode_; }

	private:
		AsyncMode asyncMode_ = AsyncMode::PRESENT;
		FenceMode fenceMode_ = FenceMode::RESOURCE;
	};

	RendererTask(Renderer &r);
	void start();
	void stop();
	void draw(DrawableHolder &drawable, Base::Window &win, Base::Window::DrawParams winParams, DrawParams params, DrawDelegate del);
	#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
	void lockDraw();
	void unlockDraw();
	#endif
	void waitForDrawFinished();
	void run(RenderTaskFuncDelegate func, IG::Semaphore *semAddr = nullptr);
	void runSync(RenderTaskFuncDelegate func);
	void acquireFenceAndWait(Gfx::SyncFence &fenceVar);
	bool addOnDrawFinished(DrawFinishedDelegate del, int priority = 0);
	bool removeOnDrawFinished(DrawFinishedDelegate del);
	void updateDrawableForSurfaceChange(DrawableHolder &drawable, Base::Window::SurfaceChange change);
	void destroyDrawable(DrawableHolder &drawable);
	IG::FrameTime lastDrawTimestamp() const;
	constexpr Renderer &renderer() const { return r; }

private:
	Renderer &r;
};

class RendererDrawTask : public RendererDrawTaskImpl
{
public:
	using RendererDrawTaskImpl::RendererDrawTaskImpl;

	RendererCommands makeRendererCommands(Drawable drawable, Viewport viewport, Mat4 projMat);
	void verifyCurrentContext() const;
	void notifyCommandsFinished();
	Renderer &renderer() const;
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
	void setTexture(Texture &t);
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

class Renderer : public RendererImpl
{
public:
	enum class ThreadMode
	{
		AUTO = 0, SINGLE, MULTI
	};

	// color replacement shaders
	DefaultTexReplaceProgram texReplaceProgram{};
	DefaultTexAlphaReplaceProgram texAlphaReplaceProgram{};
	#ifdef __ANDROID__
	DefaultTexExternalReplaceProgram texExternalReplaceProgram{};
	#endif

	// color modulation shaders
	DefaultTexProgram texProgram{};
	DefaultTexAlphaProgram texAlphaProgram{};
	#ifdef __ANDROID__
	DefaultTexExternalProgram texExternalProgram{};
	#endif
	DefaultColorProgram noTexProgram{};

	Renderer();
	Renderer(Error &err);
	Renderer(IG::PixelFormat pixelFormat, Error &err);
	static Renderer makeConfiguredRenderer(Error &err);
	static Renderer makeConfiguredRenderer(ThreadMode threadMode, Error &err);
	static Renderer makeConfiguredRenderer(ThreadMode threadMode, IG::PixelFormat pixelFormat, Error &err);
	void configureRenderer(ThreadMode threadMode);
	bool isConfigured() const;
	Base::WindowConfig addWindowConfig(Base::WindowConfig config);
	ThreadMode threadMode() const;
	bool supportsThreadMode() const;
	void flush();
	void initWindow(Base::Window &win, Base::WindowConfig config);
	void setWindowValidOrientations(Base::Window &win, Base::Orientation validO);
	void setProjectionMatrixRotation(Angle angle);
	void animateProjectionMatrixRotation(Angle srcAngle, Angle destAngle);
	static ClipRect makeClipRect(const Base::Window &win, IG::WindowRect rect);

	// shaders

	Shader makeShader(const char **src, uint32_t srcCount, uint32_t type);
	Shader makeShader(const char *src, uint32_t type);
	Shader makeCompatShader(const char **src, uint32_t srcCount, uint32_t type);
	Shader makeCompatShader(const char *src, uint32_t type);
	Shader makeDefaultVShader();
	void deleteShader(Shader shader);
	void uniformF(Program &program, int uniformLocation, float v1, float v2);
	void releaseShaderCompiler();
	void autoReleaseShaderCompiler();

	// resources

	Texture makeTexture(TextureConfig config);
	Texture makeTexture(GfxImageSource &img, bool makeMipmaps);
	Texture makeTexture(GfxImageSource &img)
	{
		return makeTexture(img, true);
	}
	PixmapTexture makePixmapTexture(TextureConfig config);
	PixmapTexture makePixmapTexture(GfxImageSource &img, bool makeMipmaps);
	PixmapTexture makePixmapTexture(GfxImageSource &img)
	{
		return makePixmapTexture(img, true);
	}
	TextureSampler makeTextureSampler(TextureSamplerConfig config);
	void makeCommonTextureSampler(CommonTextureSampler sampler);

	void setCorrectnessChecks(bool on);
	void setDebugOutput(bool on);

	// synchronization
	SyncFence addResourceSyncFence();
	SyncFence addSyncFence();
	void deleteSyncFence(SyncFence);
	void clientWaitSync(SyncFence fence, uint64_t timeoutNS = SyncFence::IGNORE_TIMEOUT);
	void waitSync(SyncFence fence);
	void waitAsyncCommands();
};

}
