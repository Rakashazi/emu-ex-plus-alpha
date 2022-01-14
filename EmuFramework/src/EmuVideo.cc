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

#define LOGTAG "EmuVideo"
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

void EmuVideo::resetImage(IG::PixelFormat newFmt)
{
	if(!vidImg)
		return;
	auto desc = deleteImage();
	if(newFmt)
		setFormat({desc.size(), newFmt});
	else
		setFormat(desc);
	app().renderSystemFramebuffer(*this);
}

IG::PixmapDesc EmuVideo::deleteImage()
{
	auto desc = vidImg.usedPixmapDesc();
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
	return fmt == IG::PIXEL_FMT_RGBA8888 ||
		fmt == IG::PIXEL_FMT_BGRA8888 ||
		fmt == IG::PIXEL_FMT_RGB565;
}

bool EmuVideo::setFormat(IG::PixmapDesc desc, EmuSystemTaskContext taskCtx)
{
	if(formatIsEqual(desc))
	{
		return false; // no change to size/format
	}
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc, texSampler};
		conf.setColorSpace(colSpace);
		vidImg = renderer().makePixmapBufferTexture(conf, bufferMode, singleBuffer);
	}
	else
	{
		vidImg.setFormat(desc, colSpace, texSampler);
	}
	logMsg("resized to:%dx%d", desc.w(), desc.h());
	if(taskCtx)
	{
		taskCtx.task().sendVideoFormatChangedReply(*this, taskCtx.semPtr);
	}
	else
	{
		dispatchFormatChanged();
	}
	return true;
}

void EmuVideo::dispatchFormatChanged()
{
	onFormatChanged(*this);
}

void EmuVideo::syncImageAccess()
{
	rTask->clientWaitSync(std::exchange(fence, {}));
}

EmuVideoImage EmuVideo::startFrame(EmuSystemTaskContext taskCtx)
{
	auto lockedTex = vidImg.lock();
	syncImageAccess();
	return {taskCtx, *this, lockedTex};
}

void EmuVideo::startFrame(EmuSystemTaskContext taskCtx, IG::Pixmap pix)
{
	finishFrame(taskCtx, pix);
}

EmuVideoImage EmuVideo::startFrameWithFormat(EmuSystemTaskContext taskCtx, IG::PixmapDesc desc)
{
	setFormat(desc, taskCtx);
	return startFrame(taskCtx);
}

void EmuVideo::startFrameWithFormat(EmuSystemTaskContext taskCtx, IG::Pixmap pix)
{
	setFormat(pix, taskCtx);
	startFrame(taskCtx, pix);
}

void EmuVideo::startFrameWithAltFormat(EmuSystemTaskContext taskCtx, IG::Pixmap pix)
{
	auto destFmt = renderPixelFormat();
	assumeExpr(isValidRenderFormat(pix.format()));
	if(pix.format() == destFmt)
	{
		startFrameWithFormat(taskCtx, pix);
	}
	else // down-convert to RGB565
	{
		auto img = startFrameWithFormat(taskCtx, {pix.size(), IG::PIXEL_FMT_RGB565});
		assumeExpr(img.pixmap().format() == IG::PIXEL_FMT_RGB565);
		assumeExpr(img.pixmap().size() == pix.size());
		img.pixmap().writeConverted(pix);
		img.endFrame();
	}
}

void EmuVideo::startUnchangedFrame(EmuSystemTaskContext taskCtx)
{
	postFrameFinished(taskCtx);
}

void EmuVideo::dispatchFrameFinished()
{
	//logDMsg("frame finished");
	onFrameFinished(*this);
}

void EmuVideo::postFrameFinished(EmuSystemTaskContext taskCtx)
{
	if(taskCtx)
	{
		taskCtx.task().sendFrameFinishedReply(*this, taskCtx.semPtr);
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

void EmuVideo::finishFrame(EmuSystemTaskContext taskCtx, IG::Pixmap pix)
{
	if(screenshotNextFrame) [[unlikely]]
	{
		doScreenshot(taskCtx, pix);
	}
	syncImageAccess();
	vidImg.write(pix, vidImg.WRITE_FLAG_ASYNC);
	postFrameFinished(taskCtx);
}

bool EmuVideo::addFence(Gfx::RendererCommands &cmds)
{
	if(!needsFence)
		return false;
	fence = cmds.clientWaitSyncReset(fence);
	return true;
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

void EmuVideo::doScreenshot(EmuSystemTaskContext taskCtx, IG::Pixmap pix)
{
	screenshotNextFrame = false;
	auto [screenshotNum, path] = app().makeNextScreenshotFilename();
	if(screenshotNum == -1)
	{
		if(taskCtx)
		{
			taskCtx.task().sendScreenshotReply(-1, false);
		}
		else
		{
			app().printScreenshotResult(-1, false);
		}
	}
	else
	{
		auto success = app().writeScreenshot(pix, path);
		if(taskCtx)
		{
			taskCtx.task().sendScreenshotReply(screenshotNum, success);
		}
		else
		{
			app().printScreenshotResult(screenshotNum, success);
		}
	}
}

bool EmuVideo::isExternalTexture() const
{
	#ifdef __ANDROID__
	return vidImg.isExternal();
	#else
	return false;
	#endif
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

IG::Pixmap EmuVideoImage::pixmap() const
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

IG::WP EmuVideo::size() const
{
	if(!vidImg)
		return {1, 1};
	else
		return vidImg.usedPixmapDesc().size();
}

bool EmuVideo::formatIsEqual(IG::PixmapDesc desc) const
{
	return vidImg && desc == vidImg.usedPixmapDesc();
}

void EmuVideo::setOnFrameFinished(FrameFinishedDelegate del)
{
	onFrameFinished = del;
}

void EmuVideo::setOnFormatChanged(FormatChangedDelegate del)
{
	onFormatChanged = del;
}

void EmuVideo::updateNeedsFence()
{
	needsFence = singleBuffer && renderer().maxSwapChainImages() > 2;
}

void EmuVideo::setTextureBufferMode(Gfx::TextureBufferMode mode)
{
	mode = renderer().makeValidTextureBufferMode(mode);
	if(bufferMode == mode)
		return;
	bufferMode = mode;
	if(renderFmt == IG::PIXEL_RGBA8888 || renderFmt == IG::PIXEL_BGRA8888)
	{
		if(setRenderPixelFormat(IG::PIXEL_RGBA8888, colSpace)) // re-apply format for possible RGB/BGR change
			return;
	}
	resetImage(renderFmt);
}

void EmuVideo::setImageBuffers(int num)
{
	assumeExpr(num < 3);
	if(!num)
	{
		num = renderer().maxSwapChainImages() < 3 || renderer().supportsSyncFences() ? 1 : 2;
	}
	bool useSingleBuffer = num == 1;
	bool modeChanged = singleBuffer != useSingleBuffer;
	singleBuffer = useSingleBuffer;
	updateNeedsFence();
	//logDMsg("image buffer count:%d fences:%s", num, needsFence ? "yes" : "no");
	if(modeChanged && vidImg)
		resetImage();
}

int EmuVideo::imageBuffers() const
{
	return singleBuffer ? 1 : 2;
}

void EmuVideo::setCompatTextureSampler(const Gfx::TextureSampler &compatTexSampler)
{
	texSampler = &compatTexSampler;
	if(!vidImg)
		return;
	vidImg.setCompatTextureSampler(compatTexSampler);
}

bool EmuVideo::setRenderPixelFormat(IG::PixelFormat fmt, Gfx::ColorSpace colorSpace)
{
	if(colorSpace != colSpace)
	{
		colSpace = colorSpace;
		logMsg("set sRGB color space:%s", colorSpace == Gfx::ColorSpace::SRGB ? "on" : "off");
		renderFmt = {}; // reset image
		if(colorSpace == Gfx::ColorSpace::SRGB)
		{
			assert(renderer().supportedColorSpace(fmt, colorSpace) == colorSpace);
		}
	}
	assert(fmt);
	assert(bufferMode != Gfx::TextureBufferMode::DEFAULT);
	if(fmt == IG::PIXEL_RGBA8888 && renderer().hasBgraFormat(bufferMode))
		fmt = IG::PIXEL_BGRA8888;
	if(renderFmt == fmt)
		return false;
	logMsg("setting render pixel format:%s", fmt.name());
	renderFmt = fmt;
	auto oldPixDesc = deleteImage();
	if(!EmuSystem::onVideoRenderFormatChange(*this, fmt) && oldPixDesc.w())
	{
		setFormat({oldPixDesc.size(), fmt});
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
	return renderPixelFormat() == IG::PIXEL_BGRA8888 ? IG::PIXEL_FMT_RGBA8888 : renderPixelFormat();
}

}
