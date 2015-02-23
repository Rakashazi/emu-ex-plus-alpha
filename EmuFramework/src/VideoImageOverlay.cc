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
#include <emuframework/EmuSystem.hh>

#define CONV_COL(x) 0, x
alignas(2) static uint8 scanlinePixmapBuff[] = { CONV_COL(0x00), CONV_COL(0xff) };
alignas(8) static uint8 diagonalPixmapBuff[] =
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
alignas(8) static uint8 crtPixmapBuff[] =
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
alignas(8) static uint32 crtRgbPixmapBuff[] =
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
			new(&pix) IG::Pixmap(PixelFormatIA88);
			pix.init(scanlinePixmapBuff, 1, 2);
		bcase CRT:
			new(&pix) IG::Pixmap(PixelFormatIA88);
			pix.init(crtPixmapBuff, 8, 8);
		bcase CRT_RGB ... CRT_RGB_2:
			new(&pix) IG::Pixmap(PixelFormatRGBA8888);
			pix.init(crtRgbPixmapBuff, 16, 2);//8, 2);
		bdefault: // turn off effect
			spr.deinit();
			img.deinit();
			return;
	}
	Gfx::TextureSampler::initDefaultNearestMipRepeatSampler();
	Gfx::TextureConfig texConf{pix};
	texConf.setWillGenerateMipmaps(true);
	img.init(texConf);
	img.write(0, pix, {});
	img.generateMipmaps();
	spr.init({}, &img, {});
	if(spr.compileDefaultProgram(Gfx::IMG_MODE_MODULATE))
	{
		Gfx::autoReleaseShaderCompiler();
	}
}

void VideoImageOverlay::place(const Gfx::Sprite &disp, uint lines)
{
	if(spr.image())
	{
		using namespace Gfx;
		spr.setPos(disp);
		Gfx::GTexC width = lines*(EmuSystem::aspectRatioInfo[0].aspect.x/(float)EmuSystem::aspectRatioInfo[0].aspect.y);
		//logMsg("width %f", (double)width);
		switch(effect)
		{
			bcase SCANLINES:
				spr.setImg(&img, {0., 0., 1.0, (Gfx::GTexC)lines});
			bcase SCANLINES_2:
				spr.setImg(&img, {0., 0., 1.0, lines*2._gtexc});
			bcase CRT:
				spr.setImg(&img, {0., 0., width/2._gtexc, lines/2._gtexc});
			bcase CRT_RGB:
				spr.setImg(&img, {0., 0., width/2._gtexc, (Gfx::GTexC)lines});
			bcase CRT_RGB_2:
				spr.setImg(&img, {0., 0., width/2._gtexc, lines*2._gtexc});
		}
	}
}

void VideoImageOverlay::draw()
{
	using namespace Gfx;
	if(spr.image())
	{
		TextureSampler::bindDefaultNearestMipRepeatSampler();
		setColor(1., 1., 1., intensity);
		setBlendMode(BLEND_MODE_ALPHA);
		spr.useDefaultProgram(IMG_MODE_MODULATE);
		spr.draw();
	}
}
