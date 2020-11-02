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
#include <emuframework/Screenshot.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include "EmuSystemTask.hh"

void EmuVideo::resetImage()
{
	setFormat(deleteImage());
}

IG::PixmapDesc EmuVideo::deleteImage()
{
	rTask.waitForDrawFinished();
	renderer().deleteSyncFence(fence);
	fence = {};
	auto desc = vidImg.usedPixmapDesc();
	vidImg = {};
	return desc;
}

void EmuVideo::setFormat(IG::PixmapDesc desc)
{
	if(formatIsEqual(desc))
	{
		return; // no change to format
	}
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc};
		vidImg = renderer().makePixmapBufferTexture(conf, bufferMode, true);
		vidImg.clear();
	}
	else
	{
		vidImg.setFormat(desc);
	}
	logMsg("resized to:%dx%d", desc.w(), desc.h());
	onFormatChanged(*this);
}

void EmuVideo::postSetFormat(EmuSystemTask &task, IG::PixmapDesc desc)
{
	if(formatIsEqual(desc))
	{
		return; // no change to format
	}
	IG::Semaphore sem{0};
	task.sendVideoFormatChangedReply(*this, desc, &sem);
	sem.wait();
}

EmuVideoImage EmuVideo::startFrame(EmuSystemTask *task)
{
	auto lockedTex = vidImg.lock();
	rTask.acquireFenceAndWait(fence);
	return {task, *this, lockedTex};
}

void EmuVideo::startFrame(EmuSystemTask *task, IG::Pixmap pix)
{
	finishFrame(task, pix);
}

EmuVideoImage EmuVideo::startFrameWithFormat(EmuSystemTask *task, IG::PixmapDesc desc)
{
	if(task)
	{
		postSetFormat(*task, desc);
	}
	else
	{
		setFormat(desc);
	}
	return startFrame(task);
}

void EmuVideo::startFrameWithFormat(EmuSystemTask *task, IG::Pixmap pix)
{
	if(task)
	{
		postSetFormat(*task, pix);
	}
	else
	{
		setFormat(pix);
	}
	startFrame(task, pix);
}

void EmuVideo::startUnchangedFrame(EmuSystemTask *task)
{
	dispatchFinishFrame(task);
}

void EmuVideo::dispatchFinishFrame(EmuSystemTask *task)
{
	onFrameFinished(*this);
}

void EmuVideo::finishFrame(EmuSystemTask *task, Gfx::LockedTextureBuffer texBuff)
{
	if(unlikely(screenshotNextFrame))
	{
		doScreenshot(task, texBuff.pixmap());
	}
	vidImg.unlock(texBuff);
	dispatchFinishFrame(task);
}

void EmuVideo::finishFrame(EmuSystemTask *task, IG::Pixmap pix)
{
	if(unlikely(screenshotNextFrame))
	{
		doScreenshot(task, pix);
	}
	rTask.acquireFenceAndWait(fence);
	vidImg.write(pix, vidImg.WRITE_FLAG_ASYNC);
	dispatchFinishFrame(task);
}

void EmuVideo::waitAsyncFrame()
{
	renderer().waitAsyncCommands();
}

void EmuVideo::addFence(Gfx::RendererCommands &cmds)
{
	fence = cmds.replaceSyncFence(fence);
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
	FS::PathString path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		if(task)
		{
			task->sendScreenshotReply(-1, false);
		}
		else
		{
			EmuApp::printScreenshotResult(-1, false);
		}
	}
	else
	{
		auto success = writeScreenshot(pix, path.data());
		if(task)
		{
			task->sendScreenshotReply(screenshotNum, success);
		}
		else
		{
			EmuApp::printScreenshotResult(screenshotNum, success);
		}
	}
}

bool EmuVideo::isExternalTexture()
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

Gfx::Renderer &EmuVideo::renderer()
{
	return rTask.renderer();
}

EmuVideoImage::EmuVideoImage() {}

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

bool EmuVideo::setTextureBufferMode(Gfx::TextureBufferMode mode)
{
	mode = renderer().makeValidTextureBufferMode(mode);
	bool modeChanged = bufferMode != mode;
	bufferMode = mode;
	return modeChanged && vidImg;
}
