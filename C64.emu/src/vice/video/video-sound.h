/*
 * video-sound.h - Video to Audio leak emulation
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_VIDEO_SOUND_H
#define VICE_VIDEO_SOUND_H

#include "video.h"
#include "viewport.h"

void video_sound_update(video_render_config_t *config, const BYTE *src,
                        unsigned int width, unsigned int height,
                        unsigned int xs, unsigned int ys,
                        unsigned int pitchs, viewport_t *viewport);
void video_sound_init(void);

#endif
