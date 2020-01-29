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
#include "EmuOptions.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/Screenshot.hh>
#include "private.hh"

void EmuVideo::resetImage()
{
	rTask.waitForDrawFinished();
	renderer().deleteSyncFence(fence);
	fence = {};
	auto desc = vidImg.usedPixmapDesc();
	vidImg = {};
	setFormat(desc);
}

void EmuVideo::setFormat(IG::PixmapDesc desc)
{
	if(formatIsEqual(desc))
	{
		return; // no change to format
	}
	if(memPix)
	{
		renderer().waitAsyncCommands();
		memPix = {};
	}
	if(!vidImg)
	{
		Gfx::TextureConfig conf{desc};
		conf.setWillWriteOften(true);
		vidImg = renderer().makePixmapTexture(conf);
		vidImg.clear(0);
	}
	else
	{
		vidImg.setFormat(desc, 1);
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
	task.sendVideoFormatChangedReply(desc, &sem);
	sem.wait();
}

EmuVideoImage EmuVideo::startFrame(EmuSystemTask *task)
{
	auto lockedTex = vidImg.lock(0);
	if(!lockedTex)
	{
		if(unlikely(!memPix))
		{
			logMsg("created backing memory pixmap");
			memPix = {vidImg.usedPixmapDesc()};
		}
		return {task, *this, (IG::Pixmap)memPix};
	}
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

void EmuVideo::dispatchFinishFrame(EmuSystemTask *task)
{
	if(task)
		task->finishVideoFrame();
	onFrameFinished(*this);
}

void EmuVideo::finishFrame(EmuSystemTask *task, Gfx::LockedTextureBuffer texBuff)
{
	if(unlikely(screenshotNextFrame))
	{
		doScreenshot(task, texBuff.pixmap());
	}
	rTask.acquireFenceAndWait(fence);
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
	vidImg.write(0, pix, {}, Gfx::Texture::COMMIT_FLAG_ASYNC);
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
	vidImg.clear(0);
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

Gfx::PixmapTexture &EmuVideo::image()
{
	return vidImg;
}

EmuVideoImage::EmuVideoImage() {}

EmuVideoImage::EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff):
	task{task}, emuVideo{&vid}, texBuff{texBuff} {}

EmuVideoImage::EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, IG::Pixmap pix):
	task{task}, emuVideo{&vid}, pix{pix} {}

IG::Pixmap EmuVideoImage::pixmap() const
{
	if(texBuff)
		return texBuff.pixmap();
	else
		return pix;
}

EmuVideoImage::operator bool() const
{
	return texBuff || pix;
}


void EmuVideoImage::endFrame()
{
	if(texBuff)
	{
		emuVideo->finishFrame(task, texBuff);
	}
	else if(pix)
	{
		emuVideo->finishFrame(task, pix);
	}
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
