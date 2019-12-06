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
	using FrameFinishedDelegate = DelegateFunc<void (EmuVideo &)>;
	using FormatChangedDelegate = DelegateFunc<void (EmuVideo &)>;

	EmuVideo(Gfx::RendererTask &rTask): rTask{rTask} {}
	void setFormat(IG::PixmapDesc desc);
	void setFormatLocked(IG::PixmapDesc desc);
	void resetImage();
	EmuVideoImage startFrame();
	void startFrame(IG::Pixmap pix);
	void finishFrame(Gfx::LockedTextureBuffer texBuff);
	void finishFrame(IG::Pixmap pix);
	void waitAsyncFrame();
	void addFence(Gfx::RendererCommands &cmds);
	void clear();
	void takeGameScreenshot();
	bool isExternalTexture();
	Gfx::PixmapTexture &image();
	Gfx::Renderer &renderer() { return rTask.renderer(); }
	IG::WP size() const;
	bool formatIsEqual(IG::PixmapDesc desc) const;
	void setOnFrameFinished(FrameFinishedDelegate del);
	void setOnFormatChanged(FormatChangedDelegate del);

protected:
	Gfx::RendererTask &rTask;
	Gfx::SyncFence fence{};
	Gfx::PixmapTexture vidImg{};
	IG::MemPixmap memPix{};
	FrameFinishedDelegate onFrameFinished{};
	FormatChangedDelegate onFormatChanged{};
	bool screenshotNextFrame = false;

	void doScreenshot(IG::Pixmap pix);
	void dispatchFinishFrame();
};
