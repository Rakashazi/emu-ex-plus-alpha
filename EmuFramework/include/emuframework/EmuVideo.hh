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

#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/SyncFence.hh>

class EmuVideo;
class EmuSystemTask;

class EmuVideoImage
{
public:
	EmuVideoImage();
	EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff);
	IG::Pixmap pixmap() const;
	explicit operator bool() const;
	void endFrame();

private:
	EmuSystemTask *task{};
	EmuVideo *emuVideo{};
	Gfx::LockedTextureBuffer texBuff{};
};

class EmuVideo
{
public:
	using FrameFinishedDelegate = DelegateFunc<void (EmuVideo &)>;
	using FormatChangedDelegate = DelegateFunc<void (EmuVideo &)>;

	constexpr EmuVideo() {}
	void setRendererTask(Gfx::RendererTask &rTask);
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
	bool addFence(Gfx::RendererCommands &cmds);
	void clear();
	void takeGameScreenshot();
	bool isExternalTexture() const;
	Gfx::PixmapBufferTexture &image();
	Gfx::Renderer &renderer() const;
	IG::WP size() const;
	bool formatIsEqual(IG::PixmapDesc desc) const;
	void setOnFrameFinished(FrameFinishedDelegate del);
	void setOnFormatChanged(FormatChangedDelegate del);
	bool setTextureBufferMode(Gfx::TextureBufferMode mode);
	bool setImageBuffers(unsigned num);
	unsigned imageBuffers() const;
	void setCompatTextureSampler(const Gfx::TextureSampler &);

protected:
	Gfx::RendererTask *rTask{};
	const Gfx::TextureSampler *texSampler{};
	Gfx::SyncFence fence{};
	Gfx::PixmapBufferTexture vidImg{};
	FrameFinishedDelegate onFrameFinished{};
	FormatChangedDelegate onFormatChanged{};
	Gfx::TextureBufferMode bufferMode{};
	bool screenshotNextFrame = false;
	bool singleBuffer = false;
	bool needsFence = false;

	void doScreenshot(EmuSystemTask *task, IG::Pixmap pix);
	void dispatchFinishFrame(EmuSystemTask *task);
	void postSetFormat(EmuSystemTask &task, IG::PixmapDesc desc);
	void syncImageAccess();
	void updateNeedsFence();
};
