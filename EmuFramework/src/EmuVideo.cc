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

void EmuVideo::resetImage()
{
	if(!vidImg)
		return;
	setFormat(deleteImage());
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

static bool formatSupportsSrgb(IG::PixelFormat fmt)
{
	return fmt == IG::PIXEL_BGRA8888 || fmt == IG::PIXEL_RGBA8888;
}

bool EmuVideo::setFormat(IG::PixmapDesc desc, EmuSystemTask *task)
{
	if(formatIsEqual(desc))
	{
		return false; // no change to size/format
	}
	colorSpace_ = useSrgbColorSpace && formatSupportsSrgb(desc.format()) ? Gfx::ColorSpace::SRGB : Gfx::ColorSpace::LINEAR;
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc, texSampler};
		conf.setColorSpace(colorSpace_);
		vidImg = renderer().makePixmapBufferTexture(conf, bufferMode, singleBuffer);
	}
	else
	{
		vidImg.setFormat(desc, colorSpace_, texSampler);
	}
	logMsg("resized to:%dx%d", desc.w(), desc.h());
	if(task)
	{
		task->sendVideoFormatChangedReply(*this);
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

EmuVideoImage EmuVideo::startFrame(EmuSystemTask *task)
{
	auto lockedTex = vidImg.lock();
	syncImageAccess();
	return {task, *this, lockedTex};
}

void EmuVideo::startFrame(EmuSystemTask *task, IG::Pixmap pix)
{
	finishFrame(task, pix);
}

EmuVideoImage EmuVideo::startFrameWithFormat(EmuSystemTask *task, IG::PixmapDesc desc)
{
	setFormat(desc, task);
	return startFrame(task);
}

void EmuVideo::startFrameWithFormat(EmuSystemTask *task, IG::Pixmap pix)
{
	setFormat(pix, task);
	startFrame(task, pix);
}

void EmuVideo::startFrameWithAltFormat(EmuSystemTask *task, IG::Pixmap pix)
{
	auto destFmt = renderPixelFormat();
	assumeExpr(isValidRenderFormat(pix.format()));
	if(pix.format() == destFmt)
	{
		startFrameWithFormat(task, pix);
	}
	else
	{
		auto img = startFrameWithFormat(task, {pix.size(), destFmt});
		assumeExpr(img.pixmap().format() == destFmt);
		assumeExpr(img.pixmap().size() == pix.size());
		img.pixmap().writeConverted(pix);
		img.endFrame();
	}
}

void EmuVideo::startUnchangedFrame(EmuSystemTask *task)
{
	postFrameFinished(task);
}

void EmuVideo::dispatchFrameFinished()
{
	//logDMsg("frame finished");
	onFrameFinished(*this);
}

void EmuVideo::postFrameFinished(EmuSystemTask *task)
{
	if(task)
	{
		task->sendFrameFinishedReply(*this);
	}
}

void EmuVideo::finishFrame(EmuSystemTask *task, Gfx::LockedTextureBuffer texBuff)
{
	if(screenshotNextFrame) [[unlikely]]
	{
		doScreenshot(task, texBuff.pixmap());
	}
	vidImg.unlock(texBuff);
	postFrameFinished(task);
}

void EmuVideo::finishFrame(EmuSystemTask *task, IG::Pixmap pix)
{
	if(screenshotNextFrame) [[unlikely]]
	{
		doScreenshot(task, pix);
	}
	syncImageAccess();
	vidImg.write(pix, vidImg.WRITE_FLAG_ASYNC);
	postFrameFinished(task);
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

void EmuVideo::doScreenshot(EmuSystemTask *task, IG::Pixmap pix)
{
	screenshotNextFrame = false;
	auto [screenshotNum, path] = app().makeNextScreenshotFilename();
	if(screenshotNum == -1)
	{
		if(task)
		{
			task->sendScreenshotReply(-1, false);
		}
		else
		{
			app().printScreenshotResult(-1, false);
		}
	}
	else
	{
		auto success = app().writeScreenshot(pix, path.data());
		if(task)
		{
			task->sendScreenshotReply(screenshotNum, success);
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

Base::ApplicationContext EmuVideo::appContext() const
{
	return rTask->appContext();
}

EmuVideoImage::EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff):
	task{task}, emuVideo{&vid}, texBuff{texBuff} {}

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
	emuVideo->finishFrame(task, texBuff);
}

IG::WP EmuVideo::size() const
{
	if(!vidImg)
		return {};
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

bool EmuVideo::setTextureBufferMode(Gfx::TextureBufferMode mode)
{
	mode = renderer().makeValidTextureBufferMode(mode);
	bool modeChanged = bufferMode != mode;
	bufferMode = mode;
	if(renderFmt == IG::PIXEL_RGBA8888 || renderFmt == IG::PIXEL_BGRA8888)
	{
		setRenderPixelFormat(IG::PIXEL_RGBA8888); // re-apply format for possible RGB/BGR change
	}
	return modeChanged && vidImg;
}

bool EmuVideo::setImageBuffers(unsigned num)
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
	return modeChanged && vidImg;
}

unsigned EmuVideo::imageBuffers() const
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

void EmuVideo::setSrgbColorSpaceOutput(bool on)
{
	if(on)
	{
		logMsg("enabling sRGB textures");
	}
	useSrgbColorSpace = on;
}

bool EmuVideo::isSrgbFormat() const
{
	return colorSpace_ == Gfx::ColorSpace::SRGB;
}

void EmuVideo::setRenderPixelFormat(IG::PixelFormat fmt)
{
	assert(fmt);
	assert(bufferMode != Gfx::TextureBufferMode::DEFAULT);
	if(fmt == IG::PIXEL_RGBA8888 && renderer().hasBgraFormat(bufferMode))
	{
		fmt = IG::PIXEL_BGRA8888;
	}
	renderFmt = fmt;
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
