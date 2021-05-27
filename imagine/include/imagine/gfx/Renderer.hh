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
	constexpr bool operator ==(TextureBufferMode mode_) const { return mode == mode_; }

};

struct BufferFormatDesc
{
	const char *name;
	IG::PixelFormat format;

	constexpr bool operator ==(IG::PixelFormat format_) const { return format == format_; }
};

class Renderer : public RendererImpl
{
public:
	using RendererImpl::RendererImpl;
	Renderer(Base::ApplicationContext, Error &errOut);
	~Renderer();
	void configureRenderer();
	bool isConfigured() const;
	const RendererTask &task() const;
	RendererTask &task();
	Base::ApplicationContext appContext() const;
	Error initMainTask(Base::Window *initialWindow, IG::PixelFormat f = PIXEL_FMT_NONE, ColorSpace c = {});
	bool attachWindow(Base::Window &, ColorSpace c = {});
	void detachWindow(Base::Window &);
	Base::NativeWindowFormat nativeWindowFormat() const;
	void setWindowValidOrientations(Base::Window &win, Base::Orientation validO);
	void animateProjectionMatrixRotation(Base::Window &win, Angle srcAngle, Angle destAngle);
	static ClipRect makeClipRect(const Base::Window &win, IG::WindowRect rect);
	bool supportsSyncFences() const;
	void setPresentationTime(Base::Window &, IG::FrameTime time) const;
	unsigned maxSwapChainImages() const;
	void setCorrectnessChecks(bool on);
	std::vector<BufferFormatDesc> supportedBufferFormats() const;

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
	TextureSampler makeTextureSampler(TextureSamplerConfig config);
	const TextureSampler &makeCommonTextureSampler(CommonTextureSampler sampler);
	const TextureSampler &make(CommonTextureSampler sampler) { return makeCommonTextureSampler(sampler); }
	const TextureSampler &commonTextureSampler(CommonTextureSampler sampler) const;
	const TextureSampler &get(CommonTextureSampler sampler) const { return commonTextureSampler(sampler); }

	// color space control

	bool setColorSpace(Base::Window &, ColorSpace c = {});
	bool supportsColorSpace() const;
	bool hasSrgbColorSpaceWriteControl() const;
};

}
