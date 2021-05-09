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
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>

static const VideoImageEffect::EffectDesc
	hq2xDesc{"hq2x-v.txt", "hq2x-f.txt", {2, 2}};

static const VideoImageEffect::EffectDesc
	scale2xDesc{"scale2x-v.txt", "scale2x-f.txt", {2, 2}};

static const VideoImageEffect::EffectDesc
	prescale2xDesc{"direct-v.txt", "direct-f.txt", {2, 2}};

static Gfx::Shader makeEffectVertexShader(Gfx::Renderer &r, const char *src)
{
	const char *posDefs =
		"#define POS pos\n"
		"in vec4 pos;\n";
	const char *shaderSrc[]
	{
		posDefs,
		src
	};
	return r.makeCompatShader(shaderSrc, std::size(shaderSrc), Gfx::ShaderType::VERTEX);
}

static Gfx::Shader makeEffectFragmentShader(Gfx::Renderer &r, const char *src, bool isExternalTex)
{
	const char *fragDefs = "FRAGCOLOR_DEF\n";
	if(isExternalTex)
	{
		const char *shaderSrc[]
		{
			// extensions -> shaderSrc[0]
			"#extension GL_OES_EGL_image_external : enable\n"
			"#extension GL_OES_EGL_image_external_essl3 : enable\n",
			// texture define -> shaderSrc[1]
			"#define TEXTURE texture2D\n",
			fragDefs,
			"uniform lowp samplerExternalOES TEX;\n",
			src
		};
		auto shader = r.makeCompatShader(shaderSrc, std::size(shaderSrc), Gfx::ShaderType::FRAGMENT);
		if(!shader)
		{
			// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
			logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
			shaderSrc[1] = "#define TEXTURE texture\n";
			shader = r.makeCompatShader(shaderSrc, std::size(shaderSrc), Gfx::ShaderType::FRAGMENT);
		}
		return shader;
	}
	else
	{
		const char *shaderSrc[]
		{
			"#define TEXTURE texture\n",
			fragDefs,
			"uniform sampler2D TEX;\n",
			src
		};
		return r.makeCompatShader(shaderSrc, std::size(shaderSrc), Gfx::ShaderType::FRAGMENT);
	}
}

void VideoImageEffect::setEffect(Gfx::Renderer &r, unsigned effect, unsigned bitDepth, bool isExternalTex, const Gfx::TextureSampler &compatTexSampler)
{
	if(effect == effect_)
		return;
	deinit(r);
	useRGB565RenderTarget = bitDepth <= 16;
	effect_ = effect;
	compile(r, isExternalTex, compatTexSampler);
}

VideoImageEffect::EffectParams VideoImageEffect::effectParams() const
{
	return {useRGB565RenderTarget ? IG::PIXEL_FMT_RGB565 : IG::PIXEL_FMT_RGBA8888, effect_};
}

void VideoImageEffect::deinit(Gfx::Renderer &r)
{
	renderTarget_ = {};
	renderTargetScale = {0, 0};
	renderTargetImgSize = {0, 0};
	deinitProgram(r);
}

void VideoImageEffect::deinitProgram(Gfx::Renderer &r)
{
	prog.deinit(r.task());
	if(vShader)
	{
		r.deleteShader(vShader);
		vShader = 0;
	}
	if(fShader)
	{
		r.deleteShader(fShader);
		fShader = 0;
	}
}

void VideoImageEffect::initRenderTargetTexture(Gfx::Renderer &r, const Gfx::TextureSampler &compatTexSampler)
{
	if(!renderTargetScale.x)
		return;
	renderTargetImgSize.x = inputImgSize.x * renderTargetScale.x;
	renderTargetImgSize.y = inputImgSize.y * renderTargetScale.y;
	IG::PixmapDesc renderPix{renderTargetImgSize, useRGB565RenderTarget ? IG::PIXEL_RGB565 : IG::PIXEL_RGBA8888};
	if(!renderTarget_)
		renderTarget_ = r.makeTexture({renderPix, &compatTexSampler});
	else
		renderTarget_.setFormat(renderPix, 1, {}, &compatTexSampler);
	r.make(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
}

void VideoImageEffect::compile(Gfx::Renderer &r, bool isExternalTex, const Gfx::TextureSampler &compatTexSampler)
{
	if(program())
		return; // already compiled
	const EffectDesc *desc{};
	switch(effect_)
	{
		bcase HQ2X:
		{
			logMsg("compiling effect HQ2X");
			desc = &hq2xDesc;
		}
		bcase SCALE2X:
		{
			logMsg("compiling effect Scale2X");
			desc = &scale2xDesc;
		}
		bcase PRESCALE2X:
		{
			logMsg("compiling effect Prescale 2X");
			desc = &prescale2xDesc;
		}
		bdefault:
			break;
	}

	if(!desc)
	{
		logErr("effect descriptor not found");
		return;
	}

	renderTargetScale = desc->scale;
	initRenderTargetTexture(r, compatTexSampler);
	auto err = compileEffect(r, *desc, isExternalTex, false);
	if(err)
	{
		auto fallbackErr = compileEffect(r, *desc, isExternalTex, true);
		if(fallbackErr)
		{
			// print error from original compile if fallback effect not found
			auto &app = EmuApp::get(r.appContext());
			app.printfMessage(3, true, "%s", err->code().value() == ENOENT ? err->what() : fallbackErr->what());
			deinit(r);
			return;
		}
		logMsg("compiled fallback version of effect");
	}
}

std::optional<std::system_error> VideoImageEffect::compileEffect(Gfx::Renderer &r, EffectDesc desc, bool isExternalTex, bool useFallback)
{
	auto &app = EmuApp::get(r.appContext());
	{
		auto file = app.openAppAssetIO(r.appContext(),
			FS::makePathStringPrintf("shaders/%s%s", useFallback ? "fallback-" : "", desc.vShaderFilename),
			IO::AccessHint::ALL);
		if(!file)
		{
			deinitProgram(r);
			return std::system_error{{ENOENT, std::system_category()}, string_makePrintf<128>("Can't open file: %s", desc.vShaderFilename).data()};
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making vertex shader");
		vShader = makeEffectVertexShader(r, text);
		if(!vShader)
		{
			deinitProgram(r);
			r.autoReleaseShaderCompiler();
			return std::system_error{{EINVAL, std::system_category()}, "GPU rejected shader (vertex compile error)"};
		}
	}
	{
		auto file = app.openAppAssetIO(r.appContext(),
			FS::makePathStringPrintf("shaders/%s%s", useFallback ? "fallback-" : "", desc.fShaderFilename),
			IO::AccessHint::ALL);
		if(!file)
		{
			deinitProgram(r);
			r.autoReleaseShaderCompiler();
			return std::system_error{{ENOENT, std::system_category()}, string_makePrintf<128>("Can't open file: %s", desc.fShaderFilename).data()};
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making fragment shader");
		fShader = makeEffectFragmentShader(r, text, isExternalTex);
		if(!fShader)
		{
			deinitProgram(r);
			r.autoReleaseShaderCompiler();
			return std::system_error{{EINVAL, std::system_category()}, "GPU rejected shader (fragment compile error)"};
		}
	}
	logMsg("linking program");
	prog.init(r.task(), vShader, fShader, false, true);
	if(!prog.link(r.task()))
	{
		deinitProgram(r);
		r.autoReleaseShaderCompiler();
		return std::system_error{{EINVAL, std::system_category()}, "GPU rejected shader (link error)"};
	}
	srcTexelDeltaU = prog.uniformLocation(r.task(), "srcTexelDelta");
	srcTexelHalfDeltaU = prog.uniformLocation(r.task(), "srcTexelHalfDelta");
	srcPixelsU = prog.uniformLocation(r.task(), "srcPixels");
	updateProgramUniforms(r);
	r.autoReleaseShaderCompiler();
	return {};
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

void VideoImageEffect::setBitDepth(Gfx::Renderer &r, unsigned bitDepth, const Gfx::TextureSampler &compatTexSampler)
{
	useRGB565RenderTarget = bitDepth <= 16;
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

void VideoImageEffect::drawRenderTarget(Gfx::RendererCommands &cmds, const Gfx::Texture &img)
{
	auto viewport = Gfx::Viewport::makeFromRect({{}, renderTargetImgSize});
	cmds.setViewport(viewport);
	cmds.set(Gfx::CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP);
	Gfx::Sprite spr{{{-1., -1.}, {1., 1.}}, {&img, {{0., 1.}, {1., 0.}}}};
	spr.draw(cmds);
}

void VideoImageEffect::setCompatTextureSampler(const Gfx::TextureSampler &compatTexSampler)
{
	if(!renderTarget_)
		return;
	renderTarget_.setCompatTextureSampler(compatTexSampler);
}
