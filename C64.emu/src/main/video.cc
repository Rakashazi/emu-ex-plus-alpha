/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "video"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "internal.hh"

extern "C"
{
	#include "palette.h"
	#include "video.h"
	#include "videoarch.h"
	#include "kbdbuf.h"
	#include "sound.h"
	#include "vsync.h"
	#include "vsyncapi.h"
}

struct video_canvas_s *activeCanvas{};
IG::Pixmap canvasSrcPix{};
double systemFrameRate = 60.0;
static std::atomic_bool runningFrame{};

void setCanvasSkipFrame(bool on)
{
	activeCanvas->skipFrame = on;
}

void startCanvasRunningFrame()
{
	runningFrame = true;
}

CLINK LVISIBLE int vsync_do_vsync2(struct video_canvas_s *c, int been_skipped);
int vsync_do_vsync2(struct video_canvas_s *c, int been_skipped)
{
	if(runningFrame) [[likely]]
	{
		//logMsg("vsync_do_vsync signaling main thread");
		runningFrame = false;
		execDoneSem.notify();
		execSem.wait();
	}
	else
	{
		logMsg("spurious vsync_do_vsync()");
	}
	return c->skipFrame;
}

void vsyncarch_refresh_frequency_changed(double rate)
{
	logMsg("system frame rate:%.4f", rate);
	systemFrameRate = rate;
	EmuApp::get(appContext).configFrameTime();
}

static IG::Pixmap makePixmapView(const struct video_canvas_s *c)
{
	return {{{c->w, c->h}, (IG::PixelFormatID)c->pixelFormat}, c->pixmapData};
}

static IG::Pixmap releasePixmapView(struct video_canvas_s *c)
{
	auto pix = makePixmapView(c);
	c->pixmapData = {};
	return pix;
}

static IG::PixelDesc pixelDesc(IG::PixelFormat fmt)
{
	return fmt == IG::PIXEL_FMT_RGBA8888 ? fmt.desc().nativeOrder() : fmt.desc();
}

void video_arch_canvas_init(struct video_canvas_s *c)
{
	logMsg("created canvas with size %d,%d", c->draw_buffer->canvas_width, c->draw_buffer->canvas_height);
	c->video_draw_buffer_callback = nullptr;
	c->bpp = pixelDesc(pixFmt).bitsPerPixel();
	assert(c->bpp == 16 || c->bpp == 32);
	activeCanvas = c;
}

int video_canvas_set_palette(video_canvas_t *c, struct palette_s *palette)
{
	const auto pDesc = pixelDesc(pixFmt);
	iterateTimes(256, i)
	{
		plugin.video_render_setrawrgb(i, pDesc.build(i/255., 0., 0., 0.), pDesc.build(0., i/255., 0., 0.), pDesc.build(0., 0., i/255., 0.));
	}
	plugin.video_render_initraw(c->videoconfig);

	if(palette)
	{
		c->palette = palette;
		iterateTimes(palette->num_entries, i)
		{
			auto col = pDesc.build(palette->entries[i].red/255., palette->entries[i].green/255., palette->entries[i].blue/255., 0.);
			logMsg("set color %d to %X (%d bpp)", i, col, c->bpp);
			plugin.video_render_setphysicalcolor(c->videoconfig, i, col, c->bpp);
		}
	}

	return 0;
}

void video_canvas_refresh(struct video_canvas_s *c, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
	xi *= c->videoconfig->scalex;
	w *= c->videoconfig->scalex;
	yi *= c->videoconfig->scaley;
	h *= c->videoconfig->scaley;
	auto pixView = makePixmapView(c);

	w = std::min(w, pixView.w());
	h = std::min(h, pixView.h());

	plugin.video_canvas_render(c, (uint8_t*)pixView.data(), w, h, xs, ys, xi, yi, pixView.pitchBytes(), c->bpp);
}

void resetCanvasSourcePixmap(struct video_canvas_s *c)
{
	unsigned canvasW = c->w;
	unsigned canvasH = c->h;
	if(optionCropNormalBorders && (canvasH == 247 || canvasH == 272))
	{
		logMsg("cropping borders");
		// Crop all vertical borders on NTSC, leaving leftover side borders
		int xBorderSize = 32, yBorderSize = 23;
		int height = 200;
		int startX = yBorderSize, startY = yBorderSize;
		if(canvasH == 272) // PAL
		{
			// Crop all horizontal borders on PAL, leaving leftover top/bottom borders
			yBorderSize = 32;
			height = 206;
			startX = xBorderSize; startY = xBorderSize;
		}
		int width = 320+(xBorderSize*2 - startX*2);
		int widthPadding = startX*2;
		canvasSrcPix = makePixmapView(c).subView({startX, startY}, {width, height});
	}
	else
	{
		canvasSrcPix = makePixmapView(c);
	}
}

static void updateCanvasMemPixmap(struct video_canvas_s *c, int x, int y)
{
	IG::PixmapDesc desc{{x, y}, pixFmt};
	c->w = x;
	c->h = y;
	c->pixelFormat = pixFmt;
	delete[] c->pixmapData;
	logMsg("allocating pixmap:%dx%d format:%d bytes:%d", x, y, (int)pixFmt, (int)desc.bytes());
	c->pixmapData = new uint8_t[desc.bytes()];
	resetCanvasSourcePixmap(c);
}

void updateCanvasPixelFormat(struct video_canvas_s *c)
{
	c->bpp = pixelDesc(pixFmt).bitsPerPixel();
	assert(c->bpp == 16 || c->bpp == 32);
	if(!c->pixmapData || c->pixelFormat == pixFmt)
		return;
	auto oldPixmap = releasePixmapView(c);
	updateCanvasMemPixmap(c, c->w, c->h);
	if(oldPixmap.data())
		makePixmapView(c).writeConverted(oldPixmap);
	delete[] oldPixmap.data();
	video_canvas_set_palette(c, c->palette);
}

void video_canvas_resize(struct video_canvas_s *c, char resize_canvas)
{
	int x = c->draw_buffer->canvas_width;
	int y = c->draw_buffer->canvas_height;
	x *= c->videoconfig->scalex;
	y *= c->videoconfig->scaley;
	logMsg("resized canvas to %d,%d, renderer %d", x, y, c->videoconfig->rendermode);
	updateCanvasMemPixmap(c, x, y);
}

video_canvas_t *video_canvas_create(video_canvas_t *c, unsigned int *width, unsigned int *height, int mapped)
{
	logMsg("canvas create:0x%p renderer %d", c, c->videoconfig->rendermode);
	return c;
}

void video_canvas_destroy(struct video_canvas_s *c)
{
	logMsg("canvas destroy:0x%p", c);
	delete[] c->pixmapData;
	releasePixmapView(c);
}
