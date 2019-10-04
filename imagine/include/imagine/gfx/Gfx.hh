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
#include <imagine/gfx/Vec3.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/base/Base.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <array>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRenderer.hh>
#endif

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

enum { BLEND_MODE_OFF = 0, BLEND_MODE_ALPHA, BLEND_MODE_INTENSITY };

enum { IMG_MODE_MODULATE = 0, IMG_MODE_BLEND, IMG_MODE_REPLACE, IMG_MODE_ADD };

enum { BLEND_EQ_ADD, BLEND_EQ_SUB, BLEND_EQ_RSUB };

enum { BOTH_FACES, FRONT_FACES, BACK_FACES };

enum GfxColorEnum { COLOR_WHITE, COLOR_BLACK };

enum TransformTargetEnum { TARGET_WORLD, TARGET_TEXTURE };

enum class CommonProgram
{
	// color replacement shaders
	TEX_REPLACE,
	TEX_ALPHA_REPLACE,
	#ifdef __ANDROID__
	TEX_EXTERNAL_REPLACE,
	#endif

	// color modulation shaders
	TEX,
	TEX_ALPHA,
	#ifdef __ANDROID__
	TEX_EXTERNAL,
	#endif
	NO_TEX
};

enum class CommonTextureSampler
{
	CLAMP,
	NEAREST_MIP_CLAMP,
	NO_MIP_CLAMP,
	NO_LINEAR_NO_MIP_CLAMP,
	REPEAT,
	NEAREST_MIP_REPEAT
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
	using SyncFenceImpl::SyncFenceImpl;
};

class DrawableHolder : public DrawableHolderImpl
{
public:
	constexpr DrawableHolder() {}
};

class RendererTask : public RendererTaskImpl
{
public:
	RendererTask(Renderer &r);
	void start(uint channels = 0);
	void pause();
	void stop();
	void draw(DrawableHolder &drawable, Base::Window &win, Base::Window::DrawParams params, DrawDelegate del, uint channel = 0);
	bool addOnDrawFinished(DrawFinishedDelegate del, int priority = 0);
	bool removeOnDrawFinished(DrawFinishedDelegate del);
	void updateDrawableForSurfaceChange(DrawableHolder &drawable, Base::Window::SurfaceChange change);
	Base::FrameTimeBase lastDrawTimestamp() const;
	Renderer &renderer() const;

private:
	Renderer &r;
};

class RendererDrawTask : public RendererDrawTaskImpl
{
public:
	using RendererDrawTaskImpl::RendererDrawTaskImpl;

	RendererCommands makeRendererCommands(Drawable drawable, Viewport viewport, Mat4 projMat);
	void waitSync(SyncFence fence);
	void verifyCurrentContext() const;
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
	void setBlendMode(uint mode);
	void setBlendEquation(uint mode);
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
	void setImgMode(uint mode);
	void setDither(bool on);
	bool dither();
	void setVisibleGeomFace(uint sides);
	void setClipTest(bool on);
	void setClipRect(ClipRect b);
	void setTexture(Texture &t);
	void setTextureSampler(TextureSampler sampler);
	void setCommonTextureSampler(CommonTextureSampler sampler);
	void setViewport(Viewport v);
	Viewport viewport();
	void vertexBufferData(const void *v, uint size);
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

	// rendering

	void clear();
	void drawPrimitives(Primitive mode, uint start, uint count);
	void drawPrimitiveElements(Primitive mode, const VertexIndex *idx, uint count);

private:
	RendererDrawTask *rTask;
	Renderer *r;
	Drawable drawable;

	void setCommonProgram(CommonProgram program, const Mat4 *modelMat);

	// no copying outside of class
	RendererCommands(const RendererCommands &) = default;
	RendererCommands &operator=(const RendererCommands &) = default;
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
	SyncFence addResourceSyncFence();
	void flush();
	void initWindow(Base::Window &win, Base::WindowConfig config);
	void setWindowValidOrientations(Base::Window &win, uint validO);
	void setProjectionMatrixRotation(Angle angle);
	void animateProjectionMatrixRotation(Angle srcAngle, Angle destAngle);
	static ClipRect makeClipRect(const Base::Window &win, IG::WindowRect rect);

	// shaders

	Shader makeShader(const char **src, uint srcCount, uint type);
	Shader makeShader(const char *src, uint type);
	Shader makeCompatShader(const char **src, uint srcCount, uint type);
	Shader makeCompatShader(const char *src, uint type);
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

	void setCorrectnessChecks(bool on);
	void setDebugOutput(bool on);
};

}
