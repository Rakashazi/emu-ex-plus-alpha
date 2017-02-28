#pragma once

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

#include <imagine/gfx/Gfx.hh>
#include <imagine/gfx/Texture.hh>

class EmuVideo;

class EmuVideoImage
{
public:
	EmuVideoImage() {}
	EmuVideoImage(EmuVideo &vid, Gfx::LockedTextureBuffer texBuff):
		emuVideo{&vid}, texBuff{texBuff} {}
	EmuVideoImage(EmuVideo &vid, IG::Pixmap pix):
		emuVideo{&vid}, pix{pix} {}

	IG::Pixmap pixmap() const
	{
		if(texBuff)
			return texBuff.pixmap();
		else
			return pix;
	}

	explicit operator bool() const
	{
		return texBuff || pix;
	}

	void endFrame();

private:
	EmuVideo *emuVideo{};
	Gfx::LockedTextureBuffer texBuff{};
	IG::Pixmap pix{};
};

class EmuVideo
{
public:
	Gfx::Renderer &r;
	Gfx::PixmapTexture vidImg{};
	IG::MemPixmap memPix{};
	bool screenshotNextFrame = false;

	// TODO: remove old API members when all systems updated
	IG::Pixmap vidPix{};
	char *pixBuff{};
	uint vidPixAlign = Gfx::Texture::MAX_ASSUME_ALIGN;

public:
	EmuVideo(Gfx::Renderer &r): r{r} {}
	void initFormat(IG::PixelFormat format);
	void reinitImage();
	void resizeImage(uint x, uint y, uint pitch = 0);
	void initImage(bool force, uint x, uint y, uint pitch = 0);
	EmuVideoImage startFrame();
	void writeFrame(Gfx::LockedTextureBuffer texBuff);
	void writeFrame(IG::Pixmap pix);
	void takeGameScreenshot();
	bool isExternalTexture();
	Gfx::Renderer &renderer() { return r; }

	// TODO: remove old API methods when all systems updated
	void initPixmap(char *pixBuff, IG::PixelFormat format, uint x, uint y, uint pitch = 0);
	void resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch = 0);
	void initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch = 0);
	void clearImage();
	void updateImage();

protected:
	void doScreenshot(IG::Pixmap pix);
};
