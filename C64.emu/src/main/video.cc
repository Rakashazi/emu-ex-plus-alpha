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
#include <emuframework/CommonFrameworkIncludes.hh>
#ifdef __APPLE__
#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/mach.h>
#else
#include <semaphore.h>
#endif

extern "C"
{
	#include "palette.h"
	#include "video.h"
	#include "videoarch.h"
	#include "kbdbuf.h"
	#include "sound.h"
}

CLINK void (*vsync_hook)(void);
extern bool runningFrame;
extern uint c64VidX, c64VidY;
alignas(8) extern uint16 pix[520*312];
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
#ifdef __APPLE__
extern semaphore_t execSem, execDoneSem;
#else
extern sem_t execSem, execDoneSem;
#endif

// Number of timer units per second, unused
CLINK signed long vsyncarch_frequency(void)
{
  // Microseconds resolution
  return 1000000;
}

CLINK unsigned long vsyncarch_gettime(void)
{
//  struct timeval now;
//  gettimeofday(&now, NULL);
//  return 1000000UL * now.tv_sec + now.tv_usec;
	bug_exit("shouldn't be called");
	return 0;
}

CLINK void vsyncarch_init(void) {}

CLINK void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled) { }

CLINK void vsyncarch_sleep(signed long delay)
{
	logMsg("called vsyncarch_sleep() with %ld", delay);
	bug_exit("shouldn't be called");
}

CLINK void vsyncarch_presync(void) {}

CLINK void vsyncarch_postsync(void) {}

CLINK int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
	sound_flush();
	kbdbuf_flush();
	vsync_hook();
	assert(EmuSystem::gameIsRunning());
	if(likely(runningFrame))
	{
		//logMsg("vsync_do_vsync signaling main thread");
		#ifdef __APPLE__
		semaphore_signal(execDoneSem);
		semaphore_wait(execSem);
		#else
		sem_post(&execDoneSem);
		sem_wait(&execSem);
		#endif
	}
	else
	{
		logMsg("spurious vsync_do_vsync()");
	}
	return 0;
}

CLINK void video_arch_canvas_init(struct video_canvas_s *canvas)
{
	logMsg("created canvas with size %d,%d", canvas->draw_buffer->canvas_width, canvas->draw_buffer->canvas_height);
	canvas->video_draw_buffer_callback = nullptr;
}

CLINK int video_canvas_set_palette(video_canvas_t *c, struct palette_s *palette)
{
	if(!palette)
	{
		return 0; // no palette, nothing to do
	}

	c->palette = palette;

	iterateTimes(palette->num_entries, i)
	{
		auto col = pixFmt.desc().build(palette->entries[i].red/255., palette->entries[i].green/255., palette->entries[i].blue/255., 0.);
		logMsg("set color %d to %X", i, col);
		video_render_setphysicalcolor(c->videoconfig, i, col, pixFmt.bitsPerPixel());
	}

	iterateTimes(256, i)
	{
		video_render_setrawrgb(i, pixFmt.desc().build(i/255., 0., 0., 0.), pixFmt.desc().build(0., i/255., 0., 0.), pixFmt.desc().build(0., 0., i/255., 0.));
	}
	video_render_initraw(c->videoconfig);

	return 0;
}

CLINK void video_canvas_refresh(struct video_canvas_s *canvas, unsigned int xs, unsigned int ys, unsigned int xi, unsigned int yi, unsigned int w, unsigned int h)
{
	video_canvas_render(canvas, (BYTE*)pix, w, h, xs, ys, xi, yi, emuVideo.vidPix.pitchBytes(), pixFmt.bitsPerPixel());
}

CLINK void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
	c64VidX = canvas->draw_buffer->canvas_width;
	c64VidY = canvas->draw_buffer->canvas_height;
	logMsg("resized canvas to %d,%d", c64VidX, c64VidY);
	if(unlikely(!emuVideo.vidImg))
	{
		emuVideo.initImage(0, c64VidX, c64VidY);
	}
}

CLINK video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
	logMsg("renderer %d", canvas->videoconfig->rendermode);
	canvas->videoconfig->filter = VIDEO_FILTER_NONE;
	return canvas;
}
