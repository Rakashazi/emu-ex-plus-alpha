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
#include <emuframework/EmuSystemTask.hh>
#include <emuframework/EmuSystemTaskContext.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/gfx/SyncFence.hh>
#include <optional>

namespace IG
{
class ApplicationContext;
}

namespace EmuEx
{

using namespace IG;
class EmuVideo;
class EmuSystem;

class [[nodiscard]] EmuVideoImage
{
public:
	constexpr EmuVideoImage() {}
	EmuVideoImage(EmuSystemTaskContext taskCtx, EmuVideo &vid, Gfx::LockedTextureBuffer texBuff);
	IG::Pixmap pixmap() const;
	explicit operator bool() const;
	void endFrame();

protected:
	EmuSystemTaskContext taskCtx{};
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
	bool setFormat(IG::PixmapDesc desc, EmuSystemTaskContext task = {});
	void dispatchFormatChanged();
	void resetImage(IG::PixelFormat newFmt = {});
	IG::PixmapDesc deleteImage();
	EmuVideoImage startFrame(EmuSystemTaskContext);
	void startFrame(EmuSystemTaskContext, IG::Pixmap pix);
	EmuVideoImage startFrameWithFormat(EmuSystemTaskContext, IG::PixmapDesc desc);
	void startFrameWithFormat(EmuSystemTaskContext, IG::Pixmap pix);
	void startFrameWithAltFormat(EmuSystemTaskContext, IG::Pixmap pix);
	void startUnchangedFrame(EmuSystemTaskContext);
	void finishFrame(EmuSystemTaskContext, Gfx::LockedTextureBuffer texBuff);
	void finishFrame(EmuSystemTaskContext, IG::Pixmap pix);
	void dispatchFrameFinished();
	bool addFence(Gfx::RendererCommands &cmds);
	void clear();
	void takeGameScreenshot();
	bool isExternalTexture() const;
	Gfx::PixmapBufferTexture &image();
	Gfx::Renderer &renderer() const;
	IG::ApplicationContext appContext() const;
	IG::WP size() const;
	bool formatIsEqual(IG::PixmapDesc desc) const;
	void setOnFrameFinished(FrameFinishedDelegate del);
	void setOnFormatChanged(FormatChangedDelegate del);
	void setTextureBufferMode(EmuSystem &, Gfx::TextureBufferMode mode);
	void setImageBuffers(int num);
	int imageBuffers() const;
	void setCompatTextureSampler(const Gfx::TextureSampler &);
	constexpr auto colorSpace() const { return colSpace; }
	bool setRenderPixelFormat(EmuSystem &, IG::PixelFormat, Gfx::ColorSpace);
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
	Gfx::ColorSpace colSpace{};

	void doScreenshot(EmuSystemTaskContext, IG::Pixmap pix);
	void postFrameFinished(EmuSystemTaskContext);
	void syncImageAccess();
	void updateNeedsFence();
};

}
