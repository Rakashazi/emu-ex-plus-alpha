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
#include <gui/View.hh>
#include <EmuOptions.hh>

class EmuView : public View
{
public:
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	Gfx::Sprite menuIcon;
	Rect2<int> menuB, fastForwardB;
	#endif
	bool ffGuiKeyPush = 0, ffGuiTouch = 0;
	VideoImageOverlay vidImgOverlay;
	Gfx::Sprite disp;
	Gfx::BufferImage vidImg;
	Pixmap vidPix {PixelFormatRGB565};
private:
	uchar *pixBuff = nullptr;
	uint vidPixAlign = Gfx::BufferImage::MAX_ASSUME_ALIGN;
	Rect2<int> gameRect_;
	Rect2<GC> gameRectG;
	Rect2<int> rect;

public:
	constexpr EmuView() {}
	void deinit() override {}
	Rect2<int> &viewRect() override { return rect; }
	void place() override;
	void placeEmu(); // game content only
	template <bool active>
	void drawContent();
	void runFrame(Gfx::FrameTimeBase frameTime);
	void draw(Gfx::FrameTimeBase frameTime) override;
	void inputEvent(const Input::Event &e) override;
	void takeGameScreenshot();

	void placeOverlay()
	{
		vidImgOverlay.place(disp, vidPix.y);
	}

	void updateAndDrawContent()
	{
		vidImg.write(vidPix, vidPixAlign);
		drawContent<1>();
	}

	void initPixmap(uchar *pixBuff, const PixelFormatDesc *format, uint x, uint y, uint extraPitch = 0)
	{
		new(&vidPix) Pixmap(*format);
		vidPix.init(pixBuff, x, y, extraPitch);
		var_selfs(pixBuff);
	}

	void reinitImage()
	{
		vidImg.init(vidPix, 0, optionImgFilter);
		disp.setImg(&vidImg);
	}

	void resizeImage(uint x, uint y, uint extraPitch = 0)
	{
		resizeImage(0, 0, x, y, x, y, extraPitch);
	}

	void resizeImage(uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint extraPitch = 0)
	{
		Pixmap basePix(vidPix.format);
		basePix.init(pixBuff, totalX, totalY, extraPitch);
		vidPix.initSubPixmap(basePix, xO, yO, x, y);
		vidImg.init(vidPix, 0, optionImgFilter);
		vidPixAlign = vidImg.bestAlignment(vidPix);
		logMsg("using %d:%d:%d:%d region of %d,%d pixmap for EmuView, aligned to min %d bytes", xO, yO, x, y, totalX, totalY, vidPixAlign);
		disp.setImg(&vidImg);
		if((uint)optionImageZoom > 100)
			placeEmu();
	}

	void initImage(bool force, uint x, uint y, uint extraPitch = 0)
	{
		if(force || !disp.image() || vidPix.x != x || vidPix.y != y)
		{
			resizeImage(x, y, extraPitch);
		}
	}

	void initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint extraPitch = 0)
	{
		if(force || !disp.image() || vidPix.x != x || vidPix.y != y)
		{
			resizeImage(xO, yO, x, y, totalX, totalY, extraPitch);
		}
	}

	const Rect2<int> &gameRect() const
	{
		return gameRect_;
	}
};
