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
#include <imagine/base/Base.hh>
#include <imagine/pixmap/PixelFormat.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRenderer.hh>
#endif

namespace Gfx
{

class RenderTarget;

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

static constexpr auto ColorFormat = IG::PIXEL_DESC_RGBA8888;

class Program : public ProgramImpl
{
public:
	constexpr Program() {}
	bool init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex);
	void deinit();
	bool link(Renderer &r);
	int uniformLocation(const char *uniformName);
};

class Renderer : public RendererImpl
{
public:
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
	Renderer(std::system_error &err);
	Renderer(IG::PixelFormat pixelFormat, std::system_error &err);
	static Renderer makeConfiguredRenderer(std::system_error &err);
	static Renderer makeConfiguredRenderer(IG::PixelFormat pixelFormat, std::system_error &err);
	void configureRenderer();
	bool isConfigured() const;
	void bind();
	void unbind();
	bool restoreBind();
	Base::WindowConfig addWindowConfig(Base::WindowConfig config);
	void updateDrawableForSurfaceChange(Drawable &drawable, Base::Window::SurfaceChange change);
	bool setCurrentDrawable(Drawable win);
	bool updateCurrentDrawable(Drawable &drawable, Base::Window &win, Base::Window::DrawParams params, Viewport viewport, Mat4 projMat);
	void deinitDrawable(Drawable &drawable);
	void presentDrawable(Drawable win);
	void finishPresentDrawable(Drawable win);
	void finish();
	void initWindow(Base::Window &win, Base::WindowConfig config);
	void setWindowValidOrientations(Base::Window &win, uint validO);
	void setRenderTarget(const RenderTarget &target);

	// render states

	void setBlend(bool on);
	void setBlendFunc(BlendFunc s, BlendFunc d);
	void setBlendMode(uint mode);
	void setBlendEquation(uint mode);
	void setImgBlendColor(ColorComp r, ColorComp g, ColorComp b, ColorComp a);
	void setZTest(bool on);
	void setZBlend(bool on);
	void setZBlendColor(ColorComp r, ColorComp g, ColorComp b);
	void clear();
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
	uint color();
	void setImgMode(uint mode);
	void setDither(bool on);
	bool dither();
	void setVisibleGeomFace(uint sides);
	void setClipRect(bool on);
	void setClipRectBounds(const Base::Window &win, int x, int y, int w, int h);
	void setClipRectBounds(const Base::Window &win, IG::WindowRect r)
	{
		setClipRectBounds(win, r.x, r.y, r.xSize(), r.ySize());
	}
	void setViewport(const Viewport &v);
	const Viewport &viewport();
	void vertexBufferData(const void *v, uint size);
	void drawPrimitives(Primitive mode, uint start, uint count);
	void drawPrimitiveElements(Primitive mode, const VertexIndex *idx, uint count);

	// transforms

	void setTransformTarget(TransformTargetEnum target);
	void loadTransform(Mat4 mat);
	void loadTranslate(GC x, GC y, GC z);
	void loadIdentTransform();
	void setProjectionMatrix(const Mat4 &mat);
	void setProjectionMatrixRotation(Angle angle);
	void animateProjectionMatrixRotation(Angle srcAngle, Angle destAngle);
	const Mat4 &projectionMatrix();

	// shaders

	Shader makeShader(const char **src, uint srcCount, uint type);
	Shader makeShader(const char *src, uint type);
	Shader makeCompatShader(const char **src, uint srcCount, uint type);
	Shader makeCompatShader(const char *src, uint type);
	Shader makeDefaultVShader();
	void deleteShader(Shader shader);
	void setProgram(Program &program);
	void setProgram(Program &program, Mat4 modelMat);
	void uniformF(int uniformLocation, float v1, float v2);
	void releaseShaderCompiler();
	void autoReleaseShaderCompiler();

	void setCorrectnessChecks(bool on);
	void setDebugOutput(bool on);
};

}
