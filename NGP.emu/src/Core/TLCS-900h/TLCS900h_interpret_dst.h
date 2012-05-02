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

	TLCS900h_interpret_dst.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_DST__
#define __TLCS900H_DST__
//=========================================================================

//===== LD (mem),#
void dstLDBi(void) __attribute__ ((hot));

//===== LD (mem),#
void dstLDWi(void) __attribute__ ((hot));

//===== POP (mem)
void dstPOPB(void) __attribute__ ((hot));

//===== POP (mem)
void dstPOPW(void) __attribute__ ((hot));

//===== LD (mem),(nn)
void dstLDBm16(void) __attribute__ ((hot));

//===== LD (mem),(nn)
void dstLDWm16(void) __attribute__ ((hot));

//===== LDA R,mem
void dstLDAW(void) __attribute__ ((hot));

//===== LDA R,mem
void dstLDAL(void) __attribute__ ((hot));

//===== ANDCF A,(mem)
void dstANDCFA(void) __attribute__ ((hot));

//===== ORCF A,(mem)
void dstORCFA(void) __attribute__ ((hot));

//===== XORCF A,(mem)
void dstXORCFA(void) __attribute__ ((hot));

//===== LDCF A,(mem)
void dstLDCFA(void) __attribute__ ((hot));

//===== STCF A,(mem)
void dstSTCFA(void) __attribute__ ((hot));

//===== LD (mem),R
void dstLDBR(void) __attribute__ ((hot));

//===== LD (mem),R
void dstLDWR(void) __attribute__ ((hot));

//===== LD (mem),R
void dstLDLR(void) __attribute__ ((hot));

//===== ANDCF #3,(mem)
void dstANDCF(void) __attribute__ ((hot));

//===== ORCF #3,(mem)
void dstORCF(void) __attribute__ ((hot));

//===== XORCF #3,(mem)
void dstXORCF(void) __attribute__ ((hot));

//===== LDCF #3,(mem)
void dstLDCF(void) __attribute__ ((hot));

//===== STCF #3,(mem)
void dstSTCF(void) __attribute__ ((hot));

//===== TSET #3,(mem)
void dstTSET(void) __attribute__ ((hot));

//===== RES #3,(mem)
void dstRES(void) __attribute__ ((hot));

//===== SET #3,(mem)
void dstSET(void) __attribute__ ((hot));

//===== CHG #3,(mem)
void dstCHG(void) __attribute__ ((hot));

//===== BIT #3,(mem)
void dstBIT(void) __attribute__ ((hot));

//===== JP cc,mem
void dstJP(void) __attribute__ ((hot));

//===== CALL cc,mem
void dstCALL(void) __attribute__ ((hot));

//===== RET cc
void dstRET(void) __attribute__ ((hot));

//=========================================================================
#endif
