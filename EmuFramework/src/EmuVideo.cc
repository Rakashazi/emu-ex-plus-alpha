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
	vidImg.deinit();
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

void EmuVideo::setFormatLocked(IG::PixmapDesc desc)
{
	if(formatIsEqual(desc))
	{
		return; // no change to format
	}
	rTask.lockDraw();
	setFormat(desc);
	rTask.unlockDraw();
}

EmuVideoImage EmuVideo::startFrame()
{
	auto lockedTex = vidImg.lock(0);
	if(!lockedTex)
	{
		if(!memPix)
		{
			logMsg("created backing memory pixmap");
			memPix = {vidImg.usedPixmapDesc()};
		}
		return {*this, (IG::Pixmap)memPix};
	}
	return {*this, lockedTex};
}

void EmuVideo::startFrame(IG::Pixmap pix)
{
	finishFrame(pix);
}

void EmuVideo::dispatchFinishFrame()
{
	renderer().flush();
	onFrameFinished(*this);
}

void EmuVideo::finishFrame(Gfx::LockedTextureBuffer texBuff)
{
	if(unlikely(screenshotNextFrame))
	{
		doScreenshot(texBuff.pixmap());
	}
	rTask.acquireFenceAndWait(fence);
	vidImg.unlock(texBuff);
	dispatchFinishFrame();
}

void EmuVideo::finishFrame(IG::Pixmap pix)
{
	if(unlikely(screenshotNextFrame))
	{
		doScreenshot(pix);
	}
	rTask.acquireFenceAndWait(fence);
	vidImg.write(0, pix, {}, Gfx::Texture::COMMIT_FLAG_ASYNC);
	dispatchFinishFrame();
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

void EmuVideo::doScreenshot(IG::Pixmap pix)
{
	screenshotNextFrame = false;
	FS::PathString path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		EmuApp::postErrorMessage("Too many screenshots");
	}
	else
	{
		auto success = writeScreenshot(pix, path.data());
		EmuApp::printfMessage(2, !success, "%s%d",
			success ? "Wrote screenshot #" : "Error writing screenshot #", screenshotNum);
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

void EmuVideoImage::endFrame()
{
	if(texBuff)
	{
		emuVideo->finishFrame(texBuff);
	}
	else if(pix)
	{
		emuVideo->finishFrame(pix);
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
