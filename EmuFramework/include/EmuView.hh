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

#include <gfx/GfxSprite.hh>
#include <gfx/GfxBufferImage.hh>
#include <VideoImageOverlay.hh>
#include <VideoImageEffect.hh>
#include <gui/View.hh>
#include <EmuOptions.hh>

class EmuView : public View
{
public:
	bool ffGuiKeyPush = 0, ffGuiTouch = 0;
	VideoImageOverlay vidImgOverlay;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	VideoImageEffect vidImgEffect;
	#endif
	Gfx::Sprite disp;
	Gfx::BufferImage vidImg;
	IG::Pixmap vidPix {PixelFormatRGB565};

private:
	char *pixBuff = nullptr;
	uint vidPixAlign = Gfx::BufferImage::MAX_ASSUME_ALIGN;
	IG::WindowRect gameRect_;
	Gfx::GCRect gameRectG;
	IG::WindowRect rect;

public:
	constexpr EmuView(Base::Window &win): View(win) {}
	void deinit() override {}
	IG::WindowRect &viewRect() override { return rect; }
	void place() override;
	void placeEmu(); // game content only
	template <bool active>
	void drawContent();
	void runFrame(Base::FrameTimeBase frameTime);
	void draw(Base::FrameTimeBase frameTime) override;
	void inputEvent(const Input::Event &e) override;
	void takeGameScreenshot();
	void placeOverlay();
	void placeEffect();
	void updateAndDrawContent();
	void compileDefaultPrograms();
	void initPixmap(char *pixBuff, const PixelFormatDesc *format, uint x, uint y, uint pitch = 0);
	void reinitImage();
	void resizeImage(uint x, uint y, uint pitch = 0);
	void resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch = 0);
	void initImage(bool force, uint x, uint y, uint pitch = 0);
	void initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint pitch = 0);

	const IG::WindowRect &gameRect() const
	{
		return gameRect_;
	}
};
