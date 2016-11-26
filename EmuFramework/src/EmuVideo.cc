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
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Screenshot.hh>


void EmuVideo::initPixmap(char *pixBuff, IG::PixelFormat format, uint x, uint y, uint pitch)
{
	if(!pitch)
		vidPix = {{{(int)x, (int)y}, format}, pixBuff};
	else
		vidPix = {{{(int)x, (int)y}, format}, pixBuff, {pitch, vidPix.BYTE_UNITS}};
	this->pixBuff = pixBuff;
}

void EmuVideo::initFormat(IG::PixelFormat format)
{
	vidPix = {{{0, 0}, format}, nullptr};
}

void EmuVideo::reinitImage()
{
	Gfx::TextureConfig conf{vidPix};
	conf.setWillWriteOften(true);
	vidImg.init(conf);

	// update all EmuVideoLayers
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	emuVideoLayer.setEffect(optionImgEffect);
	#else
	emuVideoLayer.resetImage();
	#endif
}

void EmuVideo::clearImage()
{
	if(vidImg)
	{
		vidImg.clear(0);
	}
}

void EmuVideo::resizeImage(uint x, uint y, uint pitch)
{
	resizeImage(0, 0, x, y, x, y, pitch);
}

void EmuVideo::resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	if(pixBuff)
	{
		IG::Pixmap basePix;
		if(pitch)
			basePix = {{{(int)totalX, (int)totalY}, vidPix.format()}, pixBuff, {pitch, vidPix.BYTE_UNITS}};
		else
			basePix = {{{(int)totalX, (int)totalY}, vidPix.format()}, pixBuff};
		vidPix = basePix.subPixmap({(int)xO, (int)yO}, {(int)x, (int)y});
		if(!vidImg)
		{
			reinitImage();
		}
		else if(vidPix != vidImg.usedPixmapDesc())
		{
			vidImg.setFormat(vidPix, 1);
		}
		vidPixAlign = vidImg.bestAlignment(vidPix);
		logMsg("using %d:%d:%d:%d region of %d,%d pixmap for EmuView, aligned to min %d bytes", xO, yO, x, y, totalX, totalY, vidPixAlign);
	}
	else
	{
		vidPix = {{{(int)x, (int)y}, vidPix.format()}, nullptr};
		memPix = {};
		if(!vidImg)
		{
			reinitImage();
		}
		else if(vidPix != vidImg.usedPixmapDesc())
		{
			vidImg.setFormat(vidPix, 1);
		}
		logMsg("resized to:%dx%d", x, y);
	}

	// update all EmuVideoLayers
	emuVideoLayer.resetImage();
	if((uint)optionImageZoom > 100)
		placeEmuViews();
}

void EmuVideo::initImage(bool force, uint x, uint y, uint pitch)
{
	if(force || !vidImg || vidPix.w() != x || vidPix.h() != y)
	{
		resizeImage(x, y, pitch);
	}
}

void EmuVideo::initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	if(force || !vidImg || vidPix.w() != x || vidPix.h() != y)
	{
		resizeImage(xO, yO, x, y, totalX, totalY, pitch);
	}
}

void EmuVideo::updateImage()
{
	if(!vidPix)
	{
		//logMsg("skipping write");
		return;
	}
	vidImg.write(0, vidPix, {}, vidPixAlign);
}

EmuVideoImage EmuVideo::startFrame()
{
	auto lockedTex = vidImg.lock(0);
	if(!lockedTex)
	{
		if(!memPix)
		{
			logMsg("created backing memory pixmap");
			memPix = {vidPix};
			vidPixAlign = vidImg.bestAlignment(memPix);
		}
		return {*this, (IG::Pixmap)memPix};
	}
	return {*this, lockedTex};
}

void EmuVideo::writeFrame(Gfx::LockedTextureBuffer texBuff)
{
	if(screenshotNextFrame)
	{
		doScreenshot(texBuff.pixmap());
	}
	vidImg.unlock(texBuff);
}

void EmuVideo::writeFrame(IG::Pixmap pix)
{
	if(screenshotNextFrame)
	{
		doScreenshot(pix);
	}
	vidImg.write(0, pix, {}, vidImg.bestAlignment(pix));
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
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(pix, path.data()))
		{
			popup.printf(2, 1, "Error writing screenshot #%d", screenshotNum);
		}
		else
		{
			popup.printf(2, 0, "Wrote screenshot #%d", screenshotNum);
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

void EmuVideoImage::endFrame()
{
	if(texBuff)
	{
		emuVideo->writeFrame(texBuff);
	}
	else if(pix)
	{
		emuVideo->writeFrame(pix);
	}
}
