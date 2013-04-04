/*
 * rawnet.h - raw ethernet interface
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifdef HAVE_TFE
#else
  #error RAWNET.H should not be included if HAVE_TFE is not defined!
#endif /* #ifdef HAVE_TFE */

#ifndef VICE_RAWNET_H
#define VICE_RAWNET_H

/*
 This is a helper for the _receive() function of the emulated ethernet chip to determine
 if the received frame should be accepted according to the settings.

 This function is even allowed to be called in rawnetarch.c from rawnet_arch_receive() if
 necessary. the respective helper function of the emulated ethernet chip must be registered
 using rawnet_set_should_accept_func at init time.
*/

extern int rawnet_should_accept(unsigned char *buffer, int length, int *phashed, int *phash_index, int *pcorrect_mac, int *pbroadcast, int *pmulticast);
extern void rawnet_set_should_accept_func(int (*func)(unsigned char *, int, int *, int *, int *, int *, int *));

/*

 These functions let the UI enumerate the available interfaces.

 First, rawnet_enumadapter_open() is used to start enumeration.

 rawnet_enumadapter() is then used to gather information for each adapter present
 on the system, where:

   ppname points to a pointer which will hold the name of the interface
   ppdescription points to a pointer which will hold the description of the interface

   For each of these parameters, new memory is allocated, so it has to be
   freed with lib_free().

 rawnet_enumadapter_close() must be used to stop processing.

 Each function returns 1 on success, and 0 on failure.
 rawnet_enumadapter() only fails if there is no more adpater; in this case,
   *ppname and *ppdescription are not altered.
*/
extern int rawnet_enumadapter_open(void);
extern int rawnet_enumadapter(char **ppname, char **ppdescription);
extern int rawnet_enumadapter_close(void);

#endif
