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
#include <emuframework/EmuOptions.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/gfx/Vec3.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/container/ArrayList.hh>

namespace EmuEx
{

class EmuInputView;
class EmuVideo;
class EmuSystem;
class VController;

WISE_ENUM_CLASS((ImageChannel, uint8_t),
	All,
	Red,
	Green,
	Blue);

class EmuVideoLayer
{
public:
	EmuVideoLayer(EmuVideo &video, float defaultAspectRatio);
	void place(IG::WindowRect viewRect, IG::WindowRect displayRect, EmuInputView *inputView, EmuSystem &sys);
	void draw(Gfx::RendererCommands &cmds);
	void setRendererTask(Gfx::RendererTask &);
	void setFormat(EmuSystem &, IG::PixelFormat videoFmt, IG::PixelFormat effectFmt, Gfx::ColorSpace);
	void setOverlay(ImageOverlayId id);
	ImageOverlayId overlayEffectId() const { return userOverlayEffectId; }
	void setOverlayIntensity(float intensity);
	float overlayIntensity() const { return vidImgOverlay.intensityLevel(); }
	void setEffect(EmuSystem &, ImageEffectId, IG::PixelFormat);
	ImageEffectId effectId() const { return userEffectId; }
	void updateEffect(EmuSystem &, IG::PixelFormat);
	void setEffectFormat(IG::PixelFormat);
	void setLinearFilter(bool on);
	bool usingLinearFilter() const { return useLinearFilter; }
	void onVideoFormatChanged(IG::PixelFormat effectFmt);
	Gfx::ColorSpace colorSpace() const { return colSpace; }
	bool srgbColorSpace() const { return colSpace == Gfx::ColorSpace::SRGB; }
	void setRotation(IG::Rotation);
	float evalAspectRatio(float aR);
	float channelBrightness(ImageChannel) const;
	int channelBrightnessAsInt(ImageChannel ch) const { return channelBrightness(ch) * 100.f; }
	const Gfx::Vec3 &brightnessAsRGB() const { return brightnessUnscaled; }
	void setBrightness(float brightness, ImageChannel);
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;

	void setBrightnessScale(float s)
	{
		brightnessScale = s;
		updateBrightness();
	}

	const IG::WindowRect &contentRect() const
	{
		return contentRect_;
	}

	EmuVideo &video;
private:
	VideoImageOverlay vidImgOverlay;
	IG::StaticArrayList<VideoImageEffect*, 1> effects;
	VideoImageEffect userEffect;
	Gfx::ITexQuads quad;
	Gfx::TextureSpan texture;
	IG::WindowRect contentRect_;
	Gfx::Vec3 brightness{1.f, 1.f, 1.f};
	Gfx::Vec3 brightnessSrgb{1.f, 1.f, 1.f};
	Gfx::Vec3 brightnessUnscaled{1.f, 1.f, 1.f};
	float brightnessScale;
public:
	float landscapeAspectRatio;
	float portraitAspectRatio;
	int16_t landscapeOffset{};
	int16_t portraitOffset{};
private:
	ImageEffectId userEffectId{};
	ImageOverlayId userOverlayEffectId{};
	Gfx::ColorSpace colSpace{};
public:
	Property<uint8_t, CFGKEY_CONTENT_SCALE, PropertyDesc<uint8_t>{.defaultValue = 100, .isValid = optionContentScaleIsValid}> scale;
private:
	IG::Rotation rotation{};
	bool useLinearFilter{true};

	void placeOverlay();
	void updateEffectImageSize();
	void buildEffectChain();
	bool updateConvertColorSpaceEffect();
	void updateSprite();
	void updateBrightness();
	void logOutputFormat();
	Gfx::Renderer &renderer();
	Gfx::ColorSpace videoColorSpace(IG::PixelFormat videoFmt) const;
	Gfx::TextureSamplerConfig samplerConfig() const;
};

}
