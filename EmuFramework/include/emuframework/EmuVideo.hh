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

#include <emuframework/EmuAppHelper.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/SyncFence.hh>
#include <optional>

namespace Base
{
class ApplicationContext;
}

class EmuVideo;
class EmuSystemTask;

class [[nodiscard]] EmuVideoImage
{
public:
	constexpr EmuVideoImage() {}
	EmuVideoImage(EmuSystemTask *task, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff);
	IG::Pixmap pixmap() const;
	explicit operator bool() const;
	void endFrame();

protected:
	EmuSystemTask *task{};
	EmuVideo *emuVideo{};
	Gfx::LockedTextureBuffer texBuff{};
};

class EmuVideo : public EmuAppHelper<EmuVideo>
{
public:
	using FrameFinishedDelegate = DelegateFunc<void (EmuVideo &)>;
	using FormatChangedDelegate = DelegateFunc<void (EmuVideo &)>;

	constexpr EmuVideo() {}
	void setRendererTask(Gfx::RendererTask &);
	bool hasRendererTask() const;
	bool setFormat(IG::PixmapDesc desc, EmuSystemTask *task = {});
	void dispatchFormatChanged();
	void resetImage();
	IG::PixmapDesc deleteImage();
	EmuVideoImage startFrame(EmuSystemTask *task);
	void startFrame(EmuSystemTask *task, IG::Pixmap pix);
	EmuVideoImage startFrameWithFormat(EmuSystemTask *task, IG::PixmapDesc desc);
	void startFrameWithFormat(EmuSystemTask *task, IG::Pixmap pix);
	void startFrameWithAltFormat(EmuSystemTask *task, IG::Pixmap pix);
	void startUnchangedFrame(EmuSystemTask *task);
	void finishFrame(EmuSystemTask *task, Gfx::LockedTextureBuffer texBuff);
	void finishFrame(EmuSystemTask *task, IG::Pixmap pix);
	void dispatchFrameFinished();
	bool addFence(Gfx::RendererCommands &cmds);
	void clear();
	void takeGameScreenshot();
	bool isExternalTexture() const;
	Gfx::PixmapBufferTexture &image();
	Gfx::Renderer &renderer() const;
	Base::ApplicationContext appContext() const;
	IG::WP size() const;
	bool formatIsEqual(IG::PixmapDesc desc) const;
	void setOnFrameFinished(FrameFinishedDelegate del);
	void setOnFormatChanged(FormatChangedDelegate del);
	bool setTextureBufferMode(Gfx::TextureBufferMode mode);
	bool setImageBuffers(unsigned num);
	unsigned imageBuffers() const;
	void setCompatTextureSampler(const Gfx::TextureSampler &);
	void setSrgbColorSpaceOutput(bool);
	bool isSrgbFormat() const;
	void setRenderPixelFormat(IG::PixelFormat);
	IG::PixelFormat renderPixelFormat() const;
	IG::PixelFormat internalRenderPixelFormat() const;

protected:
	Gfx::RendererTask *rTask{};
	const Gfx::TextureSampler *texSampler{};
	Gfx::SyncFence fence{};
	Gfx::PixmapBufferTexture vidImg{};
	FrameFinishedDelegate onFrameFinished{};
	FormatChangedDelegate onFormatChanged{};
	IG::PixelFormat renderFmt{};
	Gfx::TextureBufferMode bufferMode{};
	bool screenshotNextFrame{};
	bool singleBuffer{};
	bool needsFence{};
	bool useSrgbColorSpace{};
	Gfx::ColorSpace colorSpace_{};

	void doScreenshot(EmuSystemTask *task, IG::Pixmap pix);
	void postFrameFinished(EmuSystemTask *task);
	void syncImageAccess();
	void updateNeedsFence();
};
