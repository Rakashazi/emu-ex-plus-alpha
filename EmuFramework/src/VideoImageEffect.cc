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

#define LOGTAG "VideoImageEffect"
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

static constexpr VideoImageEffect::EffectDesc
	directDesc{"direct-v.txt", "direct-f.txt", {1, 1}};

static constexpr VideoImageEffect::EffectDesc
	hq2xDesc{"hq2x-v.txt", "hq2x-f.txt", {2, 2}};

static constexpr VideoImageEffect::EffectDesc
	scale2xDesc{"scale2x-v.txt", "scale2x-f.txt", {2, 2}};

static constexpr VideoImageEffect::EffectDesc
	prescale2xDesc{"direct-v.txt", "direct-f.txt", {2, 2}};

static constexpr const char *effectName(ImageEffectId id)
{
	switch(id)
	{
		case ImageEffectId::DIRECT: return "Direct";
		case ImageEffectId::HQ2X: return "HQ2X";
		case ImageEffectId::SCALE2X: return "Scale2X";
		case ImageEffectId::PRESCALE2X: return "Prescale 2X";
	}
	return nullptr;
}

static constexpr VideoImageEffect::EffectDesc effectDesc(ImageEffectId id)
{
	switch(id)
	{
		case ImageEffectId::DIRECT: return directDesc;
		case ImageEffectId::HQ2X: return hq2xDesc;
		case ImageEffectId::SCALE2X: return scale2xDesc;
		case ImageEffectId::PRESCALE2X: return prescale2xDesc;
	}
	return {};
}

static Gfx::Shader makeEffectVertexShader(Gfx::Renderer &r, std::string_view src)
{
	std::string_view posDefs =
		"#define POS pos\n"
		"in vec4 pos;\n";
	std::string_view shaderSrc[]
	{
		posDefs,
		src
	};
	return r.makeCompatShader(shaderSrc, Gfx::ShaderType::VERTEX);
}

static Gfx::Shader makeEffectFragmentShader(Gfx::Renderer &r, std::string_view src)
{
	std::string_view shaderSrc[]
	{
		"#define TEXTURE texture\n",
		"FRAGCOLOR_DEF\n",
		"uniform sampler2D TEX;\n",
		src
	};
	return r.makeCompatShader(shaderSrc, Gfx::ShaderType::FRAGMENT);
}

static PixelFormat effectFormat(IG::PixelFormat format, Gfx::ColorSpace colSpace)
{
	assert(format);
	if(colSpace == Gfx::ColorSpace::SRGB)
	{
		return IG::PIXEL_RGBA8888;
	}
	return format;
}

VideoImageEffect::VideoImageEffect(Gfx::Renderer &r, Id effect, IG::PixelFormat fmt, Gfx::ColorSpace colSpace,
	const Gfx::TextureSampler &compatTexSampler, IG::WP size):
		inputImgSize{size}, format{effectFormat(fmt, colSpace)}, colorSpace{colSpace}
{
	logMsg("compiling effect:%s", effectName(effect));
	compile(r, effectDesc(effect), compatTexSampler);
}

void VideoImageEffect::initRenderTargetTexture(Gfx::Renderer &r, const Gfx::TextureSampler &compatTexSampler)
{
	if(!renderTargetScale.x)
		return;
	renderTargetImgSize.x = inputImgSize.x * renderTargetScale.x;
	renderTargetImgSize.y = inputImgSize.y * renderTargetScale.y;
	IG::PixmapDesc renderPix{renderTargetImgSize, format};
	if(!renderTarget_)
	{
		Gfx::TextureConfig conf{renderPix, &compatTexSampler};
		conf.setColorSpace(colorSpace);
		renderTarget_ = r.makeTexture(conf);
	}
	else
		renderTarget_.setFormat(renderPix, 1, colorSpace, &compatTexSampler);
	r.make(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
}

void VideoImageEffect::compile(Gfx::Renderer &r, EffectDesc desc, const Gfx::TextureSampler &compatTexSampler)
{
	if(program())
		return; // already compiled
	if(!desc.scale.x) [[unlikely]]
	{
		logErr("invalid effect descriptor");
		return;
	}
	renderTargetScale = desc.scale;
	initRenderTargetTexture(r, compatTexSampler);
	try
	{
		compileEffect(r, desc, false);
	}
	catch(std::exception &err)
	{
		try
		{
			compileEffect(r, desc, true);
			logMsg("compiled fallback version of effect");
		}
		catch(std::exception &fallbackErr)
		{
			auto &app = EmuApp::get(r.appContext());
			app.postErrorMessage(5, fmt::format("{}, {}", err.what(), fallbackErr.what()));
			return;
		}
	}
}

void VideoImageEffect::compileEffect(Gfx::Renderer &r, EffectDesc desc, bool useFallback)
{
	auto ctx = r.appContext();
	const char *fallbackStr = useFallback ? "fallback-" : "";
	auto releaseShaderCompiler = IG::scopeGuard([&](){ r.autoReleaseShaderCompiler(); });

	auto vShader = makeEffectVertexShader(r,
		ctx.openAsset(IG::format<FS::PathString>("shaders/{}{}", fallbackStr, desc.vShaderFilename),
			IO::AccessHint::ALL).buffer().stringView());
	if(!vShader)
	{
		throw std::runtime_error{"GPU rejected shader (vertex compile error)"};
	}

	auto fShader = makeEffectFragmentShader(r,
		ctx.openAsset(IG::format<FS::PathString>("shaders/{}{}", fallbackStr, desc.fShaderFilename),
		IO::AccessHint::ALL).buffer().stringView());
	if(!fShader)
	{
		throw std::runtime_error{"GPU rejected shader (fragment compile error)"};
	}

	prog = {r.task(), vShader, fShader, false, true};
	if(!prog)
	{
		throw std::runtime_error{"GPU rejected shader (link error)"};
	}
	srcTexelDeltaU = prog.uniformLocation("srcTexelDelta");
	srcTexelHalfDeltaU = prog.uniformLocation("srcTexelHalfDelta");
	srcPixelsU = prog.uniformLocation("srcPixels");
	updateProgramUniforms(r);
}

void VideoImageEffect::updateProgramUniforms(Gfx::Renderer &r)
{
	if(srcTexelDeltaU != -1)
		r.uniformF(prog, srcTexelDeltaU, 1.0f / (float)inputImgSize.x, 1.0f / (float)inputImgSize.y);
	if(srcTexelHalfDeltaU != -1)
		r.uniformF(prog, srcTexelHalfDeltaU, 0.5f * (1.0f / (float)inputImgSize.x), 0.5f * (1.0f / (float)inputImgSize.y));
	if(srcPixelsU != -1)
		r.uniformF(prog, srcPixelsU, inputImgSize.x, inputImgSize.y);
}

void VideoImageEffect::setImageSize(Gfx::Renderer &r, IG::WP size, const Gfx::TextureSampler &compatTexSampler)
{
	if(size == IG::WP{0, 0})
		return;
	if(inputImgSize == size)
		return;
	inputImgSize = size;
	if(program())
		updateProgramUniforms(r);
	initRenderTargetTexture(r, compatTexSampler);
}

void VideoImageEffect::setFormat(Gfx::Renderer &r,IG::PixelFormat fmt, Gfx::ColorSpace colSpace, const Gfx::TextureSampler &compatTexSampler)
{
	fmt = effectFormat(fmt, colSpace);
	if(format == fmt && colorSpace == colSpace)
		return;
	format = fmt;
	colorSpace = colSpace;
	initRenderTargetTexture(r, compatTexSampler);
}

Gfx::Program &VideoImageEffect::program()
{
	return prog;
}

Gfx::Texture &VideoImageEffect::renderTarget()
{
	return renderTarget_;
}

void VideoImageEffect::drawRenderTarget(Gfx::RendererCommands &cmds, const Gfx::TextureSpan span)
{
	cmds.setViewport(renderTargetImgSize);
	cmds.set(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
	Gfx::Sprite spr{{{-1., -1.}, {1., 1.}}, {span.texture(), {{0., 1.}, {1., 0.}}}};
	spr.draw(cmds);
}

void VideoImageEffect::setCompatTextureSampler(const Gfx::TextureSampler &compatTexSampler)
{
	if(!renderTarget_)
		return;
	renderTarget_.setCompatTextureSampler(compatTexSampler);
}

}
