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

constexpr SystemLogger log{"VideoImageEffect"};

constexpr VideoImageEffect::EffectDesc directDesc{"direct-v.txt", "direct-f.txt", {1, 1}};

constexpr VideoImageEffect::EffectDesc hq2xDesc{"hq2x-v.txt", "hq2x-f.txt", {2, 2}};

constexpr VideoImageEffect::EffectDesc scale2xDesc{"scale2x-v.txt", "scale2x-f.txt", {2, 2}};

constexpr VideoImageEffect::EffectDesc prescale2xDesc{"direct-v.txt", "direct-f.txt", {2, 2}};
constexpr VideoImageEffect::EffectDesc prescale3xDesc{"direct-v.txt", "direct-f.txt", {3, 3}};
constexpr VideoImageEffect::EffectDesc prescale4xDesc{"direct-v.txt", "direct-f.txt", {4, 4}};

static constexpr const char *effectName(ImageEffectId id)
{
	switch(id)
	{
		case ImageEffectId::DIRECT: return "Direct";
		case ImageEffectId::HQ2X: return "HQ2X";
		case ImageEffectId::SCALE2X: return "Scale2X";
		case ImageEffectId::PRESCALE2X: return "Prescale 2X";
		case ImageEffectId::PRESCALE3X: return "Prescale 3X";
		case ImageEffectId::PRESCALE4X: return "Prescale 4X";
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
		case ImageEffectId::PRESCALE3X: return prescale3xDesc;
		case ImageEffectId::PRESCALE4X: return prescale4xDesc;
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
		return IG::PixelFmtRGBA8888;
	}
	return format;
}

VideoImageEffect::VideoImageEffect(Gfx::Renderer &r, Id effect, IG::PixelFormat fmt, Gfx::ColorSpace colSpace,
	Gfx::TextureSamplerConfig samplerConf, WSize size):
		quad{r.mainTask, {.size = 1}},
		inputImgSize{size == WSize{} ? WSize{1, 1} : size}, format{effectFormat(fmt, colSpace)}, colorSpace{colSpace}
{
	quad.write(0, {.bounds = {{-1, -1}, {1, 1}}});
	log.info("compiling effect:{}", effectName(effect));
	compile(r, effectDesc(effect), samplerConf);
}

void VideoImageEffect::initRenderTargetTexture(Gfx::Renderer &r, Gfx::TextureSamplerConfig samplerConf)
{
	if(!renderTargetScale.x)
		return;
	renderTargetImgSize.x = inputImgSize.x * renderTargetScale.x;
	renderTargetImgSize.y = inputImgSize.y * renderTargetScale.y;
	IG::PixmapDesc renderPix{renderTargetImgSize, format};
	if(!renderTarget_)
	{
		Gfx::TextureConfig conf{renderPix, samplerConf};
		conf.colorSpace = colorSpace;
		renderTarget_ = r.makeTexture(conf);
	}
	else
		renderTarget_.setFormat(renderPix, 1, colorSpace, samplerConf);
}

void VideoImageEffect::compile(Gfx::Renderer &r, EffectDesc desc, Gfx::TextureSamplerConfig samplerConf)
{
	if(program())
		return; // already compiled
	if(!desc.scale.x) [[unlikely]]
	{
		log.error("invalid effect descriptor");
		return;
	}
	renderTargetScale = desc.scale;
	initRenderTargetTexture(r, samplerConf);
	try
	{
		compileEffect(r, desc, false);
	}
	catch(std::exception &err)
	{
		try
		{
			compileEffect(r, desc, true);
			log.info("compiled fallback version of effect");
		}
		catch(std::exception &fallbackErr)
		{
			auto &app = EmuApp::get(r.appContext());
			app.postErrorMessage(5, std::format("{}, {}", err.what(), fallbackErr.what()));
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
		{.accessHint = IOAccessHint::All}).buffer().stringView());
	if(!vShader)
	{
		throw std::runtime_error{"GPU rejected shader (vertex compile error)"};
	}

	auto fShader = makeEffectFragmentShader(r,
		ctx.openAsset(IG::format<FS::PathString>("shaders/{}{}", fallbackStr, desc.fShaderFilename),
		{.accessHint = IOAccessHint::All}).buffer().stringView());
	if(!fShader)
	{
		throw std::runtime_error{"GPU rejected shader (fragment compile error)"};
	}
	Gfx::UniformLocationDesc uniformDescs[]
	{
		{"srcTexelDelta", &srcTexelDeltaU},
		{"srcTexelHalfDelta", &srcTexelHalfDeltaU},
		{"srcPixels", &srcPixelsU},
	};
	prog = {r.task(), vShader, fShader, {.hasTexture = true}, uniformDescs};
	if(!prog)
	{
		throw std::runtime_error{"GPU rejected shader (link error)"};
	}
	updateProgramUniforms(r);
}

void VideoImageEffect::updateProgramUniforms(Gfx::Renderer&)
{
	if(srcTexelDeltaU != -1)
		prog.uniform(srcTexelDeltaU, 1.0f / (float)inputImgSize.x, 1.0f / (float)inputImgSize.y);
	if(srcTexelHalfDeltaU != -1)
		prog.uniform(srcTexelHalfDeltaU, 0.5f * (1.0f / (float)inputImgSize.x), 0.5f * (1.0f / (float)inputImgSize.y));
	if(srcPixelsU != -1)
		prog.uniform(srcPixelsU, (float)inputImgSize.x, (float)inputImgSize.y);
}

void VideoImageEffect::setImageSize(Gfx::Renderer &r, WSize size, Gfx::TextureSamplerConfig samplerConf)
{
	if(size == WSize{0, 0})
		return;
	if(inputImgSize == size)
		return;
	inputImgSize = size;
	if(program())
		updateProgramUniforms(r);
	initRenderTargetTexture(r, samplerConf);
}

void VideoImageEffect::setFormat(Gfx::Renderer &r,IG::PixelFormat fmt, Gfx::ColorSpace colSpace, Gfx::TextureSamplerConfig samplerConf)
{
	fmt = effectFormat(fmt, colSpace);
	if(format == fmt && colorSpace == colSpace)
		return;
	format = fmt;
	colorSpace = colSpace;
	initRenderTargetTexture(r, samplerConf);
}

Gfx::Program &VideoImageEffect::program()
{
	return prog;
}

Gfx::Texture &VideoImageEffect::renderTarget()
{
	return renderTarget_;
}

void VideoImageEffect::drawRenderTarget(Gfx::RendererCommands &cmds, Gfx::TextureSpan texSpan)
{
	cmds.setViewport(renderTargetImgSize);
	cmds.set(texSpan);
	cmds.drawQuad(quad, 0);
}

void VideoImageEffect::setSampler(Gfx::TextureSamplerConfig samplerConf)
{
	renderTarget_.setSampler(samplerConf);
}

}
