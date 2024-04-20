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
#include <imagine/gfx/TextureSamplerConfig.hh>
#include <imagine/pixmap/PixelFormat.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRenderer.hh>
#endif

#include <vector>
#include <span>
#include <string_view>

namespace IG::Data
{
class PixmapSource;
}

namespace IG
{
class Window;
class Viewport;
}

namespace IG::Gfx
{

class RendererTask;
class Program;

struct TextureBufferModeDesc
{
	const char *name = "";
	TextureBufferMode mode{};
	constexpr bool operator ==(TextureBufferMode mode_) const { return mode == mode_; }

};

struct DrawableConfig
{
	PixelFormat pixelFormat{};
	ColorSpace colorSpace{};
	constexpr bool operator ==(const DrawableConfig&) const = default;
	explicit constexpr operator bool() const { return (bool)pixelFormat || (bool)colorSpace; }
};

struct DrawableConfigDesc
{
	const char *name{};
	DrawableConfig config{};
	constexpr bool operator ==(const DrawableConfig &c) const { return config == c; }
	explicit constexpr operator bool() const { return (bool)config; }
};

class Renderer : public RendererImpl
{
public:
	using RendererImpl::RendererImpl;
	Renderer(ApplicationContext);
	~Renderer();
	void configureRenderer();
	bool isConfigured() const;
	const RendererTask &task() const;
	RendererTask &task();
	ApplicationContext appContext() const;
	void initMainTask(Window *initialWindow, DrawableConfig c = {});
	bool attachWindow(Window &, DrawableConfig c = {});
	void detachWindow(Window &);
	bool setDrawableConfig(Window &, DrawableConfig);
	void setDefaultViewport(Window &, Viewport);
	bool canRenderToMultiplePixelFormats() const;
	NativeWindowFormat nativeWindowFormat() const;
	void setWindowValidOrientations(Window &, Orientations);
	void animateWindowRotation(Window &, float srcAngle, float destAngle);
	float projectionRollAngle(const Window &) const;
	static ClipRect makeClipRect(const Window &win, WindowRect rect);
	bool supportsSyncFences() const;
	bool supportsPresentationTime() const;
	PresentMode evalPresentMode(const Window &, PresentMode) const;
	int maxSwapChainImages() const;
	void setCorrectnessChecks(bool on);
	std::vector<DrawableConfigDesc> supportedDrawableConfigs() const;
	bool hasBgraFormat(TextureBufferMode) const;

	// shaders

	Shader makeShader(std::span<std::string_view> srcs, ShaderType type);
	Shader makeShader(std::string_view src, ShaderType type);
	Shader makeCompatShader(std::span<std::string_view> srcs, ShaderType type);
	Shader makeCompatShader(std::string_view src, ShaderType type);
	BasicEffect &basicEffect();
	void releaseShaderCompiler();
	void autoReleaseShaderCompiler();

	// resources

	Texture makeTexture(TextureConfig);
	Texture makeTexture(Data::PixmapSource, TextureSamplerConfig samplerConf = {}, bool makeMipmaps = true);
	PixmapBufferTexture makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode = {}, bool singleBuffer = false);
	std::vector<TextureBufferModeDesc> textureBufferModes();
	TextureBufferMode evalTextureBufferMode(TextureBufferMode mode = {});
	TextureBufferMode validateTextureBufferMode(TextureBufferMode);
	TextureSampler makeTextureSampler(TextureSamplerConfig);

	// color space control

	bool supportsColorSpace() const;
	bool hasSrgbColorSpaceWriteControl() const;
	static ColorSpace supportedColorSpace(PixelFormat, ColorSpace wantedColorSpace);

	// optional features

	static const bool enableSamplerObjects;
};

}
