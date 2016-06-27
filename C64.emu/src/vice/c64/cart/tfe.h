/*
 * tfe.h - TFE ("The final ethernet") emulation.
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
  #error TFE.H should not be included if HAVE_TFE is not defined!
#endif /* #ifdef HAVE_TFE */

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_TFE_H
#define VICE_TFE_H

struct snapshot_s;
extern int tfe_snapshot_read_module(struct snapshot_s *s);
extern int tfe_snapshot_write_module(struct snapshot_s *s);

extern int tfe_cart_enabled(void);
extern int tfe_as_rr_net;

extern void tfe_init(void);
extern int tfe_resources_init(void);
extern void tfe_resources_shutdown(void);
extern int tfe_cmdline_options_init(void);

extern void tfe_reset(void);
extern void tfe_detach(void);
extern int tfe_enable(void);

extern void tfe_clockport_changed(void);

#endif
