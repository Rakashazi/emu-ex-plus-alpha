#pragma once

#include "vice.h"
#include "video.h"
#include <stdint.h>
#include <stdbool.h>

#define ARCHDEP_SHOW_STATUSBAR_FACTORY  0

struct video_canvas_s
{
	struct video_render_config_s *videoconfig;
	struct draw_buffer_s *draw_buffer;
	struct viewport_s *viewport;
	struct geometry_s *geometry;
	struct palette_s *palette;
	void *systemPtr;
	uint8_t *pixmapData;
	int w;
	int h;
	int crt_type;
	bool initialized;
	bool created;
	bool skipFrame;
	uint8_t pixelFormat;
};
typedef struct video_canvas_s video_canvas_t;
