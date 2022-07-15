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
#include <imagine/gfx/TextureConfig.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLTexture.hh>
#endif

#include <imagine/pixmap/Pixmap.hh>
#include <utility>

namespace IG::Data
{
class PixmapSource;
}

namespace IG::Gfx
{

class Renderer;
class RendererTask;
class RendererCommands;
class TextureSampler;
class Mat4;

class LockedTextureBuffer: public LockedTextureBufferImpl
{
public:
	using LockedTextureBufferImpl::LockedTextureBufferImpl;
	MutablePixmapView pixmap() const;
	IG::WindowRect sourceDirtyRect() const;
	explicit operator bool() const;
};

class Texture: public TextureImpl
{
public:
	static constexpr uint32_t MAX_ASSUME_ALIGN = 8;
	static constexpr uint32_t WRITE_FLAG_ASYNC = IG::bit(0);
	static constexpr uint32_t WRITE_FLAG_MAKE_MIPMAPS = IG::bit(1);
	static constexpr uint32_t BUFFER_FLAG_CLEARED = IG::bit(0);

	using TextureImpl::TextureImpl;
	Texture(RendererTask &, TextureConfig, IG::ErrorCode *errorPtr = nullptr);
	Texture(RendererTask &, IG::Data::PixmapSource, const TextureSampler *compatSampler, bool makeMipmaps, IG::ErrorCode *errorPtr = nullptr);
	Texture(Texture &&o) noexcept;
	Texture &operator=(Texture &&o) noexcept;
	static uint8_t bestAlignment(PixmapView pixmap);
	bool canUseMipmaps() const;
	bool generateMipmaps();
	int levels() const;
	IG::ErrorCode setFormat(PixmapDesc, int levels, ColorSpace c = {}, const TextureSampler *compatSampler = {});
	void write(int level, PixmapView pixmap, IG::WP destPos, uint32_t writeFlags = 0);
	void writeAligned(int level, PixmapView pixmap, IG::WP destPos, uint8_t assumedDataAlignment, uint32_t writeFlags = 0);
	void clear(int level);
	LockedTextureBuffer lock(int level, uint32_t bufferFlags = 0);
	LockedTextureBuffer lock(int level, IG::WindowRect rect, uint32_t bufferFlags = 0);
	void unlock(LockedTextureBuffer lockBuff, uint32_t writeFlags = 0);
	IG::WP size(int level) const;
	PixmapDesc pixmapDesc() const;
	void setCompatTextureSampler(const TextureSampler &compatSampler);
	bool compileDefaultProgram(EnvMode mode) const;
	bool compileDefaultProgramOneShot(EnvMode mode) const;
	void useDefaultProgram(RendererCommands &cmds, EnvMode mode, const Mat4 *modelMat) const;
	void useDefaultProgram(RendererCommands &cmds, EnvMode mode) const { useDefaultProgram(cmds, mode, nullptr); }
	void useDefaultProgram(RendererCommands &cmds, EnvMode mode, Mat4 modelMat) const;
	explicit operator bool() const;
	Renderer &renderer() const;
	RendererTask &task() const;
	operator TextureSpan() const;
};

}
