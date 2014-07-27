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

#include <EmuVideo.hh>
#include <EmuOptions.hh>
#include <EmuApp.hh>
#include <Screenshot.hh>

void EmuVideo::initPixmap(char *pixBuff, const PixelFormatDesc *format, uint x, uint y, uint pitch)
{
	new(&vidPix) IG::Pixmap(*format);
	if(!pitch)
		vidPix.init(pixBuff, x, y);
	else
		vidPix.init2(pixBuff, x, y, pitch);
	var_selfs(pixBuff);
}

void EmuVideo::reinitImage()
{
	vidImg.init(vidPix, 0);

	// update all EmuVideoLayers
	emuVideoLayer.resetImage();
}

void EmuVideo::resizeImage(uint x, uint y, uint pitch)
{
	resizeImage(0, 0, x, y, x, y, pitch);
}

void EmuVideo::resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	IG::Pixmap basePix(vidPix.format);
	if(pitch)
		basePix.init2(pixBuff, totalX, totalY, pitch);
	else
		basePix.init(pixBuff, totalX, totalY);
	vidPix.initSubPixmap(basePix, xO, yO, x, y);
	vidImg.init(vidPix, 0);
	vidPixAlign = vidImg.bestAlignment(vidPix);
	logMsg("using %d:%d:%d:%d region of %d,%d pixmap for EmuView, aligned to min %d bytes", xO, yO, x, y, totalX, totalY, vidPixAlign);

	// update all EmuVideoLayers
	emuVideoLayer.resetImage();
	if((uint)optionImageZoom > 100)
		placeEmuViews();
}

void EmuVideo::initImage(bool force, uint x, uint y, uint pitch)
{
	if(force || !vidImg || vidPix.x != x || vidPix.y != y)
	{
		resizeImage(x, y, pitch);
	}
}

void EmuVideo::initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch)
{
	if(force || !vidImg || vidPix.x != x || vidPix.y != y)
	{
		resizeImage(xO, yO, x, y, totalX, totalY, pitch);
	}
}

void EmuVideo::takeGameScreenshot()
{
	FsSys::PathString path;
	int screenshotNum = sprintScreenshotFilename(path);
	if(screenshotNum == -1)
	{
		popup.postError("Too many screenshots");
	}
	else
	{
		if(!writeScreenshot(vidPix, path.data()))
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
	return optionSurfaceTexture;
	#else
	return false;
	#endif
}
