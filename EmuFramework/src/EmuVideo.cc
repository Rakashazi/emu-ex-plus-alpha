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

#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"EmuVideo"};

void EmuVideo::resetImage(IG::PixelFormat newFmt)
{
	if(!vidImg)
		return;
	auto desc = deleteImage();
	if(newFmt)
		setFormat({desc.size, newFmt});
	else
		setFormat(desc);
	app().renderSystemFramebuffer(*this);
}

IG::PixmapDesc EmuVideo::deleteImage()
{
	auto desc = vidImg.pixmapDesc();
	vidImg = {};
	return desc;
}

void EmuVideo::setRendererTask(Gfx::RendererTask &rTask_)
{
	rTask = &rTask_;
}

bool EmuVideo::hasRendererTask() const
{
	return rTask;
}

static bool isValidRenderFormat(IG::PixelFormat fmt)
{
	return fmt == IG::PixelFmtRGBA8888 ||
		fmt == IG::PixelFmtBGRA8888 ||
		fmt == IG::PixelFmtRGB565;
}

bool EmuVideo::setFormat(IG::PixmapDesc desc, EmuSystemTaskContext taskCtx)
{
	if(formatIsEqual(desc))
	{
		return false; // no change to size/format
	}
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc, samplerConfig()};
		conf.colorSpace = colSpace;
		bool singleBuffer = renderer().maxSwapChainImages() < 3 || app().effectiveFrameTimeSource() != FrameTimeSource::Renderer;
		vidImg = renderer().makePixmapBufferTexture(conf, bufferMode, singleBuffer);
	}
	else
	{
		vidImg.setFormat(desc, colSpace, samplerConfig());
	}
	log.info("resized to:{}x{}", desc.w(), desc.h());
	if(taskCtx)
	{
		taskCtx.task().sendVideoFormatChangedReply(*this);
	}
	else
	{
		dispatchFormatChanged();
	}
	return true;
}

EmuVideoImage EmuVideo::startFrame(EmuSystemTaskContext taskCtx)
{
	auto lockedTex = vidImg.lock();
	return {taskCtx, *this, lockedTex};
}

void EmuVideo::startFrame(EmuSystemTaskContext taskCtx, IG::PixmapView pix)
{
	finishFrame(taskCtx, pix);
}

EmuVideoImage EmuVideo::startFrameWithFormat(EmuSystemTaskContext taskCtx, IG::PixmapDesc desc)
{
	setFormat(desc, taskCtx);
	return startFrame(taskCtx);
}

void EmuVideo::startFrameWithFormat(EmuSystemTaskContext taskCtx, IG::PixmapView pix)
{
	setFormat(pix.desc(), taskCtx);
	startFrame(taskCtx, pix);
}

void EmuVideo::startFrameWithAltFormat(EmuSystemTaskContext taskCtx, IG::PixmapView pix)
{
	auto destFmt = renderPixelFormat();
	auto srcFmt = pix.format();
	assumeExpr(isValidRenderFormat(srcFmt));
	if(srcFmt == destFmt)
	{
		startFrameWithFormat(taskCtx, pix);
	}
	else // down-convert to RGB565
	{
		auto img = startFrameWithFormat(taskCtx, {pix.size(), IG::PixelFmtRGB565});
		assumeExpr(img.pixmap().format() == IG::PixelFmtRGB565);
		assumeExpr(img.pixmap().size() == pix.size());
		img.pixmap().writeConverted(pix);
		img.endFrame();
	}
}

void EmuVideo::startUnchangedFrame(EmuSystemTaskContext taskCtx)
{
	postFrameFinished(taskCtx);
}

void EmuVideo::postFrameFinished(EmuSystemTaskContext taskCtx)
{
	if(taskCtx)
	{
		taskCtx.task().sendFrameFinishedReply(*this);
	}
}

void EmuVideo::finishFrame(EmuSystemTaskContext taskCtx, Gfx::LockedTextureBuffer texBuff)
{
	if(screenshotNextFrame) [[unlikely]]
	{
		doScreenshot(taskCtx, texBuff.pixmap());
	}
	vidImg.unlock(texBuff);
	postFrameFinished(taskCtx);
}

void EmuVideo::finishFrame(EmuSystemTaskContext taskCtx, IG::PixmapView pix)
{
	if(screenshotNextFrame) [[unlikely]]
	{
		doScreenshot(taskCtx, pix);
	}
	vidImg.write(pix, {.async = true});
	postFrameFinished(taskCtx);
}

void EmuVideo::clear()
{
	if(!vidImg)
		return;
	vidImg.clear();
}

void EmuVideo::takeGameScreenshot()
{
	screenshotNextFrame = true;
}

void EmuVideo::doScreenshot(EmuSystemTaskContext taskCtx, IG::PixmapView pix)
{
	screenshotNextFrame = false;
	auto success = app().writeScreenshot(pix, app().makeNextScreenshotFilename());
	if(taskCtx)
	{
		taskCtx.task().sendScreenshotReply(success);
	}
	else
	{
		app().printScreenshotResult(success);
	}
}

bool EmuVideo::isExternalTexture() const
{
	if constexpr(Config::envIsAndroid)
		return vidImg.isExternal();
	else
		return false;
}

Gfx::PixmapBufferTexture &EmuVideo::image()
{
	return vidImg;
}

Gfx::Renderer &EmuVideo::renderer() const
{
	return rTask->renderer();
}

IG::ApplicationContext EmuVideo::appContext() const
{
	return rTask->appContext();
}

EmuVideoImage::EmuVideoImage(EmuSystemTaskContext taskCtx, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff):
	taskCtx{taskCtx}, emuVideo{&vid}, texBuff{texBuff} {}

IG::MutablePixmapView EmuVideoImage::pixmap() const
{
	return texBuff.pixmap();
}

EmuVideoImage::operator bool() const
{
	return (bool)texBuff;
}

void EmuVideoImage::endFrame()
{
	assumeExpr(texBuff);
	emuVideo->finishFrame(taskCtx, texBuff);
}

WSize EmuVideo::size() const
{
	if(!vidImg)
		return {1, 1};
	else
		return vidImg.pixmapDesc().size;
}

bool EmuVideo::formatIsEqual(IG::PixmapDesc desc) const
{
	return vidImg && desc == vidImg.pixmapDesc();
}

void EmuVideo::setTextureBufferMode(EmuSystem &sys, Gfx::TextureBufferMode mode)
{
	mode = renderer().evalTextureBufferMode(mode);
	if(bufferMode == mode)
		return;
	bufferMode = mode;
	if(renderFmt == IG::PixelFmtRGBA8888 || renderFmt == IG::PixelFmtBGRA8888)
	{
		if(setRenderPixelFormat(sys, IG::PixelFmtRGBA8888, colSpace)) // re-apply format for possible RGB/BGR change
			return;
	}
	resetImage(renderFmt);
}

void EmuVideo::setSampler(Gfx::TextureSamplerConfig samplerConf)
{
	useLinearFilter = samplerConf.minLinearFilter;
	vidImg.setSampler(samplerConf);
}

bool EmuVideo::setRenderPixelFormat(EmuSystem &sys, IG::PixelFormat fmt, Gfx::ColorSpace colorSpace)
{
	if(colorSpace != colSpace)
	{
		colSpace = colorSpace;
		log.info("set sRGB color space:{}", colorSpace == Gfx::ColorSpace::SRGB ? "on" : "off");
		renderFmt = {}; // reset image
		if(colorSpace == Gfx::ColorSpace::SRGB)
		{
			assert(renderer().supportedColorSpace(fmt, colorSpace) == colorSpace);
		}
	}
	assert(fmt);
	assert(bufferMode != Gfx::TextureBufferMode::DEFAULT);
	if(fmt == IG::PixelFmtRGBA8888 && renderer().hasBgraFormat(bufferMode))
		fmt = IG::PixelFmtBGRA8888;
	if(renderFmt == fmt)
		return false;
	log.info("setting render pixel format:{}", fmt.name());
	renderFmt = fmt;
	auto oldPixDesc = deleteImage();
	if(!sys.onVideoRenderFormatChange(*this, fmt) && oldPixDesc.w())
	{
		setFormat({oldPixDesc.size, fmt});
	}
	app().renderSystemFramebuffer(*this);
	return true;
}

IG::PixelFormat EmuVideo::renderPixelFormat() const
{
	assumeExpr(isValidRenderFormat(renderFmt));
	return renderFmt;
}

IG::PixelFormat EmuVideo::internalRenderPixelFormat() const
{
	return renderPixelFormat() == IG::PixelFmtBGRA8888 ? IG::PixelFmtRGBA8888 : renderPixelFormat();
}

Gfx::TextureSamplerConfig EmuVideo::samplerConfigForLinearFilter(bool useLinearFilter)
{
	return useLinearFilter ? Gfx::SamplerConfigs::noMipClamp : Gfx::SamplerConfigs::noLinearNoMipClamp;
}

MutablePixmapView EmuVideo::takeInterlacedFields(MutablePixmapView pix, bool isOddField)
{
	// Take half the image on the Y axis and double the pitch to isolate the even/odd fields
	WSize size = {pix.w(), pix.h() / 2};
	return {pix.desc().makeNewSize(size), pix.data({0, isOddField}), {pix.pitchBytes() * 2, PixmapUnits::BYTE}};
}

}
