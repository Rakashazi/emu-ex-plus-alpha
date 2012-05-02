/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* compile.h                                                                 */
/*                                                                           */
/*****************************************************************************/
#ifndef _COMPILE_H_
#define _COMPILE_H_
#include "generator.h"
#include "cpu68k.h"

//uint8 *compile_make(t_ipclist *list);
void (*compile_make(t_ipclist *list))(struct _t_ipc *ipc);

#endif
