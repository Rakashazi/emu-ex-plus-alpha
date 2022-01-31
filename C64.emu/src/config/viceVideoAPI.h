#ifndef VICE_VIDEO_H
#error must be included as part of video.h
#endif

VICE_API void video_render_setphysicalcolor(video_render_config_t *config,
                                          int index, uint32_t color, int depth);
VICE_API void video_render_setrawrgb(video_render_color_tables_t *color_tab, unsigned int index,
                                     uint32_t r, uint32_t g, uint32_t b);
VICE_API void video_render_initraw(struct video_render_config_s *videoconfig);
VICE_API struct video_canvas_s *video_canvas_create(struct video_canvas_s *canvas,
                                                  unsigned int *width, unsigned int *height,
                                                  int mapped);
VICE_API void video_arch_canvas_init(struct video_canvas_s *canvas);
VICE_API void video_canvas_refresh(struct video_canvas_s *canvas,
                                 unsigned int xs, unsigned int ys,
                                 unsigned int xi, unsigned int yi,
                                 unsigned int w, unsigned int h);
VICE_API int video_canvas_set_palette(struct video_canvas_s *canvas,
                                    struct palette_s *palette);
VICE_API void video_canvas_destroy(struct video_canvas_s *canvas);
VICE_API void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas);
VICE_API void video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg,
                                  int width, int height, int xs, int ys,
                                  int xt, int yt, int pitcht);
