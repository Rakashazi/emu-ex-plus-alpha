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
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/Program.hh>
#include <imagine/gfx/SyncFence.hh>
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

static_assert((uint8_t)TextureBufferMode::DEFAULT == 0, "TextureBufferMode::DEFAULT != 0");

struct TextureBufferModeDesc
{
	const char *name = "";
	TextureBufferMode mode{};

	constexpr TextureBufferModeDesc() {}
	constexpr TextureBufferModeDesc(const char *name, TextureBufferMode mode):name{name}, mode{mode} {}
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
	PixmapBufferTexture makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode = {}, bool singleBuffer = false);
	std::vector<TextureBufferModeDesc> textureBufferModes();
	TextureBufferMode makeValidTextureBufferMode(TextureBufferMode mode = {});
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
