/*****************************************************************************
**
** This command engine may be used in any MSX emulator. It does not matter 
** under which conditions that the emulator gets distributed. Be it open-
** source or closed-source. Be it commercially or free-of-charge.
** 
** This command engine may not be used in any other kind of softwares.
** Using this command engine is at own risk. The author is not responsible 
** nor liable for any damages that may occur intentionally or unintentionally 
** from it's usage.
**
******************************************************************************
*/
#ifndef VDPCMD_H
#define VDPCMD_H

#include "MsxTypes.h"


typedef struct VdpCmdState VdpCmdState;

/*************************************************************
** vdpCmdCreate
**
** Description:
**      Creates a V99x8 command engine. A timer running at
**      21477270 Hz drives the command engine. systemTime
**      is the initial time of this timer.
**************************************************************
*/
VdpCmdState* vdpCmdCreate(int vramSize, UInt8* vramPtr, UInt32 systemTime);

void vdpCmdDestroy(VdpCmdState* state);

/*************************************************************
** vdpCmdWrite
**
** Description:
**      Writes a new command to the VDP
**************************************************************
*/
void  vdpCmdWrite(VdpCmdState* state, UInt8 reg, UInt8 value, UInt32 systemTime);

/*************************************************************
** vdpCmdPeek
**
** Description:
**      Returns the current value of a VDP command register
**************************************************************
*/
UInt8 vdpCmdPeek(VdpCmdState* vdpCmd, UInt8 reg, UInt32 systemTime);

void vdpSetScreenMode(VdpCmdState* state, int screenMode, int commandEnable);
void vdpSetTimingMode(VdpCmdState* state, UInt8 timingMode);


UInt8 vdpGetStatus(VdpCmdState* state);
UInt16 vdpGetBorderX(VdpCmdState* state);
UInt8 vdpGetColor(VdpCmdState* state);

/*************************************************************
** vdpCmdExecute
**
** Description:
**      Executes a number of cycles of the VDP Command engine
**      (in 3579545 Hz)
**************************************************************
*/
void  vdpCmdExecute(VdpCmdState* state, UInt32 systemTime);


/*************************************************************
** vdpCmdFlush
**
** Description:
**      Flushes current VDP command
**************************************************************
*/
void  vdpCmdFlush(VdpCmdState* state);


/*************************************************************
** vdpCmdLoadState
**
** Description:
**      Loads the state of the command engine. 
**************************************************************
*/
void vdpCmdLoadState(VdpCmdState* state);


/*************************************************************
** vdpCmdSaveState
**
** Description:
**      Saves the state of the command engine.
**************************************************************
*/
void vdpCmdSaveState(VdpCmdState* state);

#endif /* VDPCMD_H */
