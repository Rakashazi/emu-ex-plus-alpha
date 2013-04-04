/*
 * 6510core.h - Core definitions for the MOS 6510 CPU emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_6510CORE_H
#define VICE_6510CORE_H

/* Masks to extract information. */
#define OPINFO_DELAYS_INTERRUPT_MSK     (1 << 8)
#define OPINFO_DISABLES_IRQ_MSK         (1 << 9)
#define OPINFO_ENABLES_IRQ_MSK          (1 << 10)

/* Return nonzero if `opinfo' causes a 1-cycle interrupt delay.  */
#define OPINFO_DELAYS_INTERRUPT(opinfo)         \
    ((opinfo) & OPINFO_DELAYS_INTERRUPT_MSK)

/* Return nonzero if `opinfo' has changed the I flag from 0 to 1, so that an
   IRQ that happened 2 or more cycles before the end of the opcode should be
   allowed.  */
#define OPINFO_DISABLES_IRQ(opinfo)             \
    ((opinfo) & OPINFO_DISABLES_IRQ_MSK)

/* Return nonzero if `opinfo' has changed the I flag from 1 to 0, so that an
   IRQ that happened 2 or more cycles before the end of the opcode should not
   be allowed.  */
#define OPINFO_ENABLES_IRQ(opinfo)              \
    ((opinfo) & OPINFO_ENABLES_IRQ_MSK)

/* Set the information for `opinfo'.  `number' is the opcode number,
   `delays_interrupt' must be non-zero if it causes a 1-cycle interrupt
   delay, `disables_interrupts' must be non-zero if it disabled IRQs.  */
#define OPINFO_SET(opinfo,                                                \
                   number, delays_interrupt, disables_irq, enables_irq)   \
    ((opinfo) = ((number)                                                 \
                 | ((delays_interrupt) ? OPINFO_DELAYS_INTERRUPT_MSK : 0) \
                 | ((disables_irq) ? OPINFO_DISABLES_IRQ_MSK : 0)         \
                 | ((enables_irq) ? OPINFO_ENABLES_IRQ_MSK : 0)))

/* Set whether the opcode causes the 1-cycle interrupt delay according to
   `delay'.  */
#define OPINFO_SET_DELAYS_INTERRUPT(opinfo, delay)   \
    do {                                             \
        if ((delay)) {                               \
            (opinfo) |= OPINFO_DELAYS_INTERRUPT_MSK; \
        }                                            \
    } while (0)

/* Set whether the opcode disables previously enabled IRQs according to
   `disable'.  */
#define OPINFO_SET_DISABLES_IRQ(opinfo, disable) \
    do {                                         \
        if ((disable)) {                         \
            (opinfo) |= OPINFO_DISABLES_IRQ_MSK; \
        }                                        \
    } while (0)

/* Set whether the opcode enables previously disabled IRQs according to
   `enable'.  */
#define OPINFO_SET_ENABLES_IRQ(opinfo, enable)  \
    do {                                        \
        if ((enable)) {                         \
            (opinfo) |= OPINFO_ENABLES_IRQ_MSK; \
        }                                       \
    } while (0)

#endif
