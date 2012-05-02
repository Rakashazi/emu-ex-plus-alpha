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

	TLCS900h_interpret_single.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_SINGLE__
#define __TLCS900H_SINGLE__
//=========================================================================

//===== NOP
void sngNOP(void) __attribute__ ((hot));

//===== NORMAL
void sngNORMAL(void) __attribute__ ((hot));

//===== PUSH SR
void sngPUSHSR(void) __attribute__ ((hot));

//===== POP SR
void sngPOPSR(void) __attribute__ ((hot));

//===== MAX
void sngMAX(void) __attribute__ ((hot));

//===== HALT
void sngHALT(void) __attribute__ ((hot));

//===== EI #3
void sngEI(void) __attribute__ ((hot));

//===== RETI
void sngRETI(void) __attribute__ ((hot));

//===== LD (n), n
void sngLD8_8(void) __attribute__ ((hot));

//===== PUSH n
void sngPUSH8(void) __attribute__ ((hot));

//===== LD (n), nn
void sngLD8_16(void) __attribute__ ((hot));

//===== PUSH nn
void sngPUSH16(void) __attribute__ ((hot));

//===== INCF
void sngINCF(void) __attribute__ ((hot));

//===== DECF
void sngDECF(void) __attribute__ ((hot));

//===== RET condition
void sngRET(void) __attribute__ ((hot));

//===== RETD dd
void sngRETD(void) __attribute__ ((hot));

//===== RCF
void sngRCF(void) __attribute__ ((hot));

//===== SCF
void sngSCF(void) __attribute__ ((hot));

//===== CCF
void sngCCF(void) __attribute__ ((hot));

//===== ZCF
void sngZCF(void) __attribute__ ((hot));

//===== PUSH A
void sngPUSHA(void) __attribute__ ((hot));

//===== POP A
void sngPOPA(void) __attribute__ ((hot));

//===== EX F,F'
void sngEX(void) __attribute__ ((hot));

//===== LDF #3
void sngLDF(void) __attribute__ ((hot));

//===== PUSH F
void sngPUSHF(void) __attribute__ ((hot));

//===== POP F
void sngPOPF(void) __attribute__ ((hot));

//===== JP nn
void sngJP16(void) __attribute__ ((hot));

//===== JP nnn
void sngJP24(void) __attribute__ ((hot));

//===== CALL #16
void sngCALL16(void) __attribute__ ((hot));

//===== CALL #24
void sngCALL24(void) __attribute__ ((hot));

//===== CALR $+3+d16
void sngCALR(void) __attribute__ ((hot));

//===== LD R, n
void sngLDB(void) __attribute__ ((hot));

//===== PUSH RR
void sngPUSHW(void) __attribute__ ((hot));

//===== LD RR, nn
void sngLDW(void) __attribute__ ((hot));

//===== PUSH XRR
void sngPUSHL(void) __attribute__ ((hot));

//===== LD XRR, nnnn
void sngLDL(void) __attribute__ ((hot));

//===== POP RR
void sngPOPW(void) __attribute__ ((hot));

//===== POP XRR
void sngPOPL(void) __attribute__ ((hot));

//===== JR cc,PC + d
void sngJR(void) __attribute__ ((hot));

//===== JR cc,PC + dd
void sngJRL(void) __attribute__ ((hot));

//===== LDX dst,src
void sngLDX(void) __attribute__ ((hot));

//===== SWI num
void sngSWI(void) __attribute__ ((hot));

//=============================================================================
#endif
