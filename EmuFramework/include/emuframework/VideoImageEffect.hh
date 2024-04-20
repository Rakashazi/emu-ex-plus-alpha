#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/Program.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/util/enum.hh>

namespace EmuEx
{

using namespace IG;

WISE_ENUM_CLASS((ImageEffectId, uint8_t),
	(DIRECT, 0),
	(HQ2X, 1),
	(SCALE2X, 2),
	(PRESCALE2X, 3),
	(PRESCALE3X, 4),
	(PRESCALE4X, 5));

class VideoImageEffect
{
public:
	using Id = ImageEffectId;

	struct EffectDesc
	{
		const char *vShaderFilename;
		const char *fShaderFilename;
		WSize scale;
	};

	constexpr	VideoImageEffect() = default;
	VideoImageEffect(Gfx::Renderer &r, Id effect, PixelFormat, Gfx::ColorSpace, Gfx::TextureSamplerConfig, WSize size);
	void setImageSize(Gfx::Renderer &r, WSize size, Gfx::TextureSamplerConfig);
	void setFormat(Gfx::Renderer &r, IG::PixelFormat, Gfx::ColorSpace, Gfx::TextureSamplerConfig);
	void setSampler(Gfx::TextureSamplerConfig);
	Gfx::Program &program();
	Gfx::Texture &renderTarget();
	void drawRenderTarget(Gfx::RendererCommands &, Gfx::TextureSpan);
	constexpr IG::PixelFormat imageFormat() const { return format; }
	operator bool() const { return (bool)prog; }

private:
	Gfx::ITexQuads quad;
	Gfx::Texture renderTarget_;
	Gfx::Program prog;
	int srcTexelDeltaU{};
	int srcTexelHalfDeltaU{};
	int srcPixelsU{};
	WSize renderTargetScale;
	WSize renderTargetImgSize;
	WSize inputImgSize{1, 1};
	IG::PixelFormat format;
	Gfx::ColorSpace colorSpace{Gfx::ColorSpace::LINEAR};

	void initRenderTargetTexture(Gfx::Renderer &r, Gfx::TextureSamplerConfig);
	void updateProgramUniforms(Gfx::Renderer &r);
	void compile(Gfx::Renderer &r, EffectDesc desc, Gfx::TextureSamplerConfig);
	void compileEffect(Gfx::Renderer &r, EffectDesc desc, bool useFallback);
};

}
