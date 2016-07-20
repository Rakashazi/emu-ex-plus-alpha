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

#include <imagine/gfx/Gfx.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/RenderTarget.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <system_error>

class VideoImageEffect
{
public:
	struct EffectDesc
	{
		const char *vShaderFilename;
		const char *fShaderFilename;
		IG::WP scale;
	};

private:
	Gfx::Program prog{};
	Gfx::Shader vShader{};
	Gfx::Shader fShader{};
	int srcTexelDeltaU{};
	int srcTexelHalfDeltaU{};
	int srcPixelsU{};
	uint effect_ = NO_EFFECT;
	Gfx::RenderTarget renderTarget_{};
	IG::WP renderTargetScale{};
	IG::WP renderTargetImgSize{};
	IG::WP inputImgSize{1, 1};
	bool useRGB565RenderTarget = true;

	void initRenderTargetTexture();
	void updateProgramUniforms();
	void compile(bool isExternalTex);
	std::system_error compileEffect(EffectDesc desc, bool isExternalTex, bool useFallback);
	void deinitProgram();

public:
	enum
	{
		NO_EFFECT = 0,
		HQ2X = 1,
		SCALE2X = 2,
		PRESCALE2X = 3,

		LAST_EFFECT_VAL
	};

	constexpr	VideoImageEffect() {}
	void setEffect(uint effect, bool isExternalTex);
	uint effect();
	void setImageSize(IG::WP size);
	void setBitDepth(uint bitDepth);
	Gfx::Program &program();
	Gfx::RenderTarget &renderTarget();
	void drawRenderTarget(Gfx::PixmapTexture &img);
	void deinit();
};
