#pragma once

#include "vice.h"

struct video_canvas_s
{
	unsigned int initialized;
	unsigned int created;
	struct video_render_config_s *videoconfig;
	struct draw_buffer_s *draw_buffer;
	struct viewport_s *viewport;
	struct geometry_s *geometry;
	struct palette_s *palette;
	struct video_draw_buffer_callback_s *video_draw_buffer_callback;
};
typedef struct video_canvas_s video_canvas_t;
