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
#include <imagine/io/sys.hh>

struct EffectDesc
{
	const char *vShaderFilename;
	const char *fShaderFilename;
	IG::Point2D<uint> scale;

	bool needsRenderTarget() const
	{
		return scale.x;
	}
};

static const EffectDesc
	hq2xDesc{"hq2x-v.txt", "hq2x-f.txt", {2, 2}};

static const EffectDesc
	scale2xDesc{"scale2x-v.txt", "scale2x-f.txt", {2, 2}};

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
	iterateTimes(2, i)
	{
		prog[i].deinit();
		if(vShader[i])
		{
			Gfx::deleteShader(vShader[i]);
			vShader[i] = 0;
		}
		if(fShader[i])
		{
			Gfx::deleteShader(fShader[i]);
			fShader[i] = 0;
		}
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
	renderTarget_.initTexture(renderPix, useLinearFilter ? Gfx::BufferImage::LINEAR : Gfx::BufferImage::NEAREST);
}

void VideoImageEffect::compile(bool isExternalTex)
{
	if(hasProgram())
		return; // already compiled
	const EffectDesc *desc = nullptr;
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
		bdefault:
			break;
	}

	if(!desc)
	{
		logErr("effect descriptor not found");
		return;
	}

	if(desc->needsRenderTarget())
	{
		renderTargetScale = desc->scale;
		renderTarget_.init();
		initRenderTargetTexture();
	}
	{
		auto file = IOFile(openAppAssetIo(makeFSPathStringPrintf("shaders/%s", desc->vShaderFilename)));
		if(!file)
		{
			deinit();
			popup.printf(3, true, "Can't open file: %s", desc->vShaderFilename);
			return;
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making vertex shader (replace mode)");
		vShader[0] = Gfx::makePluginVertexShader(text, Gfx::IMG_MODE_REPLACE);
		if(!desc->needsRenderTarget())
		{
			logMsg("making vertex shader (modulate mode)");
			vShader[1] = Gfx::makePluginVertexShader(text, Gfx::IMG_MODE_MODULATE);
		}
		iterateTimes(programs(), i)
		{
			if(!vShader[i])
			{
				deinit();
				popup.postError("GPU rejected shader (vertex compile error)");
				Gfx::autoReleaseShaderCompiler();
				return;
			}
		}
	}
	{
		auto file = IOFile(openAppAssetIo(makeFSPathStringPrintf("shaders/%s", desc->fShaderFilename)));
		if(!file)
		{
			deinit();
			popup.printf(3, true, "Can't open file: %s", desc->fShaderFilename);
			Gfx::autoReleaseShaderCompiler();
			return;
		}
		auto fileSize = file.size();
		char text[fileSize + 1];
		file.read(text, fileSize);
		text[fileSize] = 0;
		file.close();
		//logMsg("read source:\n%s", text);
		logMsg("making fragment shader (replace mode)");
		fShader[0] = Gfx::makePluginFragmentShader(text, Gfx::IMG_MODE_REPLACE, isExternalTex);
		if(!desc->needsRenderTarget())
		{
			logMsg("making fragment shader (modulate mode)");
			fShader[1] = Gfx::makePluginFragmentShader(text, Gfx::IMG_MODE_MODULATE, isExternalTex);
		}
		iterateTimes(programs(), i)
		{
			if(!fShader[i])
			{
				deinit();
				popup.postError("GPU rejected shader (fragment compile error)");
				Gfx::autoReleaseShaderCompiler();
				return;
			}
		}
	}
	iterateTimes(programs(), i)
	{
		logMsg("linking program: %d", i);
		prog[i].init(vShader[i], fShader[i], i == 0, true);
		if(!prog[i].link())
		{
			deinit();
			popup.postError("GPU rejected shader (link error)");
			Gfx::autoReleaseShaderCompiler();
			return;
		}
		srcTexelDeltaU[i] = prog[i].uniformLocation("srcTexelDelta");
		srcTexelHalfDeltaU[i] = prog[i].uniformLocation("srcTexelHalfDelta");
		srcPixelsU[i] = prog[i].uniformLocation("srcPixels");
		updateProgramUniforms();
	}
	Gfx::autoReleaseShaderCompiler();
}

void VideoImageEffect::updateProgramUniforms()
{
	iterateTimes(programs(), i)
	{
		setProgram(prog[i]);
		if(srcTexelDeltaU[i] != -1)
			Gfx::uniformF(srcTexelDeltaU[i], 1.0f / (float)inputImgSize.x, 1.0f / (float)inputImgSize.y);
		if(srcTexelHalfDeltaU[i] != -1)
			Gfx::uniformF(srcTexelHalfDeltaU[i], 0.5f * (1.0f / (float)inputImgSize.x), 0.5f * (1.0f / (float)inputImgSize.y));
		if(srcPixelsU[i] != -1)
			Gfx::uniformF(srcPixelsU[i], inputImgSize.x, inputImgSize.y);
	}
}

void VideoImageEffect::setImageSize(IG::Point2D<uint> size)
{
	if(inputImgSize.x == size.x && inputImgSize.y == size.y)
		return;
	inputImgSize = {size.x, size.y};
	if(hasProgram())
		updateProgramUniforms();
	if(renderTarget_)
		initRenderTargetTexture();
}

void VideoImageEffect::setLinearFilter(bool on)
{
	if(useLinearFilter == on)
		return;
	useLinearFilter = on;
	if(renderTarget_)
	{
		renderTarget_.texture().setLinearFilter(useLinearFilter);
	}
}

void VideoImageEffect::setBitDepth(uint bitDepth)
{
	useRGB565RenderTarget = bitDepth <= 16;
}

Gfx::Program &VideoImageEffect::program(uint imgMode)
{
	return prog[(imgMode == Gfx::IMG_MODE_MODULATE && !renderTarget_) ? 1 : 0];
}

uint VideoImageEffect::programs() const
{
	return renderTarget_ ? 1 : 2;
}

bool VideoImageEffect::hasProgram() const
{
	return prog[0];
}

Gfx::RenderTarget &VideoImageEffect::renderTarget()
{
	return renderTarget_;
}

void VideoImageEffect::drawRenderTarget(Gfx::BufferImage &img)
{
	auto viewport = Gfx::Viewport::makeFromRect({0, 0, (int)renderTargetImgSize.x, (int)renderTargetImgSize.y});
	Gfx::setViewport(viewport);
	Gfx::setProjectionMatrix({});
	Gfx::Sprite spr;
	spr.init(-1., -1., 1., 1.);
	spr.setImg(&img, 0., 1., 1., 0.);
	spr.draw();
	spr.setImg(nullptr);
}
