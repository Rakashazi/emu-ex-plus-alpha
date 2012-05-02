/* asm-sparc.h -- dynamic assembler for Sparc
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
 * Last edited: Thu Jan 13 12:00:11 2000 by piumarta (Ian Piumarta) on pingu
 */

/* Addition: xnor and xnorcc */

#ifndef __ccg_asm_sparc_h
#define __ccg_asm_sparc_h

#if !defined(__sparc__)
# warning:
# warning: FOREIGN ARCHITECTURE SELECTED
# warning:
#endif

//#include "alpha"

typedef unsigned int insn;

#include "asm-common.h"

#define _UL(X)		((unsigned long)(X))

#define _ck_s(W,I)	(_siP(W,I) ? (_UL(I) & _MASK(W)) : ASMFAIL(  "signed integer "#I" too large for "#W"-bit field"))
#define _ck_u(W,I)    	(_uiP(W,I) ? (_UL(I) & _MASK(W)) : ASMFAIL("unsigned integer "#I" too large for "#W"-bit field"))
#define _ck_d(W,I)    	(_siP(W,I) ? (_UL(I) & _MASK(W)) : ASMFAIL(    "displacement "#I" too large for "#W"-bit field"))

#define _s13(I)         _ck_s(13,I)
				   
#define _u1(I)          _ck_u( 1,I)
#define _u2(I)          _ck_u( 2,I)
#define _u3(I)          _ck_u( 3,I)
#define _u4(I)          _ck_u( 4,I)
#define _u5(I)          _ck_u( 5,I)
#define _u6(I)          _ck_u( 6,I)
#define _u8(I)          _ck_u( 8,I)
#define _u9(I)          _ck_u( 9,I)
#define _u22(I)         _ck_u(22,I)

#define _d30(BD)        ((_UL(BD) - _UL(asm_pc))>>2)
#define _d22(BD)        ((asm_pass==2) ? _ck_d(22,_d30(BD)) : (_d30(BD) & _MASK(22)))

#define _HI(I)          (_UL(I) >>     (10))
#define _LO(I)          (_UL(I) & _MASK(10))

#define	_O7		15

#define _GEN(X)         ((*asm_pc= (X)), ++asm_pc)

/* register names */

#define _Rr(N)		( 0+(N))
#define _Rg(N)		( 0+(N))
#define _Ro(N)		( 8+(N))
#define _Rl(N)		(16+(N))
#define _Ri(N)		(24+(N))

/* primitive instruction formats [1, Figure 5-1, page 44] */

#define _1(  OP,			  DSP)	_GEN((_u2(OP)<<30)|											_d30(DSP))
	
#define _2i( OP,    RD, OP2,		  IMM)	_GEN((_u2(OP)<<30)|		(_u5(RD)<<25)|(_u3(OP2)<<22)|						_u22(IMM))
#define _2(  OP, A, CC, OP2,		  DSP)	_GEN((_u2(OP)<<30)|(_u1(A)<<29)|(_u4(CC)<<25)|(_u3(OP2)<<22)|						_d22(DSP))
	
#define _3(  OP,    RD, OP3, RS1, I, ASI, RS2)	_GEN((_u2(OP)<<30)|		(_u5(RD)<<25)|	(_u6(OP3)<<19)|(_u5(RS1)<<14)|(_u1(I)<<13)|(_u8(ASI)<<5)|_u5(RS2))
#define _3i( OP,    RD, OP3, RS1, I,	  IMM)	_GEN((_u2(OP)<<30)|		(_u5(RD)<<25)|	(_u6(OP3)<<19)|(_u5(RS1)<<14)|(_u1(I)<<13)|		_s13(IMM))
#define _3f( OP,    RD, OP3, RS1,    OPF, RS2)	_GEN((_u2(OP)<<30)|		(_u5(RD)<<25)|	(_u6(OP3)<<19)|(_u5(RS1)<<14)|		   (_u9(OPF)<<5)|_u5(RS2))

/* +++ intrinsic instructions  [1, Section B, page 87] */

#define ADDrrr(RS1, RS2, RD)		_3   (2, RD,  0, RS1, 0, 0, RS2)
#define ADDrir(RS1, IMM, RD)		_3i  (2, RD,  0, RS1, 1,    IMM)

#define ADDCCrrr(RS1, RS2, RD)		_3   (2, RD, 16, RS1, 0, 0, RS2)
#define ADDCCrir(RS1, IMM, RD)		_3i  (2, RD, 16, RS1, 1,    IMM)
					
#define ANDrrr(RS1, RS2, RD)		_3   (2, RD,  1, RS1, 0, 0, RS2)
#define ANDrir(RS1, IMM, RD)		_3i  (2, RD,  1, RS1, 1,    IMM)
#define ANDCCrrr(RS1, RS2, RD)		_3   (2, RD, 17, RS1, 0, 0, RS2)
#define ANDCCrir(RS1, IMM, RD)		_3i  (2, RD, 17, RS1, 1,    IMM)

#define ANDNrrr(RS1, RS2, RD)		_3   (2, RD,  5, RS1, 0, 0, RS2)
#define ANDNrir(RS1, IMM, RD)		_3i  (2, RD,  5, RS1, 1,    IMM)
#define ANDNCCrrr(RS1, RS2, RD)		_3   (2, RD, 21, RS1, 0, 0, RS2)
#define ANDNCCrir(RS1, IMM, RD)		_3i  (2, RD, 21, RS1, 1,    IMM)
			
#define BNi(DISP)			_2   (0, 0,  0, 2, DISP)
#define BN_Ai(DISP)			_2   (0, 1,  0, 2, DISP)
#define BEi(DISP)			_2   (0, 0,  1, 2, DISP)
#define BE_Ai(DISP)			_2   (0, 1,  1, 2, DISP)
#define BLEi(DISP)			_2   (0, 0,  2, 2, DISP)
#define BLE_Ai(DISP)			_2   (0, 1,  2, 2, DISP)
#define BLi(DISP)			_2   (0, 0,  3, 2, DISP)
#define BL_Ai(DISP)			_2   (0, 1,  3, 2, DISP)
#define BLEUi(DISP)			_2   (0, 0,  4, 2, DISP)
#define BLEU_Ai(DISP)			_2   (0, 1,  4, 2, DISP)
#define BCSi(DISP)			_2   (0, 0,  5, 2, DISP)
#define BCS_Ai(DISP)			_2   (0, 1,  5, 2, DISP)
#define BNEGi(DISP)			_2   (0, 0,  6, 2, DISP)
#define BNEG_Ai(DISP)			_2   (0, 1,  6, 2, DISP)
#define BVSi(DISP)			_2   (0, 0,  7, 2, DISP)
#define BVS_Ai(DISP)			_2   (0, 1,  7, 2, DISP)
					
#define BAi(DISP)			_2   (0, 0,  8, 2, DISP)
#define BA_Ai(DISP)			_2   (0, 1,  8, 2, DISP)
#define BNEi(DISP)			_2   (0, 0,  9, 2, DISP)
#define BNE_Ai(DISP)			_2   (0, 1,  9, 2, DISP)
#define BGi(DISP)			_2   (0, 0, 10, 2, DISP)
#define BG_Ai(DISP)			_2   (0, 1, 10, 2, DISP)
#define BGEi(DISP)			_2   (0, 0, 11, 2, DISP)
#define BGE_Ai(DISP)			_2   (0, 1, 11, 2, DISP)
#define BGUi(DISP)			_2   (0, 0, 12, 2, DISP)
#define BGU_Ai(DISP)			_2   (0, 1, 12, 2, DISP)
#define BCCi(DISP)			_2   (0, 0, 13, 2, DISP)
#define BCC_Ai(DISP)			_2   (0, 1, 13, 2, DISP)
#define BPOSi(DISP)			_2   (0, 0, 14, 2, DISP)
#define BPOS_Ai(DISP)			_2   (0, 1, 14, 2, DISP)
#define BVCi(DISP)			_2   (0, 0, 15, 2, DISP)
#define BVC_Ai(DISP)			_2   (0, 1, 15, 2, DISP)
					
#define CALLi(DISP)			_1   (1, DISP)
					
#define FLUSHrr(RS1, RS2)		_3   (2, 0, 0x3b, RS1, 0, 0, RS2)
#define FLUSHir(IMM, RS1)		_3i  (2, 0, 0x3b, RS1, 1,    IMM)
					
#define JMPLxr(RS1, RS2, RD)		_3   (2, RD, 56, RS1, 0, 0, RS2)
#define JMPLmr(RS1, IMM, RD)		_3i  (2, RD, 56, RS1, 1,    IMM)
					
#define LDxr(RS1, RS2, RD)		_3   (3, RD,  0, RS1, 0, 0, RS2)
#define LDmr(RS1, IMM, RD)		_3i  (3, RD,  0, RS1, 1,    IMM)
#define LDUBxr(RS1, RS2, RD)		_3   (3, RD,  1, RS1, 0, 0, RS2)
#define LDUBmr(RS1, IMM, RD)		_3i  (3, RD,  1, RS1, 1,    IMM)
#define LDUHxr(RS1, RS2, RD)		_3   (3, RD,  2, RS1, 0, 0, RS2)
#define LDUHmr(RS1, IMM, RD)		_3i  (3, RD,  2, RS1, 1,    IMM)
#define LDDxr(RS1, RS2, RD)		_3   (3, RD,  3, RS1, 0, 0, RS2)
#define LDDmr(RS1, IMM, RD)		_3i  (3, RD,  3, RS1, 1,    IMM)
#define LDSBxr(RS1, RS2, RD)		_3   (3, RD,  9, RS1, 0, 0, RS2)
#define LDSBmr(RS1, IMM, RD)		_3i  (3, RD,  9, RS1, 1,    IMM)
#define LDSHxr(RS1, RS2, RD)		_3   (3, RD, 10, RS1, 0, 0, RS2)
#define LDSHmr(RS1, IMM, RD)		_3i  (3, RD, 10, RS1, 1,    IMM)
					
#define ORrrr(RS1, RS2, RD)		_3   (2, RD,  2, RS1, 0, 0, RS2)
#define ORrir(RS1, IMM, RD)		_3i  (2, RD,  2, RS1, 1,    IMM)
#define ORCCrrr(RS1, RS2, RD)		_3   (2, RD, 18, RS1, 0, 0, RS2)
#define ORCCrir(RS1, IMM, RD)		_3i  (2, RD, 18, RS1, 1,    IMM)
	
#define ORNrrr(RS1, RS2, RD)		_3   (2, RD,  6, RS1, 0, 0, RS2)
#define ORNrir(RS1, IMM, RD)		_3i  (2, RD,  6, RS1, 1,    IMM)
#define ORNCCrrr(RS1, RS2, RD)		_3   (2, RD, 22, RS1, 0, 0, RS2)
#define ORNCCrir(RS1, IMM, RD)		_3i  (2, RD, 22, RS1, 1,    IMM)
				
#define RESTORErrr(RS1, RS2, RD)	_3   (2, RD, 61, RS1, 0, 0, RS2)
#define RESTORErir(RS1, IMM, RD)	_3i  (2, RD, 61, RS1, 1,    IMM)
					
#define SAVErrr(RS1, RS2, RD)		_3   (2, RD, 60, RS1, 0, 0, RS2)
#define SAVErir(RS1, IMM, RD)		_3i  (2, RD, 60, RS1, 1,    IMM)
					
#define SETHIir(IMM, RD)		_2i  (0, RD, 4, IMM)
					
#define SDIVrrr(RS1, RS2, RD)		_3   (2, RD, 15, RS1, 0, 0, RS2)
#define SDIVrir(RS1, IMM, RD)		_3i  (2, RD, 15, RS1, 1,    IMM)
#define SDIVCCrrr(RS1, RS2, RD)		_3   (2, RD, 31, RS1, 0, 0, RS2)
#define SDIVCCrir(RS1, IMM, RD)		_3i  (2, RD, 31, RS1, 1,    IMM)
					
#define SLLrrr(RS1, RS2, RD)		_3   (2, RD, 37, RS1, 0, 0, RS2)
#define SLLrir(RS1, IMM, RD)		_3i  (2, RD, 37, RS1, 1,    IMM)
					
#define SMULrrr(RS1, RS2, RD)		_3   (2, RD, 11, RS1, 0, 0, RS2)
#define SMULrir(RS1, IMM, RD)		_3i  (2, RD, 11, RS1, 1,    IMM)
#define SMULCCrrr(RS1, RS2, RD)		_3   (2, RD, 27, RS1, 0, 0, RS2)
#define SMULCCrir(RS1, IMM, RD)		_3i  (2, RD, 27, RS1, 1,    IMM)
					
#define SRArrr(RS1, RS2, RD)		_3   (2, RD, 39, RS1, 0, 0, RS2)
#define SRArir(RS1, IMM, RD)		_3i  (2, RD, 39, RS1, 1,    IMM)
					
#define SRLrrr(RS1, RS2, RD)		_3   (2, RD, 38, RS1, 0, 0, RS2)
#define SRLrir(RS1, IMM, RD)		_3i  (2, RD, 38, RS1, 1,    IMM)
					
#define STrx(RS, RD1, RD2)		_3   (3, RS,  4, RD1, 0, 0, RD2)
#define STrm(RS, RD, IMM)		_3i  (3, RS,  4, RD,  1,    IMM)
#define STBrx(RS, RD1, RD2)		_3   (3, RS,  5, RD1, 0, 0, RD2)
#define STBrm(RS, RD, IMM)		_3i  (3, RS,  5, RD,  1,    IMM)
#define STHrx(RS, RD1, RD2)		_3   (3, RS,  6, RD1, 0, 0, RD2)
#define STHrm(RS, RD, IMM)		_3i  (3, RS,  6, RD,  1,    IMM)
#define STDrx(RS, RD1, RD2)		_3   (3, RS,  7, RD1, 0, 0, RD2)
#define STDrm(RS, RD, IMM)		_3i  (3, RS,  7, RD,  1,    IMM)
					
#define STBAR()				_3i  (2, 0, 0x28, 15, 0, 0)
					
#define SUBrrr(RS1, RS2, RD)		_3   (2, RD,  4, RS1, 0, 0, RS2)
#define SUBrir(RS1, IMM, RD)		_3i  (2, RD,  4, RS1, 1,    IMM)
#define SUBCCrrr(RS1, RS2, RD)		_3   (2, RD, 20, RS1, 0, 0, RS2)
#define SUBCCrir(RS1, IMM, RD)		_3i  (2, RD, 20, RS1, 1,    IMM)
					
#define TADDCCrrr(RS1, RS2, RD)		_3   (2, RD, 32, RS1, 0, 0, RS2)
#define TADDCCrir(RS1, IMM, RD)		_3i  (2, RD, 32, RS1, 1,    IMM)
#define TADDCCTVrrr(RS1, RS2, RD)	_3   (2, RD, 34, RS1, 0, 0, RS2)
#define TADDCCTVrir(RS1, IMM, RD)	_3i  (2, RD, 34, RS1, 1,    IMM)
					
#define TSUBCCrrr(RS1, RS2, RD)		_3   (2, RD, 33, RS1, 0, 0, RS2)
#define TSUBCCrir(RS1, IMM, RD)		_3i  (2, RD, 33, RS1, 1,    IMM)
#define TSUBCCTVrrr(RS1, RS2, RD)	_3   (2, RD, 35, RS1, 0, 0, RS2)
#define TSUBCCTVrir(RS1, IMM, RD)	_3i  (2, RD, 35, RS1, 1,    IMM)
					
#define XORrrr(RS1, RS2, RD)		_3   (2, RD,  3, RS1, 0, 0, RS2)
#define XORrir(RS1, IMM, RD)		_3i  (2, RD,  3, RS1, 1,    IMM)
#define XNORrrr(RS1, RS2, RD)		_3   (2, RD,  7, RS1, 0, 0, RS2)
#define XNORrir(RS1, IMM, RD)		_3i  (2, RD,  7, RS1, 1,    IMM)
#define XORCCrrr(RS1, RS2, RD)		_3   (2, RD, 19, RS1, 0, 0, RS2)
#define XORCCrir(RS1, IMM, RD)		_3i  (2, RD, 19, RS1, 1,    IMM)
#define XNORCCrrr(RS1, RS2, RD)		_3   (2, RD, 23, RS1, 0, 0, RS2)
#define XNORCCrir(RS1, IMM, RD)		_3i  (2, RD, 23, RS1, 1,    IMM)
    
/* synonyms */	  
    
#define Bi(DISP)			BAi(DISP)
#define B_Ai(DISP)			BA_Ai(DISP)
#define BNZi(DISP)			BNEi(DISP)
#define BNZ_Ai(DISP)			BNE_Ai(DISP)
#define BZi(DISP)			BEi(DISP)
#define BZ_Ai(DISP)			BE_Ai(DISP)
#define BGEUi(DISP)			BCCi(DISP)
#define BGEU_Ai(DISP)			BCC_Ai(DISP)
#define BLUi(DISP)			BCSi(DISP)
#define BLU_Ai(DISP)			BCS_Ai(DISP)
    
/* synthetic instructions [1, Table A-1, page 85] */	
    
#define NOP()				SETHIir(0, 0)
					
#define CMPrr(RS1, RS2)			SUBCCrrr(RS1, RS2, 0)
#define CMPri(RS1, IMM)			SUBCCrir(RS1, IMM, 0)
					
#define JMPm(R,I)			JMPLmr(R,I, 0)
#define JMPx(R,S)			JMPLxr(R,S, 0)

#define CALLm(R,I)			JMPLmr(R,I, _O7)
#define CALLx(R,S)			JMPLxr(R,S, _O7)

#define TSTr(R)				ORCCrrr(0, R, 0)

#define RET()				JMPLmr(31,8 ,0)
#define RETL()				JMPLmr(15,8 ,0)

#define RESTORE()			RESTORErrr(0, 0, 0)
#define SAVE()				SAVErrr(0, 0, 0)

#define SETir(I,R)			(_siP(13,I) ? MOVir(I,R) : (SETHIir(_HI(I),R), ORrir(R,_LO(I),R)))

#define NOTrr(R,S)			XNORrrr(R, 0, S)
#define NOTr(R)				XNORrrr(R, 0, R)

#define NEGrr(R,S)			SUBrrr(0, R, S)
#define NEGr(R)				SUBrrr(0, R, R)
					
#define INCr(R)				ADDrir(R, 1, R)
#define INCir(I,R)			ADDrir(R, I, R)
#define INCCCr(R)			ADDCCrir(R, 1, R)
#define INCCCir(I,R)			ADDCCrir(R, I, R)

#define DECr(R)				SUBrir(R, 1, R)
#define DECir(I,R)			SUBrir(R, I, R)
#define DECCCr(R)			SUBCCrir(R, 1, R)
#define DECCCir(I,R)			SUBCCrir(R, I, R)

#define BTSTrr(R,S)			ANDCCrrr(R, S, 0)
#define BTSTir(R,I)			ANDCCrir(R, I, 0)

#define BSETrr(R,S)			ORrrr(R, S, S)
#define BSETir(R,I)			ORrir(R, I, R)

#define BCLRrr(R,S)			ANDNrrr(R, S, S)
#define BCLRir(R,I)			ANDNrir(R, I, R)

#define BTOGrr(R,S)			XORrrr(R, S, S)
#define BTOGir(R,I)			XORrir(R, I, R)

#define CLRr(R)				ORrrr(0, 0, R)
#define CLRBm(R,I)			STBrm(0, R,I)
#define CLRBx(R,S)			STBrm(0, R,S)
#define CLRHm(R,I)			STHrm(0, R,I)
#define CLRHx(R,S)			STHrm(0, R,S)
#define CLRm(R,I)			STrm(0, R,I)
#define CLRx(R,S)			STrm(0, R,S)

#define MOVrr(R,S)			ORrrr(0, R, S)
#define MOVir(I, R)			ORrir(0, I, R)
					
/*** References:                                                                              */
/*                                                                                            */
/* [1] SPARC International, "The SPARC Architecture Manual, Version 8", Prentice-Hall, 1992.  */

#endif /* __ccg_asm_sparc_h */
