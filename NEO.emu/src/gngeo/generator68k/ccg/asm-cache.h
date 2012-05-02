/* asm-cache.h -- instruction/data cache synchronisation
 *
 * Copyright (C) 1999, 2000 Ian Piumarta <ian.piumarta@inria.fr>
 *
 * This file is part of CCG.
 *
 * CCG is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CCG is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the file COPYING for more details.
 *
 * Last edited: Thu Jan 13 12:00:45 2000 by piumarta (Ian Piumarta) on pingu
 */

#ifndef __ccg_asm_cache_h
#define __ccg_asm_cache_h



#if defined(__i386__)					  /**** i386  ****/

# define iflush(first,last)

#elif defined(__PPC__) || defined(_POWER) || defined(_IBMR2)  /**** PowerPC ****/

# if defined(__GNUC__)
    static void iflush(register insn *addr, register insn *last)
    {
      while (addr < last) {
	asm volatile ("dcbst 0,%0; sync; icbi 0,%0; isync" : : "r"(addr));
	++addr;	/* conservative */
      }
    }
# elif defined(macintosh) || defined(__MWERKS__)
    /* PowerMac: assumes CodeWarrior 8 or later (other compilers might barf) */
    /* (tho' this probably works with MrC and Motorola compilers too...)     */
#   pragma internal on
    static asm void iflush(register insn *addr, register insn *last)
    {
 	  cmplw   cr1, addr, last
 	  bgelr   cr1             // return if addr >= last
    loop: dcbst   r0, addr        // flush d-cache
 	  sync
 	  icbi    r0, addr        // sync i-cache
 	  isync
 	  addi    addr, addr, 4   // next cache line (4 is very conservative)
 	  cmplw   cr1, addr, last // finished?
 	  blt     cr1, loop       // not yet...
 	  blr                     // go bye bye
    }
#   pragma internal off
# else
#   define _ASM_NOCOMP
# endif

#elif defined(__sparc__)				  /**** Sparc ****/

# if defined(__GNUC__)
    static void iflush(register insn *addr, register insn *last)
    {
      asm volatile ("stbar");			/* PSO memory model */
      while (addr < last) {
	asm volatile ("flush %0"::"r"(addr));	/* all memory models */
	++addr;	/* conservative */
      }
      /* [SPARC Architecture Manual v8, page 139, implementation note #5] */
      asm volatile ("nop; nop; nop; nop; nop");
    }
# else
#   define _ASM_NOCOMP
# endif

#elif 1

# define _ASM_NOARCH

#endif



#ifdef _ASM_NOCOMP
# error: please reimplement iflush() for your compiler,
# error: and EMAIL THE CODE to: ian.piumarta@inria.fr
#endif

#ifdef _ASM_NOARCH
# error: please implement iflush() for your architecture,
# error: and EMAIL THE CODE to: ian.piumarta@inria.fr
#endif



#endif /* __ccg_asm_cache_h */
