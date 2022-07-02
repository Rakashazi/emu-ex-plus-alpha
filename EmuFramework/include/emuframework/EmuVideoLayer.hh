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

#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/VideoImageEffect.hh>
#include <imagine/gfx/GfxSprite.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

class EmuInputView;
class EmuVideo;
class EmuSystem;
class VController;

class EmuVideoLayer
{
public:
	EmuVideoLayer(EmuVideo &video);
	void place(IG::WindowRect viewRect, IG::WindowRect displayRect, Gfx::ProjectionPlane projP, EmuInputView *inputView, EmuSystem &sys);
	void draw(Gfx::RendererCommands &cmds, const Gfx::ProjectionPlane &projP);
	void setFormat(EmuSystem &, IG::PixelFormat videoFmt, IG::PixelFormat effectFmt, Gfx::ColorSpace);
	void setOverlay(ImageOverlayId id);
	void setOverlayIntensity(float intensity);
	void setEffect(EmuSystem &, ImageEffectId, IG::PixelFormat);
	void setEffectFormat(IG::PixelFormat);
	void setLinearFilter(bool on);
	void setBrightness(float b);
	void setAspectRatio(double ratio) { aspectRatio_ = ratio; }
	auto aspectRatio() { return aspectRatio_; }
	void onVideoFormatChanged(IG::PixelFormat effectFmt);
	EmuVideo &emuVideo() const { return video; }
	Gfx::ColorSpace colorSpace() const { return colSpace; }
	bool srgbColorSpace() const { return colSpace == Gfx::ColorSpace::SRGB; }
	void setZoom(uint8_t val) { zoom_ = val; }
	auto zoom() const { return zoom_; }

	const IG::WindowRect &gameRect() const
	{
		return gameRect_;
	}

private:
	VideoImageOverlay vidImgOverlay{};
	IG::StaticArrayList<VideoImageEffect*, 1> effects{};
	EmuVideo &video;
	const Gfx::TextureSampler *texSampler{};
	VideoImageEffect userEffect{};
	Gfx::Sprite disp{};
	IG::WindowRect gameRect_{};
	Gfx::GCRect gameRectG{};
	double aspectRatio_ = 1.;
	float brightness = 1.f;
	float brightnessSrgb = 1.f;
	ImageEffectId userEffectId{};
	ImageOverlayId userOverlayEffectId{};
	Gfx::ColorSpace colSpace{};
	uint8_t zoom_{100};

	void placeOverlay();
	void updateEffectImageSize();
	void buildEffectChain();
	bool updateConvertColorSpaceEffect();
	void updateSprite();
	void logOutputFormat();
	Gfx::Renderer &renderer();
	Gfx::ColorSpace videoColorSpace(IG::PixelFormat videoFmt) const;
};

}
