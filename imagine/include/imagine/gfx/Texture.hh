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

#include <utility>
#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/data-type/image/GfxImageSource.hh>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/Texture.hh>
#endif

namespace Gfx
{

class Renderer;
class RendererCommands;

class TextureSamplerConfig
{
private:
	bool minLinearFiltering = true;
	bool magLinearFiltering = true;
	MipFilterMode mipFiltering = MIP_FILTER_LINEAR;
	WrapMode xWrapMode_ = WRAP_CLAMP;
	WrapMode yWrapMode_ = WRAP_CLAMP;

public:
	constexpr TextureSamplerConfig() {}

	void setLinearFilter(bool on)
	{
		minLinearFiltering = magLinearFiltering = on;
	}

	void setMinLinearFilter(bool on)
	{
		minLinearFiltering = on;
	}

	void setMagLinearFilter(bool on)
	{
		magLinearFiltering = on;
	}

	bool minLinearFilter() const
	{
		return minLinearFiltering;
	}

	bool magLinearFilter() const
	{
		return magLinearFiltering;
	}

	void setMipFilter(MipFilterMode filter)
	{
		mipFiltering = filter;
	}

	MipFilterMode mipFilter() const
	{
		return mipFiltering;
	}

	void setWrapMode(WrapMode mode)
	{
		xWrapMode_ = yWrapMode_ = mode;
	}

	void setXWrapMode(WrapMode mode)
	{
		xWrapMode_ = mode;
	}

	void setYWrapMode(WrapMode mode)
	{
		yWrapMode_ = mode;
	}

	WrapMode xWrapMode() const
	{
		return xWrapMode_;
	}

	WrapMode yWrapMode() const
	{
		return yWrapMode_;
	}

	static TextureSamplerConfig makeWithVideoUseConfig()
	{
		TextureSamplerConfig config;
		config.setLinearFilter(true);
		config.setMipFilter(MIP_FILTER_NONE);
		config.setWrapMode(WRAP_CLAMP);
		return config;
	}
};

class TextureConfig
{
private:
	bool writeOften = false;
	bool genMipmaps = false;
	uint32_t levels_ = 1;
	IG::PixmapDesc pixmapDesc_;

public:
	constexpr TextureConfig() {}
	constexpr TextureConfig(IG::PixmapDesc pixDesc): pixmapDesc_{pixDesc} {}

	void setLevels(uint32_t levels)
	{
		levels_ = levels;
	}

	void setAllLevels()
	{
		levels_ = 0;
	}

	uint32_t levels()
	{
		return levels_;
	}

	void setWillWriteOften(bool on)
	{
		writeOften = on;
	}

	bool willWriteOften() const
	{
		return writeOften;
	}

	void setWillGenerateMipmaps(bool on)
	{
		genMipmaps = on;
		if(on)
			setAllLevels();
	}

	bool willGenerateMipmaps() const
	{
		return genMipmaps;
	}

	void setPixmapDesc(IG::PixmapDesc pixDesc)
	{
		pixmapDesc_ = pixDesc;
	}

	IG::PixmapDesc pixmapDesc()
	{
		return pixmapDesc_;
	}
};

class TextureSampler: public TextureSamplerImpl
{
public:
	constexpr TextureSampler() {}
	void init2(Renderer &r, TextureSamplerConfig config);
	void deinit(Renderer &r);
	explicit operator bool() const;
	static void initDefaultClampSampler(Renderer &r);
	static void initDefaultNearestMipClampSampler(Renderer &r);
	static void initDefaultNoMipClampSampler(Renderer &r);
	static void initDefaultNoLinearNoMipClampSampler(Renderer &r);
	static void initDefaultRepeatSampler(Renderer &r);
	static void initDefaultNearestMipRepeatSampler(Renderer &r);
	static void bindDefaultClampSampler(RendererCommands &cmds);
	static void bindDefaultNearestMipClampSampler(RendererCommands &cmds);
	static void bindDefaultNoMipClampSampler(RendererCommands &cmds);
	static void bindDefaultNoLinearNoMipClampSampler(RendererCommands &cmds);
	static void bindDefaultRepeatSampler(RendererCommands &cmds);
	static void bindDefaultNearestMipRepeatSampler(RendererCommands &cmds);
};

class LockedTextureBuffer: public LockedTextureBufferImpl
{
public:
	constexpr LockedTextureBuffer() {}
	IG::Pixmap pixmap() const;
	IG::WindowRect sourceDirtyRect() const;
	explicit operator bool() const;
};

class Texture: public TextureImpl
{
public:
	static constexpr uint32_t MAX_ASSUME_ALIGN = 8;
	static constexpr uint32_t COMMIT_FLAG_ASYNC = IG::bit(0);

	constexpr Texture() {}
	Texture(Texture &&o);
	Texture &operator=(Texture &&o);
	Error init2(Renderer &r, TextureConfig config);
	Error init2(Renderer &r, GfxImageSource &img, bool makeMipmaps);
	void deinit();
	static uint32_t bestAlignment(const IG::Pixmap &pixmap);
	bool canUseMipmaps() const;
	bool generateMipmaps();
	uint32_t levels() const;
	Error setFormat(IG::PixmapDesc desc, uint32_t levels);
	void write(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t commitFlags = 0);
	void writeAligned(uint32_t level, const IG::Pixmap &pixmap, IG::WP destPos, uint32_t assumedDataAlignment, uint32_t commitFlags = 0);
	void clear(uint32_t level);
	LockedTextureBuffer lock(uint32_t level);
	LockedTextureBuffer lock(uint32_t level, IG::WindowRect rect);
	void unlock(LockedTextureBuffer lockBuff);
	IG::WP size(uint32_t level) const;
	IG::PixmapDesc pixmapDesc() const;
	bool compileDefaultProgram(uint32_t mode);
	bool compileDefaultProgramOneShot(uint32_t mode);
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode, const Mat4 *modelMat) const;
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode) const { useDefaultProgram(cmds, mode, nullptr); }
	void useDefaultProgram(RendererCommands &cmds, uint32_t mode, Mat4 modelMat) const { useDefaultProgram(cmds, mode, &modelMat); }
	explicit operator bool() const;
	Renderer &renderer();

protected:
	Renderer *r{};

	// no copying outside of class
	Texture(const Texture &) = default;
	Texture &operator=(const Texture &) = default;
};

class PixmapTexture: public Texture
{
public:
	constexpr PixmapTexture() {}
	Error init2(Renderer &r, TextureConfig config);
	Error init2(Renderer &r, GfxImageSource &img, bool makeMipmaps);
	Error setFormat(IG::PixmapDesc desc, uint32_t levels);
	IG::Rect2<GTexC> uvBounds() const;
	IG::PixmapDesc usedPixmapDesc() const;

private:
	IG::Rect2<GTexC> uv{};
	IG::WP usedSize{};

	void updateUV(IG::WP pixPos, IG::WP pixSize);
};

}
