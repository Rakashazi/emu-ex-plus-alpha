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
	#include "viewport.h"
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

static bool isValidPixelFormat(IG::PixelFormat fmt)
{
	return fmt == IG::PIXEL_FMT_RGB565 || fmt == IG::PIXEL_FMT_RGBA8888 || fmt == IG::PIXEL_FMT_BGRA8888;
}

static IG::Pixmap makePixmapView(const struct video_canvas_s *c)
{
	IG::PixelFormat fmt{(IG::PixelFormatID)c->pixelFormat};
	assumeExpr(isValidPixelFormat(fmt));
	return {{{c->w, c->h}, fmt}, c->pixmapData};
}

static IG::PixelDesc pixelDesc(IG::PixelFormat fmt)
{
	assumeExpr(isValidPixelFormat(fmt));
	return fmt == IG::PIXEL_FMT_RGB565 ? fmt.desc() : fmt.desc().nativeOrder();
}

static void updateInternalPixelFormat(struct video_canvas_s *c, IG::PixelFormat fmt)
{
	assumeExpr(isValidPixelFormat(pixFmt));
	c->pixelFormat = fmt;
	c->bpp = pixelDesc(fmt).bitsPerPixel();
}

void video_arch_canvas_init(struct video_canvas_s *c)
{
	logMsg("created canvas with size %d,%d", c->draw_buffer->canvas_width, c->draw_buffer->canvas_height);
	c->video_draw_buffer_callback = nullptr;
	updateInternalPixelFormat(c, pixFmt);
	activeCanvas = c;
}

int video_canvas_set_palette(video_canvas_t *c, struct palette_s *palette)
{
	IG::PixelFormat fmt{(IG::PixelFormatID)c->pixelFormat};
	const auto pDesc = pixelDesc(fmt);
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
	IG::PixelFormat fmt{(IG::PixelFormatID)c->pixelFormat};
	assumeExpr(isValidPixelFormat(fmt));
	IG::PixmapDesc desc{{x, y}, fmt};
	c->w = x;
	c->h = y;
	delete[] c->pixmapData;
	logMsg("allocating pixmap:%dx%d format:%s bytes:%d", x, y, fmt.name(), (int)desc.bytes());
	c->pixmapData = new uint8_t[desc.bytes()];
	resetCanvasSourcePixmap(c);
}

static void refreshFullCanvas(video_canvas_t *canvas)
{
	auto viewport = canvas->viewport;
	auto geometry = canvas->geometry;
	video_canvas_refresh(canvas,
		viewport->first_x + geometry->extra_offscreen_border_left,
		viewport->first_line,
		viewport->x_offset,
		viewport->y_offset,
		std::min(canvas->draw_buffer->canvas_width, geometry->screen_size.width - viewport->first_x),
		std::min(canvas->draw_buffer->canvas_height, viewport->last_line - viewport->first_line + 1));
}

bool updateCanvasPixelFormat(struct video_canvas_s *c, IG::PixelFormat fmt)
{
	assumeExpr(isValidPixelFormat(fmt));
	if(c->pixelFormat == fmt)
		return false;
	updateInternalPixelFormat(c, fmt);
	if(!c->pixmapData)
		return false;
	updateCanvasMemPixmap(c, c->w, c->h);
	video_canvas_set_palette(c, c->palette);
	refreshFullCanvas(c);
	return true;
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
	c->pixmapData = {};
}
