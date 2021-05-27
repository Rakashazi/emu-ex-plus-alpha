#pragma once

#include "vice.h"
#include <stdint.h>
#include <stdbool.h>

struct video_canvas_s
{
	struct video_render_config_s *videoconfig;
	struct draw_buffer_s *draw_buffer;
	struct viewport_s *viewport;
	struct geometry_s *geometry;
	struct palette_s *palette;
	struct video_draw_buffer_callback_s *video_draw_buffer_callback;
	uint8_t *pixmapData;
	int w;
	int h;
	bool initialized;
	bool created;
	bool skipFrame;
	uint8_t pixelFormat;
	int8_t bpp;
};
typedef struct video_canvas_s video_canvas_t;
