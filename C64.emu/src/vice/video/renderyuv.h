/*
 * renderyuv.h - YUV rendering.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_RENDERYUV_H
#define VICE_RENDERYUV_H

#include "types.h"
#include "video-render.h"

/* Image struct, modeled after XvImage. */
typedef struct {
    int width, height;
    int data_size;              /* bytes */
    int num_planes;
    int *pitches;               /* bytes */
    int *offsets;               /* bytes */
    BYTE *data;
} image_t;

/* A FOURCC consists of four bytes that can be interpreted either as
   a four-character string or as a four-byte integer. */
typedef union {
    int id;
    char label[4];
} fourcc_t;

/* YUV formats in preferred order. Since the order of the four bytes
   is fixed, the integer representation of the FOURCC's is different
   on little and big endian platforms. */
#ifdef WORDS_BIGENDIAN

/* YUV 4:2:2 formats: */
#define FOURCC_UYVY 0x55595659
#define FOURCC_YUY2 0x59555932
#define FOURCC_YVYU 0x59565955
/* YUV 4:1:1 formats: */
#define FOURCC_YV12 0x59563132
#define FOURCC_I420 0x49343230
#define FOURCC_IYUV 0x49595556

#else

/* YUV 4:2:2 formats: */
#define FOURCC_UYVY 0x59565955
#define FOURCC_YUY2 0x32595559
#define FOURCC_YVYU 0x55595659
/* YUV 4:1:1 formats: */
#define FOURCC_YV12 0x32315659
#define FOURCC_I420 0x30323449
#define FOURCC_IYUV 0x56555949

#endif


extern void render_yuv_image(viewport_t *viewport,
                             int true_pal_mode,
                             int pal_blur,
                             int pal_scanline_shade,
                             fourcc_t format,
                             image_t* image,
                             unsigned char* src,
                             int src_pitch,
                             struct video_render_config_s *config,
                             int src_x, int src_y,
                             unsigned int src_w, unsigned int src_h,
                             int dest_x, int dest_y);

extern void renderyuv_4_2_2(image_t* image,
                            int shift_y0, int shift_u,
                            int shift_v, int shift_y1,
                            unsigned char* src,
                            int src_pitch,
                            unsigned int* src_color,
                            int src_x, int src_y,
                            unsigned int src_w, unsigned int src_h,
                            int dest_x, int dest_y, int *yuv_updated);

extern void renderyuv_2x_4_2_2(image_t* image,
                               int shift_y0, int shift_u,
                               int shift_v, int shift_y1,
                               unsigned char* src,
                               int src_pitch,
                               unsigned int* src_color,
                               int src_x, int src_y,
                               unsigned int src_w, unsigned int src_h,
                               int dest_x, int dest_y,
                               int double_scan, int pal_scanline_shade, int *yuv_updated);

extern void renderyuv_4_1_1(image_t* image,
                            int plane_y, int plane_u, int plane_v,
                            unsigned char* src,
                            int src_pitch,
                            unsigned int* src_color,
                            int src_x, int src_y,
                            unsigned int src_w, unsigned int src_h,
                            int dest_x, int dest_y, int *yuv_updated);

extern void renderyuv_2x_4_1_1(image_t* image,
                               int plane_y, int plane_u, int plane_v,
                               unsigned char* src,
                               int src_pitch,
                               unsigned int* src_color,
                               int src_x, int src_y,
                               unsigned int src_w, unsigned int src_h,
                               int dest_x, int dest_y,
                               int double_scan, int pal_scanline_shade, int *yuv_updated);

extern void renderyuv_4_1_1_pal(image_t* image,
                                int plane_y, int plane_u, int plane_v,
                                unsigned char* src,
                                int src_pitch,
                                unsigned int* src_color,
                                int src_x, int src_y,
                                unsigned int src_w, unsigned int src_h,
                                int dest_x, int dest_y,
                                int pal_blur, int *yuv_updated);

extern void renderyuv_2x_4_1_1_pal(image_t* image,
                                   int plane_y, int plane_u, int plane_v,
                                   unsigned char* src,
                                   int src_pitch,
                                   unsigned int* src_color,
                                   int src_x, int src_y,
                                   unsigned int src_w, unsigned int src_h,
                                   int dest_x, int dest_y,
                                   int pal_blur,
                                   int pal_scanline_shade,
                                   int *yuv_updated);

#endif
