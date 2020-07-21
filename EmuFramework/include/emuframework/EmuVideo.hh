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

#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/SyncFence.hh>
#include <imagine/pixmap/MemPixmap.hh>

class EmuVideo;
class EmuSystemTask;

class EmuVideoImage
{
public:
	EmuVideoImage();
	EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff);
	EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, IG::Pixmap pix);
	IG::Pixmap pixmap() const;
	explicit operator bool() const;
	void endFrame();

private:
	EmuSystemTask *task{};
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
	void resetImage();
	IG::PixmapDesc deleteImage();
	EmuVideoImage startFrame(EmuSystemTask *task);
	void startFrame(EmuSystemTask *task, IG::Pixmap pix);
	EmuVideoImage startFrameWithFormat(EmuSystemTask *task, IG::PixmapDesc desc);
	void startFrameWithFormat(EmuSystemTask *task, IG::Pixmap pix);
	void startUnchangedFrame(EmuSystemTask *task);
	void finishFrame(EmuSystemTask *task, Gfx::LockedTextureBuffer texBuff);
	void finishFrame(EmuSystemTask *task, IG::Pixmap pix);
	void waitAsyncFrame();
	void addFence(Gfx::RendererCommands &cmds);
	void clear();
	void takeGameScreenshot();
	bool isExternalTexture();
	Gfx::PixmapTexture &image();
	Gfx::Renderer &renderer();
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

	void doScreenshot(EmuSystemTask *task, IG::Pixmap pix);
	void dispatchFinishFrame(EmuSystemTask *task);
	void postSetFormat(EmuSystemTask &task, IG::PixmapDesc desc);
};
