//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
//=========================================================================

	TLCS900h_interpret.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

21 JUL 2002 - neopop_uk
=======================================
- Added the 'instruction_error' function declaration here.

28 JUL 2002 - neopop_uk
=======================================
- Removed CYCLE_WARNING as it is now obsolete.
- Added generic DIV prototypes.

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_INTERPRET__
#define __TLCS900H_INTERPRET__
//=============================================================================

//Interprets a single instruction from 'pc', 
//pc is incremented to the start of the next instruction.
//Returns the number of cycles taken for this instruction
uint32 TLCS900h_interpret(void) __attribute__ ((hot));

//=============================================================================

extern uint32 mem;
extern int size;
extern uint8 first;			//First byte
extern uint8 second;			//Second byte
extern uint8 R;				//(second & 7)
extern uint8 rCode;
extern uint32 cycles;
extern bool brCode;

//=============================================================================

#ifndef NDEBUG
	void __cdecl instruction_error(const char* vaMessage,...);
#else
	static void __cdecl instruction_error(const char* vaMessage,...) { }
#endif

//=============================================================================

#define FETCH8		loadB(pc++)

uint16 fetch16(void) __attribute__ ((hot));
uint32 fetch24(void) __attribute__ ((hot));
uint32 fetch32(void) __attribute__ ((hot));

//=============================================================================

void parityB(uint8 value) __attribute__ ((hot));
void parityW(uint16 value) __attribute__ ((hot));

//=============================================================================

void push8(uint8 data) __attribute__ ((hot));
void push16(uint16 data) __attribute__ ((hot));
void push32(uint32 data) __attribute__ ((hot));

uint8 pop8(void) __attribute__ ((hot));
uint16 pop16(void) __attribute__ ((hot));
uint32 pop32(void) __attribute__ ((hot));

//=============================================================================

//DIV ===============
uint16 generic_DIV_B(uint16 val, uint8 div) __attribute__ ((hot));
uint32 generic_DIV_W(uint32 val, uint16 div) __attribute__ ((hot));

//DIVS ===============
uint16 generic_DIVS_B(int16 val, int8 div) __attribute__ ((hot));
uint32 generic_DIVS_W(int32 val, int16 div) __attribute__ ((hot));

//ADD ===============
uint8	generic_ADD_B(uint8 dst, uint8 src) __attribute__ ((hot));
uint16 generic_ADD_W(uint16 dst, uint16 src) __attribute__ ((hot));
uint32 generic_ADD_L(uint32 dst, uint32 src) __attribute__ ((hot));

//ADC ===============
uint8	generic_ADC_B(uint8 dst, uint8 src) __attribute__ ((hot));
uint16 generic_ADC_W(uint16 dst, uint16 src) __attribute__ ((hot));
uint32 generic_ADC_L(uint32 dst, uint32 src) __attribute__ ((hot));

//SUB ===============
uint8	generic_SUB_B(uint8 dst, uint8 src) __attribute__ ((hot));
uint16 generic_SUB_W(uint16 dst, uint16 src) __attribute__ ((hot));
uint32 generic_SUB_L(uint32 dst, uint32 src) __attribute__ ((hot));

//SBC ===============
uint8	generic_SBC_B(uint8 dst, uint8 src) __attribute__ ((hot));
uint16 generic_SBC_W(uint16 dst, uint16 src) __attribute__ ((hot));
uint32 generic_SBC_L(uint32 dst, uint32 src) __attribute__ ((hot));

//=============================================================================

//Confirms a condition code check
bool conditionCode(int cc) __attribute__ ((hot));

//=============================================================================

//Translate an rr or RR value for MUL/MULS/DIV/DIVS
uint8 get_rr_Target(void) __attribute__ ((hot));
uint8 get_RR_Target(void) __attribute__ ((hot));

//=============================================================================
#endif
