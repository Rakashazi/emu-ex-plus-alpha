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
#include <imagine/gfx/Viewport.hh>
#include <imagine/gfx/PixmapTexture.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/pixmap/PixelFormat.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRenderer.hh>
#endif

#include <vector>

class GfxImageSource;

namespace Base
{
class Window;
}

namespace Gfx
{

class RendererTask;
class Program;

static_assert((uint8_t)TextureBufferMode::DEFAULT == 0, "TextureBufferMode::DEFAULT != 0");

struct TextureBufferModeDesc
{
	const char *name = "";
	TextureBufferMode mode{};

	constexpr TextureBufferModeDesc() {}
	constexpr TextureBufferModeDesc(const char *name, TextureBufferMode mode):name{name}, mode{mode} {}
};

class RendererConfig
{
public:
	constexpr RendererConfig() {}
	constexpr RendererConfig(IG::PixelFormat pixelFormat): pixelFormat_{pixelFormat} {}

	void setPixelFormat(IG::PixelFormat pixelFormat)
	{
		pixelFormat_ = pixelFormat;
	}

	IG::PixelFormat pixelFormat() const
	{
		return pixelFormat_;
	}

private:
	IG::PixelFormat pixelFormat_{};
};

class Renderer : public RendererImpl
{
public:
	using RendererImpl::RendererImpl;
	constexpr Renderer() {}
	Renderer(Error &err);
	Renderer(RendererConfig config, Error &err);
	Renderer(Renderer &&o);
	Renderer &operator=(Renderer &&o);
	static std::pair<Renderer, Error> makeConfiguredRenderer(RendererConfig config = {});
	void configureRenderer();
	bool isConfigured() const;
	RendererTask &task() const;
	Base::WindowConfig addWindowConfig(Base::WindowConfig config) const;
	void initWindow(Base::Window &win, Base::WindowConfig config);
	void setWindowValidOrientations(Base::Window &win, Base::Orientation validO);
	void setProjectionMatrixRotation(Angle angle);
	void animateProjectionMatrixRotation(Base::Window &win, Angle srcAngle, Angle destAngle);
	static ClipRect makeClipRect(const Base::Window &win, IG::WindowRect rect);
	bool supportsSyncFences() const;
	void setPresentationTime(Drawable drawable, IG::FrameTime time) const;
	unsigned maxSwapChainImages() const;

	// shaders

	Shader makeShader(const char **src, uint32_t srcCount, ShaderType type);
	Shader makeShader(const char *src, ShaderType type);
	Shader makeCompatShader(const char **src, uint32_t srcCount, ShaderType type);
	Shader makeCompatShader(const char *src, ShaderType type);
	Shader makeDefaultVShader();
	bool makeCommonProgram(CommonProgram);
	bool commonProgramIsCompiled(CommonProgram program) const;
	void deleteShader(Shader shader);
	void uniformF(Program &program, int uniformLocation, float v1, float v2);
	void releaseShaderCompiler();
	void autoReleaseShaderCompiler();

	// resources

	Texture makeTexture(TextureConfig config);
	Texture makeTexture(GfxImageSource &img, const TextureSampler *compatSampler = {}, bool makeMipmaps = true);
	PixmapTexture makePixmapTexture(TextureConfig config);
	PixmapTexture makePixmapTexture(GfxImageSource &img, const TextureSampler *compatSampler = {}, bool makeMipmaps = true);
	PixmapBufferTexture makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode = {}, bool singleBuffer = false);
	std::vector<TextureBufferModeDesc> textureBufferModes();
	TextureBufferMode makeValidTextureBufferMode(TextureBufferMode mode = {});
	static bool textureBufferModeCanDoubleBuffer(TextureBufferMode mode);
	TextureSampler makeTextureSampler(TextureSamplerConfig config);
	TextureSampler &makeCommonTextureSampler(CommonTextureSampler sampler);
	TextureSampler &make(CommonTextureSampler sampler) { return makeCommonTextureSampler(sampler); }

	void setCorrectnessChecks(bool on);
};

}
