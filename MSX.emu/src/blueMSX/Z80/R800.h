/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Z80/R800.h,v $
**
** $Revision: 1.20 $
**
** $Date: 2008-06-29 07:53:25 $
**
** Author: Daniel Vik
**
** Description: Emulation of the Z80/R800 processor
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
**
** This code emulates the Z80/R800 processor. The emulation starts in
** Z80 mode which makes it useful for any Z80 based emulator.
** 
** The code has been verified with the 'zexall' test suite.
**
** An internal clock that runs at 21.477270 MHz is being used. However, 
** the frequency is scaled down to:
**     Z80    3.579545 MHz
**     R800   7.159090 MHz
**
** It is possible to chance the clock speed to any frequency in the 
** series 21477270 / n where n is an integer greater or equal to 1.
**
** The emulation is driven by subsequent calls to 'r800execute' which
** runs the emulation until the time given as an argument has passed.
** The time used as argument should come from a timer running at
** 2.1477270 MHz (= 6 * normal MSX frequency).
**
**
** Development notes:
**     This Z80/R800 emulation was developed by Daniel Vik the summer and fall
**     of 2004. The development were done by implementing instruction by 
**     instruction and using an existing Z80 to emulate 'not yet implemented' 
**     instructions. Nothing from this emulation remains in the final source.
**
**     Even though the Undocumented Z80 Documented document gives information
**     about the X and Y flags, some blanks had to be filled. As extra source
**     of information regarding these flags, the Z80 emulation in openMSX was
**     used as reference.
**
**     Information about the mul instructions were collected from the R800
**     extension to Marcel de Kogel's Z80 emulation made by Lex Lechz.
**
**     The Z80Em emulation by Marcel de Kogel and the Z80 emulation in openMSX
**     by Wouter Vermaelen was used during the development as proof of 
**     concept.
**
**     Furthermore, the Z80 test suite 'zexall' by Frank Cringle and
**     J.G. Harston was used for verification of the emulation.
**
**
** References:
**     - Z80 Family CPU Users Manual, UM008001-1000, ZiLOG 2001
**     - The Undocumented Z80 Documented version 0.6, Sean Young 2003
**     - R800 vs Z80 timing sheet, Tobias Keizer 2004
**
******************************************************************************
*/
#ifndef R800_H
#define R800_H

#include "MsxTypes.h"

/*****************************************************
** Configuration options
*/
#include <Config/Z80.h>

/*****************************************************
** CpuMode
**
** CPU mode definitions
******************************************************
*/
typedef enum { 
    CPU_Z80 = 0, 
    CPU_R800 = 1, 
    CPU_UNKNOWN 
} CpuMode;


/*****************************************************
** CpuFlags
**
** CPU configuration
******************************************************
*/
typedef enum { 
    CPU_VDP_IO_DELAY = 0x0001,
    CPU_ENABLE_M1    = 0x0002,
} CpuFlags;


/*****************************************************
** R800_MASTER_FREQUENCY
**
** Frequency of the system clock.
******************************************************
*/
#define R800_MASTER_FREQUENCY  21477270


/*****************************************************
** ASDBG_*
**
** asmsx debug commands
******************************************************
*/
#define ASDBG_TRACE 1
#define ASDBG_SETBP 2


/*****************************************************
** SystemTime
**
** Type used for the system time
******************************************************
*/
typedef UInt32 SystemTime;


/*****************************************************
** RegisterPair
**
** Defines a register pair. Define __BIG_ENDIAN__ on
** big endian host machines.
******************************************************
*/
typedef union {
  struct { 
#ifdef __BIG_ENDIAN__
      UInt8 h;
      UInt8 l; 
#else
      UInt8 l; 
      UInt8 h; 
#endif
  } B;
  UInt16 W;
} RegisterPair;


/*****************************************************
** CpuRegs
**
** CPU registers.
******************************************************
*/
typedef struct {
    RegisterPair AF;
    RegisterPair BC;
    RegisterPair DE;
    RegisterPair HL;
    RegisterPair IX;
    RegisterPair IY;
    RegisterPair PC;
    RegisterPair SP;
    RegisterPair AF1;
    RegisterPair BC1;
    RegisterPair DE1;
    RegisterPair HL1;
    RegisterPair SH;
    UInt8 I;
    UInt8 R;
    UInt8 R2;

    UInt8 iff1;
    UInt8 iff2;
    UInt8 im;
    UInt8 halt;	UInt8 ei_mode;
} CpuRegs;


/*****************************************************
** Callback types
**
** Read, write and patch callback types.
******************************************************
*/
typedef UInt8 (*R800ReadCb)(void*, UInt16);
typedef void  (*R800WriteCb)(void*, UInt16, UInt8);
typedef void  (*R800PatchCb)(void*, CpuRegs*);
typedef void  (*R800BreakptCb)(void*, UInt16);
typedef void  (*R800DebugCb)(void*, int, const char*);
typedef void  (*R800TrapCb)(void*, UInt8);
typedef void  (*R800TimerCb)(void*);


/*****************************************************
** Status flags.
**
** May be used in the disk patch and cassette patch
******************************************************
*/
#define C_FLAG      0x01
#define N_FLAG      0x02
#define P_FLAG      0x04
#define V_FLAG      0x04
#define X_FLAG      0x08
#define H_FLAG      0x10
#define Y_FLAG      0x20
#define Z_FLAG      0x40
#define S_FLAG      0x80

/*****************************************************
** R800
**
** Structure that defines the R800 core.
******************************************************
*/
typedef struct
{
    SystemTime    systemTime;       /* Current system time             */
    UInt32        vdpTime;          /* Time of last access to MSX vdp  */
    UInt16        cachePage;        /* Current page in cache           */
    CpuRegs       regs;             /* Active register bank            */
    UInt32        delay[32];        /* Instruction timing table        */
    UInt8         dataBus;          /* Current value on the data bus   */
    UInt8         defaultDatabus;   /* Value that is set after im2     */
    int           intState;         /* Sate of interrupt line          */
    int           nmiState;         /* Current NMI state               */
    int           nmiEdge;          /* Current NMI state               */

    CpuMode       cpuMode;          /* Current CPU mode                */
    CpuMode       oldCpuMode;       /* CPU mode before CPU switch      */
    CpuRegs       regBanks[2];      /* Z80 and R800 register banks     */
    UInt32        cpuFlags;         /* Current CPU flags               */

    UInt32        instCnt;          /* Instruction counter             */

    UInt32        frequencyZ80;     /* Frequency of Z80 (in Hz)        */
    UInt32        frequencyR800;    /* Frequency of R800 (in Hz)       */

    int           terminate;        /* Termination flag                */
    SystemTime    timeout;          /* User scheduled timeout          */

    //R800ReadCb    readMemory;       /* Callback functions for reading  */
    //R800WriteCb   writeMemory;      /* and writing memory and IO ports */
    R800ReadCb    readIoPort;
    R800WriteCb   writeIoPort;
    R800PatchCb   patch;
    R800TimerCb   timerCb;
    R800BreakptCb breakpointCb;
    R800DebugCb   debugCb;
    R800TrapCb    trapCb;
    void*         ref;              /* User defined pointer which is   */
                                    /* passed to the callbacks         */

#ifdef ENABLE_CALLSTACK
    UInt32        callstackSize;    /* Nr of entries in the callstack  */
                                    /* only last 256 entries are stored*/
    UInt16        callstack[256];   /* Callstack buffer                */
#endif

#ifdef ENABLE_BREAKPOINTS
    int           breakpointCount;  /* Number of breakpoints set       */
    char          breakpoints[0x10000];
#endif
} R800;


/************************************************************************
** r800Create
**
** The method initializes and returns a pointer to a newly created R800 
** object. The CPU is started in Z80 mode.
**
** Arguments:
**      readMemory   - Function called on read access to RAM
**      writeMemory  - Function called on write access to RAM
**      readIoPort   - Function called on read access to I/O ports
**      writeIoPort  - Function called on write access to I/O ports
**      patch        - Function called when the patch instruction ED FE
**                     is executed. 
**      timerCb      - Function called on user scheduled timeouts
**      breakpointCb - Function called when a breakpoint is hit
**      debugCb      - Function called when a debug trap is hit
**      trapCb       - Function called when a trap is hit
**      ref          - User defined reference that will be passed to the
**                     callbacks.
**
** Return value:
**      A pointer to a new R800 object.
*************************************************************************
*/
R800* r800Create(UInt32 cpuFlags, 
                 R800ReadCb readMemory, R800WriteCb writeMemory,
                 R800ReadCb readIoPort, R800WriteCb writeIoPort, 
                 R800PatchCb patch,     R800TimerCb timerCb,
                 R800BreakptCb bpCb,    R800DebugCb debugCb,
                 R800TrapCb trapCb,
                 void* ref);

/************************************************************************
** r800Destroy
**
** Destroys an R800 object and frees allocated resources.
**
** Arguments:
**      r800        - Pointer to the R800 object to be destroyed
*************************************************************************
*/
void r800Destroy(R800* r800);

/************************************************************************
** r800SetFrequency
**
** Sets the frequency of the CPU mode specified by the cpuMode argument.
** The selected frequency must be an integer fraction of the master
** frequency, e.g. R800_MASTER_FREQUENCY / 6. If a non integer fraction
** is selected the timing tables will become invalid.
**
** Arguments:
**      r800        - Pointer to an R800 object
**      cpuMode     - CPU mode that gets a new frequency
**      frequency   - New frequency of the CPU
*************************************************************************
*/
void r800SetFrequency(R800* r800, CpuMode cpuMode, UInt32 frequency);

/************************************************************************
** r800Reset
**
** Resets the R800.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800Reset(R800* r800, UInt32 cpuTime);

/************************************************************************
** r800SetMode
**
** Sets the CPU mode.
**
** Arguments:
**      r800        - Pointer to an R800 object
**      mode        - New CPU mode
*************************************************************************
*/
void r800SetMode(R800* r800, CpuMode mode);

/************************************************************************
** r800GetMode
**
** Gets the current CPU mode.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
CpuMode r800GetMode(R800* r800);

/************************************************************************
** r800SetInt
**
** Raises the interrupt line.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800SetInt(R800* r800);

/************************************************************************
** r800ClearInt
**
** Clears the interrupt line.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800ClearInt(R800* r800);

/************************************************************************
** r800SetNmi
**
** Raises the non maskable interrupt line.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800SetNmi(R800* r800);

/************************************************************************
** r800ClearNmi
**
** Clears the non maskable interrupt line.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800ClearNmi(R800* r800);

/************************************************************************
** r800SetDataBus
**
** Sets the data on the data bus. The default value is 0xff.
**
** Arguments:
**      r800        - Pointer to an R800 object
**      value       - New value on the data bus
**      defValue    - Value that the data bus restores to after int
**      useDef      - Tells whether to modify the def value
*************************************************************************
*/
void r800SetDataBus(R800* r800, UInt8 value, UInt8 defValue, int useDef);

/************************************************************************
** r800Execute
**
** Executes CPU instructions until the r800StopExecution function is
** called.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800Execute(R800* r800);

/************************************************************************
** r800ExecuteUntil
**
** Executes CPU instructions until endTime has passed.
**
** Arguments:
**      r800        - Pointer to an R800 object
**      endTime     - Time when to end execution and return from the
**                    function
*************************************************************************
*/
void r800ExecuteUntil(R800* r800, UInt32 endTime);

/************************************************************************
** r800ExecuteInstruction
**
** Executes one CPU instruction.
**
** Arguments:
**      r800        - Pointer to an R800 object
*************************************************************************
*/
void r800ExecuteInstruction(R800* r800);

void r800StopExecution(R800* r800);
void r800SetTimeoutAt(R800* r800, SystemTime time);

void r800SetBreakpoint(R800* r800, UInt16 address);
void r800ClearBreakpoint(R800* r800, UInt16 address);

/************************************************************************
** r800GetSystemTime
**
** Returns the current system time.
**
** Arguments:
**      r800        - Pointer to an R800 object
**
** Return value:
**      Current system time.
*************************************************************************
*/
SystemTime r800GetSystemTime(R800* r800);

#endif /* Z80_H */
