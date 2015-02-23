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

static const VideoImageEffect::EffectDesc
	hq2xDesc{"hq2x-v.txt", "hq2x-f.txt", {2, 2}};

static const VideoImageEffect::EffectDesc
	scale2xDesc{"scale2x-v.txt", "scale2x-f.txt", {2, 2}};

static const VideoImageEffect::EffectDesc
	prescale2xDesc{"direct-v.txt", "direct-f.txt", {2, 2}};

static Gfx::Shader makeEffectVertexShader(const char *src)
{
	const char *posDefs =
	"#define OUT_POSITION gl_Position = pos\n"
	"in vec4 pos;\n";
	const char *shaderSrc[]
	{
		posDefs,
		src
	};
	return Gfx::makeCompatShader(shaderSrc, sizeofArray(shaderSrc), GL_VERTEX_SHADER);
}

static Gfx::Shader makeEffectFragmentShader(const char *src, bool isExternalTex)
{
	const char *externalTexExt =
	"#extension GL_OES_EGL_image_external : require\n";
	const char *fragDefs =
	"#define OUT_FRAGCOLOR(c) FRAGCOLOR = c\n"
	"FRAGCOLOR_DEF\n";
	const char *samplerExternal = "uniform samplerExternalOES TEX;\n";
	const char *sampler2D = "uniform sampler2D TEX;\n";
	const char *shaderSrc[]
	{
		#ifdef __ANDROID__
		isExternalTex ? externalTexExt : "",
		#endif
		fragDefs,
		(Config::envIsAndroid && isExternalTex) ? samplerExternal : sampler2D,
		src
	};
	return Gfx::makeCompatShader(shaderSrc, sizeofArray(shaderSrc), GL_FRAGMENT_SHADER);
}

void VideoImageEffect::setEffect(uint effect, bool isExternalTex)
{
	if(effect == effect_)
		return;
	deinit();
	effect_ = effect;
	compile(isExternalTex);
}

void VideoImageEffect::deinit()
{
	renderTarget_.deinit();
	renderTargetScale = {0, 0};
	renderTargetImgSize = {0, 0};
	deinitProgram();
}

void VideoImageEffect::deinitProgram()
{
	prog.deinit();
	if(vShader)
	{
		Gfx::deleteShader(vShader);
		vShader = 0;
	}
	if(fShader)
	{
		Gfx::deleteShader(fShader);
		fShader = 0;
	}
}

uint VideoImageEffect::effect()
{
	return effect_;
}

void VideoImageEffect::initRenderTargetTexture()
{
	renderTargetImgSize.x = inputImgSize.x * renderTargetScale.x;
	renderTargetImgSize.y = inputImgSize.y * renderTargetScale.y;
	IG::PixmapDesc renderPix{useRGB565RenderTarget ? PixelFormatRGB565 : PixelFormatRGBA8888};
	renderPix.x = renderTargetImgSize.x;
	renderPix.y = renderTargetImgSize.y;
	renderPix.pitch = renderPix.x * renderPix.format.bytesPerPixel;
	renderTarget_.initTexture(renderPix);
	Gfx::TextureSampler::initDefaultNoLinearNoMipClampSampler();
}

void VideoImageEffect::compile(bool isExternalTex)
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
	renderTarget_.init();
	initRenderTargetTexture();
	ErrorMessage msg{};
	if(compileEffect(*desc, isExternalTex, false, &msg) != OK)
	{
		ErrorMessage fallbackMsg{};
		auto r = compileEffect(*desc, isExternalTex, true, &fallbackMsg);
		if(r != OK)
		{
			// print error from original compile if fallback effect not found
			popup.printf(3, true, "%s", r == NOT_FOUND ? msg.data() : fallbackMsg.data(), 3);
			deinit();
			return;
		}
		logMsg("compiled fallback version of effect");
	}
}

CallResult VideoImageEffect::compileEffect(EffectDesc desc, bool isExternalTex, bool useFallback, ErrorMessage *msg)
{
	{
		auto file = openAppAssetIO(makeFSPathStringPrintf("shaders/%s%s", useFallback ? "fallback-" : "", desc.vShaderFilename));
		if(!file)
		{
			deinitProgram();
			if(msg)
				string_printf(*msg, "Can't open file: %s", desc.vShaderFilename);
			return NOT_FOUND;
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making vertex shader");
		vShader = makeEffectVertexShader(text);
		if(!vShader)
		{
			deinitProgram();
			if(msg)
				string_printf(*msg, "GPU rejected shader (vertex compile error)");
			Gfx::autoReleaseShaderCompiler();
			return INVALID_PARAMETER;
		}
	}
	{
		auto file = openAppAssetIO(makeFSPathStringPrintf("shaders/%s%s", useFallback ? "fallback-" : "", desc.fShaderFilename));
		if(!file)
		{
			deinitProgram();
			if(msg)
				string_printf(*msg, "Can't open file: %s", desc.fShaderFilename);
			Gfx::autoReleaseShaderCompiler();
			return NOT_FOUND;
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making fragment shader");
		fShader = makeEffectFragmentShader(text, isExternalTex);
		if(!fShader)
		{
			deinitProgram();
			if(msg)
				string_printf(*msg, "GPU rejected shader (fragment compile error)");
			Gfx::autoReleaseShaderCompiler();
			return INVALID_PARAMETER;
		}
	}
	logMsg("linking program");
	prog.init(vShader, fShader, false, true);
	if(!prog.link())
	{
		deinitProgram();
		if(msg)
			string_printf(*msg, "GPU rejected shader (link error)");
		Gfx::autoReleaseShaderCompiler();
		return INVALID_PARAMETER;
	}
	srcTexelDeltaU = prog.uniformLocation("srcTexelDelta");
	srcTexelHalfDeltaU = prog.uniformLocation("srcTexelHalfDelta");
	srcPixelsU = prog.uniformLocation("srcPixels");
	updateProgramUniforms();
	Gfx::autoReleaseShaderCompiler();
	return OK;
}

void VideoImageEffect::updateProgramUniforms()
{
	setProgram(prog);
	if(srcTexelDeltaU != -1)
		Gfx::uniformF(srcTexelDeltaU, 1.0f / (float)inputImgSize.x, 1.0f / (float)inputImgSize.y);
	if(srcTexelHalfDeltaU != -1)
		Gfx::uniformF(srcTexelHalfDeltaU, 0.5f * (1.0f / (float)inputImgSize.x), 0.5f * (1.0f / (float)inputImgSize.y));
	if(srcPixelsU != -1)
		Gfx::uniformF(srcPixelsU, inputImgSize.x, inputImgSize.y);
}

void VideoImageEffect::setImageSize(IG::Point2D<uint> size)
{
	if(inputImgSize.x == size.x && inputImgSize.y == size.y)
		return;
	inputImgSize = {size.x, size.y};
	if(program())
		updateProgramUniforms();
	if(renderTarget_)
		initRenderTargetTexture();
}

void VideoImageEffect::setBitDepth(uint bitDepth)
{
	useRGB565RenderTarget = bitDepth <= 16;
}

Gfx::Program &VideoImageEffect::program()
{
	return prog;
}

Gfx::RenderTarget &VideoImageEffect::renderTarget()
{
	return renderTarget_;
}

void VideoImageEffect::drawRenderTarget(Gfx::PixmapTexture &img)
{
	auto viewport = Gfx::Viewport::makeFromRect({0, 0, (int)renderTargetImgSize.x, (int)renderTargetImgSize.y});
	Gfx::setViewport(viewport);
	Gfx::TextureSampler::bindDefaultNoLinearNoMipClampSampler();
	Gfx::Sprite spr;
	spr.init({-1., -1., 1., 1.}, &img, {0., 1., 1., 0.});
	spr.draw();
	spr.setImg(nullptr);
}
