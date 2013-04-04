#pragma once

#include <gfx/GfxSprite.hh>
#include <gfx/GfxBufferImage.hh>
#include <VideoImageOverlay.hh>
#include <gui/View.hh>
#include <EmuOptions.hh>

class EmuView : public View
{
public:
	constexpr EmuView() { }
	Gfx::Sprite disp;
	uchar *pixBuff = nullptr;
	Pixmap vidPix {PixelFormatRGB565};
	uint vidPixAlign = Gfx::BufferImage::MAX_ASSUME_ALIGN;
	Gfx::BufferImage vidImg;
	VideoImageOverlay vidImgOverlay;

	Rect2<int> gameRect;
	Rect2<GC> gameRectG;

	void deinit() { }
	Rect2<int> rect;
	Rect2<int> &viewRect() { return rect; }

	void place();
	void placeEmu(); // game content only
	template <bool active>
	void drawContent();
	void runFrame(Gfx::FrameTimeBase frameTime);
	void draw(Gfx::FrameTimeBase frameTime);
	void inputEvent(const Input::Event &e);

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
		logMsg("using %d:%d:%d:%d region of %d,%d pixmap for EmuView", xO, yO, x, y, totalX, totalY);
		vidImg.init(vidPix, 0, optionImgFilter);
		disp.setImg(&vidImg);
		if((uint)optionImageZoom > 100)
			placeEmu();
	}

	void initImage(bool force, uint x, uint y, uint extraPitch = 0)
	{
		if(force || !disp.img || vidPix.x != x || vidPix.y != y)
		{
			resizeImage(x, y, extraPitch);
		}
	}

	void initImage(bool force, uint xO, uint yO, uint x, uint y, uint totalX, uint totalY, uint extraPitch = 0)
	{
		if(force || !disp.img || vidPix.x != x || vidPix.y != y)
		{
			resizeImage(xO, yO, x, y, totalX, totalY, extraPitch);
		}
	}
};
