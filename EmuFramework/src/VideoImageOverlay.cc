/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <VideoImageOverlay.hh>
#include <EmuSystem.hh>
#include <meta.h>

#define CONV_COL(x) 0, x
static ATTRS(aligned (2)) uchar scanlinePixmapBuff[] = { CONV_COL(0x00), CONV_COL(0xff) };
static ATTRS(aligned (8)) uchar diagonalPixmapBuff[] =
{
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff),
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff),
};
#undef CONV_COL

#define CONV_COL(x) 31, x
static ATTRS(aligned (8)) uchar crtPixmapBuff[] =
{
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
		CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0x00), CONV_COL(0xff), CONV_COL(0x00),
		CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff), CONV_COL(0xff),
};
#undef CONV_COL

//#define CONV_COL(r,g,b) ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
#define CONV_COL(r,g,b) uint((r << 24) | (g << 16) | (b << 8) | 127)
static ATTRS(aligned (8)) uint32 crtRgbPixmapBuff[] =
{
		CONV_COL(0xcc,0,0x32), CONV_COL(0xff,0,0), CONV_COL(0xcb,0x33,0), CONV_COL(0x98,0x66,0), CONV_COL(0x65,0x99,0), CONV_COL(0x32,0xcc,0), CONV_COL(0,0xff,0), CONV_COL(0,0xcb,0x33), CONV_COL(0,0x98,0x66), CONV_COL(0,0x65,0x99), CONV_COL(0,0x32,0xcc), CONV_COL(0,0,0xff), CONV_COL(0x33,0,0xcb), CONV_COL(0x66,0,0x98), CONV_COL(0x99,0,0x65), CONV_COL(0xcb,0,0x33),
		CONV_COL(0,0x98,0x66), CONV_COL(0,0x65,0x99), CONV_COL(0,0x32,0xcc), CONV_COL(0,0,0xff), CONV_COL(0x33,0,0xcb), CONV_COL(0x66,0,0x98), CONV_COL(0x99,0,0x65), CONV_COL(0xcb,0,0x33), CONV_COL(0xcc,0,0x32), CONV_COL(0xff,0,0), CONV_COL(0xcb,0x33,0), CONV_COL(0x98,0x66,0), CONV_COL(0x65,0x99,0), CONV_COL(0x32,0xcc,0), CONV_COL(0,0xff,0), CONV_COL(0,0xcb,0x33),
		/*CONV_COL(0xFF,0,0), CONV_COL(0,0xFF,0), CONV_COL(0,0xFF,0), CONV_COL(0,0xFF,0), CONV_COL(0,0,0xFF), CONV_COL(0,0,0xFF), CONV_COL(0,0,0xFF), CONV_COL(0xFF,0,0),
		CONV_COL(0,0,0xFF), CONV_COL(0,0,0xFF), CONV_COL(0,0,0xFF), CONV_COL(0xFF,0,0), CONV_COL(0xFF,0,0), CONV_COL(0,0xFF,0), CONV_COL(0,0xFF,0), CONV_COL(0,0xFF,0),*/
};
#undef CONV_COL

void VideoImageOverlay::setEffect(uint effect)
{
	var_selfs(effect);
	switch(effect)
	{
		bcase SCANLINES ... SCANLINES_2:
			pix.init(scanlinePixmapBuff, &PixelFormatIA88, 1, 2);
		bcase CRT:
			pix.init(crtPixmapBuff, &PixelFormatIA88, 8, 8);
		bcase CRT_RGB ... CRT_RGB_2:
			pix.init((uchar*)crtRgbPixmapBuff, &PixelFormatRGBA8888, 16, 2);//8, 2);
		bdefault: // turn off effect
			if(spr.img)
				spr.deinitAndFreeImg();
			return;
	}

	bool mipmapFilter = 1;
	img.init(pix, 0, GfxBufferImage::linear, mipmapFilter ? 0 : GfxBufferImage::HINT_NO_MINIFY, 1);
	spr.init(&img);
	img.write(pix);
}

void VideoImageOverlay::place(const GfxSprite &disp)
{
	if(spr.img)
	{
		spr.setPos(disp);
		// TODO: get line count directly from pixmap
		uint lines = 224;
		if(string_equal(CONFIG_APP_NAME, "2600.emu"))
			lines = 210;
		else if(string_equal(CONFIG_APP_NAME, "PCE.emu"))
			lines = 232;
		else if(string_equal(CONFIG_APP_NAME, "GBC.emu"))
			lines = 144;
		else if(string_equal(CONFIG_APP_NAME, "NGP.emu"))
			lines = 152;
		float width = lines*(EmuSystem::aspectRatioX/(float)EmuSystem::aspectRatioY);
		//logMsg("width %f", (double)width);
		switch(effect)
		{
			bcase SCANLINES:
				spr.setImg(&img, 0, 0, 1.0, lines);
			bcase SCANLINES_2:
				spr.setImg(&img, 0, 0, 1.0, lines*2.);
			bcase CRT:
				spr.setImg(&img, 0, 0, width/2., lines/2.);
			bcase CRT_RGB:
				spr.setImg(&img, 0, 0, width/2., lines);
			bcase CRT_RGB_2:
				spr.setImg(&img, 0, 0, width/2., lines*2.);
		}
	}
}

void VideoImageOverlay::draw()
{
	using namespace Gfx;
	if(spr.img)
	{
		setColor(1., 1., 1., intensity);
		setImgMode(IMG_MODE_MODULATE);
		setBlendMode(BLEND_MODE_ALPHA);
		spr.draw(0);
	}
}
