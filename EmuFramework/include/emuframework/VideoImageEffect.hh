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
#include <system_error>
#include <optional>

class VideoImageEffect
{
public:
	enum
	{
		NO_EFFECT = 0,
		HQ2X = 1,
		SCALE2X = 2,
		PRESCALE2X = 3,

		LAST_EFFECT_VAL
	};

	struct EffectDesc
	{
		const char *vShaderFilename;
		const char *fShaderFilename;
		IG::WP scale;
	};

	constexpr	VideoImageEffect() {}
	void setEffect(Gfx::Renderer &r, uint effect, bool isExternalTex);
	uint effect();
	void setImageSize(Gfx::Renderer &r, IG::WP size);
	void setBitDepth(Gfx::Renderer &r, uint bitDepth);
	Gfx::Program &program();
	Gfx::Texture &renderTarget();
	void drawRenderTarget(Gfx::RendererCommands &cmds, Gfx::PixmapTexture &img);
	void deinit(Gfx::Renderer &r);

private:
	Gfx::Texture renderTarget_{};
	Gfx::Program prog{};
	Gfx::Shader vShader{};
	Gfx::Shader fShader{};
	int srcTexelDeltaU{};
	int srcTexelHalfDeltaU{};
	int srcPixelsU{};
	uint effect_ = NO_EFFECT;
	IG::WP renderTargetScale{};
	IG::WP renderTargetImgSize{};
	IG::WP inputImgSize{1, 1};
	bool useRGB565RenderTarget = true;

	void initRenderTargetTexture(Gfx::Renderer &r);
	void updateProgramUniforms(Gfx::Renderer &r);
	void compile(Gfx::Renderer &r, bool isExternalTex);
	std::optional<std::system_error> compileEffect(Gfx::Renderer &r, EffectDesc desc, bool isExternalTex, bool useFallback);
	void deinitProgram(Gfx::Renderer &r);
};
