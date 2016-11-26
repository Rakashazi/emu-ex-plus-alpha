#pragma once

#include "vice.h"
#ifdef __cplusplus
#include <imagine/pixmap/Pixmap.hh>
#endif

struct video_canvas_s
{
	struct video_render_config_s *videoconfig;
	struct draw_buffer_s *draw_buffer;
	struct viewport_s *viewport;
	struct geometry_s *geometry;
	struct palette_s *palette;
	struct video_draw_buffer_callback_s *video_draw_buffer_callback;
	#ifdef __cplusplus
	IG::MemPixmap *pixmap;
	#else
	void *pixmap;
	#endif
	unsigned int initialized;
	unsigned int created;
	char skipFrame;
};
typedef struct video_canvas_s video_canvas_t;
