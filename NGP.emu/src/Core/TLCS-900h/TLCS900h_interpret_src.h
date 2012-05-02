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

	TLCS900h_interpret_src.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_SRC__
#define __TLCS900H_SRC__
//=========================================================================

//===== PUSH (mem)
void srcPUSH(void) __attribute__ ((hot));

//===== RLD A,(mem)
void srcRLD(void) __attribute__ ((hot));

//===== RRD A,(mem)
void srcRRD(void) __attribute__ ((hot));

//===== LDI
void srcLDI(void) __attribute__ ((hot));

//===== LDIR
void srcLDIR(void) __attribute__ ((hot));

//===== LDD
void srcLDD(void) __attribute__ ((hot));

//===== LDDR
void srcLDDR(void) __attribute__ ((hot));

//===== CPI
void srcCPI(void) __attribute__ ((hot));

//===== CPIR
void srcCPIR(void) __attribute__ ((hot));

//===== CPD
void srcCPD(void) __attribute__ ((hot));

//===== CPDR
void srcCPDR(void) __attribute__ ((hot));

//===== LD (nn),(mem)
void srcLD16m(void) __attribute__ ((hot));

//===== LD R,(mem)
void srcLD(void) __attribute__ ((hot));

//===== EX (mem),R
void srcEX(void) __attribute__ ((hot));

//===== ADD (mem),#
void srcADDi(void) __attribute__ ((hot));

//===== ADC (mem),#
void srcADCi(void) __attribute__ ((hot));

//===== SUB (mem),#
void srcSUBi(void) __attribute__ ((hot));

//===== SBC (mem),#
void srcSBCi(void) __attribute__ ((hot));

//===== AND (mem),#
void srcANDi(void) __attribute__ ((hot));

//===== OR (mem),#
void srcORi(void) __attribute__ ((hot));

//===== XOR (mem),#
void srcXORi(void) __attribute__ ((hot));

//===== CP (mem),#
void srcCPi(void) __attribute__ ((hot));

//===== MUL RR,(mem)
void srcMUL(void) __attribute__ ((hot));

//===== MULS RR,(mem)
void srcMULS(void) __attribute__ ((hot));

//===== DIV RR,(mem)
void srcDIV(void) __attribute__ ((hot));

//===== DIVS RR,(mem)
void srcDIVS(void) __attribute__ ((hot));

//===== INC #3,(mem)
void srcINC(void) __attribute__ ((hot));

//===== DEC #3,(mem)
void srcDEC(void) __attribute__ ((hot));

//===== RLC (mem)
void srcRLC(void) __attribute__ ((hot));

//===== RRC (mem)
void srcRRC(void) __attribute__ ((hot));

//===== RL (mem)
void srcRL(void) __attribute__ ((hot));

//===== RR (mem)
void srcRR(void) __attribute__ ((hot));

//===== SLA (mem)
void srcSLA(void) __attribute__ ((hot));

//===== SRA (mem)
void srcSRA(void) __attribute__ ((hot));

//===== SLL (mem)
void srcSLL(void) __attribute__ ((hot));

//===== SRL (mem)
void srcSRL(void) __attribute__ ((hot));

//===== ADD R,(mem)
void srcADDRm(void) __attribute__ ((hot));

//===== ADD (mem),R
void srcADDmR(void) __attribute__ ((hot));

//===== ADC R,(mem)
void srcADCRm(void) __attribute__ ((hot));

//===== ADC (mem),R
void srcADCmR(void) __attribute__ ((hot));

//===== SUB R,(mem)
void srcSUBRm(void) __attribute__ ((hot));

//===== SUB (mem),R
void srcSUBmR(void) __attribute__ ((hot));

//===== SBC R,(mem)
void srcSBCRm(void) __attribute__ ((hot));

//===== SBC (mem),R
void srcSBCmR(void) __attribute__ ((hot));

//===== AND R,(mem)
void srcANDRm(void) __attribute__ ((hot));

//===== AND (mem),R
void srcANDmR(void) __attribute__ ((hot));

//===== XOR R,(mem)
void srcXORRm(void) __attribute__ ((hot));

//===== XOR (mem),R
void srcXORmR(void) __attribute__ ((hot));

//===== OR R,(mem)
void srcORRm(void) __attribute__ ((hot));

//===== OR (mem),R
void srcORmR(void) __attribute__ ((hot));

//===== CP R,(mem)
void srcCPRm(void) __attribute__ ((hot));

//===== CP (mem),R
void srcCPmR(void) __attribute__ ((hot));

//=============================================================================
#endif
