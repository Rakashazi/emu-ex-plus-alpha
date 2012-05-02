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

	TLCS900h_interpret_reg.h

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

//---------------------------------------------------------------------------
*/

#ifndef __TLCS900H_REG__
#define __TLCS900H_REG__
//=========================================================================

//===== LD r,#
void regLDi(void) __attribute__ ((hot));

//===== PUSH r
void regPUSH(void) __attribute__ ((hot));

//===== POP r
void regPOP(void) __attribute__ ((hot));

//===== CPL r
void regCPL(void) __attribute__ ((hot));

//===== NEG r
void regNEG(void) __attribute__ ((hot));

//===== MUL rr,#
void regMULi(void) __attribute__ ((hot));

//===== MULS rr,#
void regMULSi(void) __attribute__ ((hot));

//===== DIV rr,#
void regDIVi(void) __attribute__ ((hot));

//===== DIVS rr,#
void regDIVSi(void) __attribute__ ((hot));

//===== LINK r,dd
void regLINK(void) __attribute__ ((hot));

//===== UNLK r
void regUNLK(void) __attribute__ ((hot));

//===== BS1F A,r
void regBS1F(void) __attribute__ ((hot));

//===== BS1B A,r
void regBS1B(void) __attribute__ ((hot));

//===== DAA r
void regDAA(void) __attribute__ ((hot));

//===== EXTZ r
void regEXTZ(void) __attribute__ ((hot));

//===== EXTS r
void regEXTS(void) __attribute__ ((hot));

//===== PAA r
void regPAA(void) __attribute__ ((hot));

//===== MIRR r
void regMIRR(void) __attribute__ ((hot));

//===== MULA r
void regMULA(void) __attribute__ ((hot));

//===== DJNZ r,d
void regDJNZ(void) __attribute__ ((hot));

//===== ANDCF #,r
void regANDCFi(void) __attribute__ ((hot));

//===== ORCF #,r
void regORCFi(void) __attribute__ ((hot));

//===== XORCF #,r
void regXORCFi(void) __attribute__ ((hot));

//===== LDCF #,r
void regLDCFi(void) __attribute__ ((hot));

//===== STCF #,r
void regSTCFi(void) __attribute__ ((hot));

//===== ANDCF A,r
void regANDCFA(void) __attribute__ ((hot));

//===== ORCF A,r
void regORCFA(void) __attribute__ ((hot));

//===== XORCF A,r
void regXORCFA(void) __attribute__ ((hot));

//===== LDCF A,r
void regLDCFA(void) __attribute__ ((hot));

//===== STCF A,r
void regSTCFA(void) __attribute__ ((hot));

//===== LDC cr,r
void regLDCcrr(void) __attribute__ ((hot));

//===== LDC r,cr
void regLDCrcr(void) __attribute__ ((hot));

//===== RES #,r
void regRES(void) __attribute__ ((hot));

//===== SET #,r
void regSET(void) __attribute__ ((hot));

//===== CHG #,r
void regCHG(void) __attribute__ ((hot));

//===== BIT #,r
void regBIT(void) __attribute__ ((hot));

//===== TSET #,r
void regTSET(void) __attribute__ ((hot));

//===== MINC1 #,r
void regMINC1(void) __attribute__ ((hot));

//===== MINC2 #,r
void regMINC2(void) __attribute__ ((hot));

//===== MINC4 #,r
void regMINC4(void) __attribute__ ((hot));

//===== MDEC1 #,r
void regMDEC1(void) __attribute__ ((hot));

//===== MDEC2 #,r
void regMDEC2(void) __attribute__ ((hot));

//===== MDEC4 #,r
void regMDEC4(void) __attribute__ ((hot));

//===== MUL RR,r
void regMUL(void) __attribute__ ((hot));

//===== MULS RR,r
void regMULS(void) __attribute__ ((hot));

//===== DIV RR,r
void regDIV(void) __attribute__ ((hot));

//===== DIVS RR,r
void regDIVS(void) __attribute__ ((hot));

//===== INC #3,r
void regINC(void) __attribute__ ((hot));

//===== DEC #3,r
void regDEC(void) __attribute__ ((hot));

//===== SCC cc,r
void regSCC(void) __attribute__ ((hot));

//===== LD R,r
void regLDRr(void) __attribute__ ((hot));

//===== LD r,R
void regLDrR(void) __attribute__ ((hot));

//===== ADD R,r
void regADD(void) __attribute__ ((hot));

//===== ADC R,r
void regADC(void) __attribute__ ((hot));

//===== SUB R,r
void regSUB(void) __attribute__ ((hot));

//===== SBC R,r
void regSBC(void) __attribute__ ((hot));

//===== LD r,#3
void regLDr3(void) __attribute__ ((hot));

//===== EX R,r
void regEX(void) __attribute__ ((hot));

//===== ADD r,#
void regADDi(void) __attribute__ ((hot));

//===== ADC r,#
void regADCi(void) __attribute__ ((hot));

//===== SUB r,#
void regSUBi(void) __attribute__ ((hot));

//===== SBC r,#
void regSBCi(void) __attribute__ ((hot));

//===== CP r,#
void regCPi(void) __attribute__ ((hot));

//===== AND r,#
void regANDi(void) __attribute__ ((hot));

//===== OR r,#
void regORi(void) __attribute__ ((hot));

//===== XOR r,#
void regXORi(void) __attribute__ ((hot));

//===== AND R,r
void regAND(void) __attribute__ ((hot));

//===== OR R,r
void regOR(void) __attribute__ ((hot));

//===== XOR R,r
void regXOR(void) __attribute__ ((hot));

//===== CP r,#3
void regCPr3(void) __attribute__ ((hot));

//===== CP R,r
void regCP(void) __attribute__ ((hot));

//===== RLC #,r
void regRLCi(void) __attribute__ ((hot));

//===== RRC #,r
void regRRCi(void) __attribute__ ((hot));

//===== RL #,r
void regRLi(void) __attribute__ ((hot));

//===== RR #,r
void regRRi(void) __attribute__ ((hot));

//===== SLA #,r
void regSLAi(void) __attribute__ ((hot));

//===== SRA #,r
void regSRAi(void) __attribute__ ((hot));

//===== SLL #,r
void regSLLi(void) __attribute__ ((hot));

//===== SRL #,r
void regSRLi(void) __attribute__ ((hot));

//===== RLC A,r
void regRLCA(void) __attribute__ ((hot));

//===== RRC A,r
void regRRCA(void) __attribute__ ((hot));

//===== RL A,r
void regRLA(void) __attribute__ ((hot));

//===== RR A,r
void regRRA(void) __attribute__ ((hot));

//===== SLA A,r
void regSLAA(void) __attribute__ ((hot));

//===== SRA A,r
void regSRAA(void) __attribute__ ((hot));

//===== SLL A,r
void regSLLA(void) __attribute__ ((hot));

//===== SRL A,r
void regSRLA(void) __attribute__ ((hot));

//=========================================================================
#endif
