/* Dynamic assembler for i386
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
 * Last edited: Thu Jan 13 14:44:35 2000 by piumarta (Ian Piumarta) on tif
 */

#ifndef __ccg_asm_i386_h
#define __ccg_asm_i386_h

#ifndef __i386__
# warning:
# warning: FOREIGN ARCHITECTURE SELECTED
# warning:
#endif

#include <sys/types.h>

typedef u_int8_t insn;

#include "asm-common.h"

#define _b00		0
#define _b01		1
#define _b10		2
#define _b11		3
	
#define _b000		0
#define _b001		1
#define _b010		2
#define _b011		3
#define _b100		4
#define _b101		5
#define _b110		6
#define _b111		7


/*** REGISTERS ***/	/* [size,,number] */


#define _AL		0x10
#define _CL		0x11
#define _DL		0x12
#define _BL		0x13
#define _AH		0x14
#define _CH		0x15
#define _DH		0x16
#define _BH		0x17

#define _AX		0x20
#define _CX		0x21
#define _DX		0x22
#define _BX		0x23
#define _SP		0x24
#define _BP		0x25
#define _SI		0x26
#define _DI		0x27

#define _EAX		0x40
#define _ECX		0x41
#define _EDX		0x42
#define _EBX		0x43
#define _ESP		0x44
#define _EBP		0x45
#define _ESI		0x46
#define _EDI		0x47

#define _rS(R)		((R)>>4)
#define _rN(R)		((R)&0x7)

#define _r0P(R)		((R)==0)
#define _rLP(R)		(((R)>=_AL)&&((R)<=_BL))
#define _rHP(R)		(((R)>=_AH)&&((R)<=_BH))
#define _r1P(R)		(_rS(R)==1)
#define _r2P(R)		(_rS(R)==2)
#define _r4P(R)		(_rS(R)==4)

#define _rL(R)		(_rLP(R) ? _rN(R) : ASMFAIL( "8-bit L register required"))
#define _rH(R)		(_rHP(R) ? _rN(R) : ASMFAIL( "8-bit H register required"))
#define _r1(R)		(_r1P(R) ? _rN(R) : ASMFAIL( "8-bit register required"))
#define _r2(R)		(_r2P(R) ? _rN(R) : ASMFAIL("16-bit register required"))
#define _r4(R)		(_r4P(R) ? _rN(R) : ASMFAIL("32-bit register required"))

#define _rAL(R)		(((R)==_AL)  ? _rN(R) : ASMFAIL( "AL register required"))
#define _rAX(R)		(((R)==_AX)  ? _rN(R) : ASMFAIL( "AX register required"))
#define _rEAX(R)	(((R)==_EAX) ? _rN(R) : ASMFAIL("EAX register required"))


/*** IMMEDIATES ***/


#define _s0P(I)		((I)==0)
#define _s1P(I)		_siP(8,I)
#define _s2P(I)		_siP(16,I)

#define _u1P(I)		(((I) & 0xffffff00) == 0)
#define _u2P(I)		(((I) & 0xffff0000) == 0)

#define _s1(I)		(_s1P(I) ? ((u_int8_t)(I)) : ASMFAIL( "8-bit signed immediate required"))
#define _s2(I)		(_s2P(I) ? ((u_int16_t)(I)) : ASMFAIL("16-bit signed immediate required"))

#define _u1(I)		(_u1P(I) ? (I) : ASMFAIL( "8-bit unsigned immediate required"))
#define _u2(I)		(_u2P(I) ? (I) : ASMFAIL("16-bit unsigned immediate required"))

#define _d1(I)		(_s1P(I) ? ((u_int8_t)(I)) : ASMFAIL( "8-bit displacement out of range"))


/*** ASSEMBLER ***/


#ifdef __STRICT_ANSI__

  static int _B(int x) {
    u_int8_t *pc= (u_int8_t *)asm_pc;
    *pc++= (u_int8_t)x;
    asm_pc= (insn *)pc;
    return x;
  }
 
  static int _W(int x) {
    u_int16_t *pc= (u_int16_t *)asm_pc;
    *pc++= (u_int16_t)x;
    asm_pc= (insn *)pc;
    return x;
  }
 
  static int _L(int x) {
    u_int32_t *pc= (u_int32_t *)asm_pc;
    *pc++= (u_int32_t)x;
    asm_pc= (insn *)pc;
    return x;
  }
 
# define _OFF4(D)	((u_int32_t)d - (u_int32_t)asm_pc)
# define _CKD1(D)	((asm_pass==1) ? _OFF4(D) : _d1(_OFF4(D)))

  static int8_t _D1(int d) {
    int8_t *pc= (int8_t *)asm_pc;
    int8_t off= 0;
    _B(0);
    off= (int8_t)_CKD1(d);
    *pc= off;
    return off;
  }

  static int32_t _D4(int d) {
    int32_t *pc= (int32_t *)asm_pc;
    int32_t off= 0;
    _L(0);
    off= _OFF4(d);
    *pc= off;
    return off;
  }

#else /* !__STRICT_ANSI__ */

# define _UC(X)		((u_int8_t   )(X))
# define _PUC(X)	((u_int8_t  *)(X))
# define _US(X)		((u_int16_t  )(X))
# define _PUS(X)	((u_int16_t *)(X))
# define _UL(X)		((u_int32_t  )(X))
# define _PUL(X)	((u_int32_t *)(X))

# ifdef __cplusplus
#  define _RPUC(X)	((u_int8_t  *&)(X))
#  define _RPUS(X)	((u_int16_t *&)(X))
#  define _RPUL(X)	((u_int32_t *&)(X))
#  define _B(B)		((*_RPUC(asm_pc)++)= _UC((B)&  0xff))
#  define _W(W)		((*_RPUS(asm_pc)++)= _US((W)&0xffff))
#  define _L(L)		((*_RPUL(asm_pc)++)= _UL((L)       ))
# else
#  define _B(B)		((*_PUC(asm_pc)++)= _UC((B)&  0xff))
#  define _W(W)		((*_PUS(asm_pc)++)= _US((W)&0xffff))
#  define _L(L)		((*_PUL(asm_pc)++)= _UL((L)       ))
# endif

# define _OFF4(D)	(_UL(D) - _UL(asm_pc))
# define _CKD1(D)	((asm_pass==1) ? _OFF4(D) : _d1(_OFF4(D)))

# define _D1(D)		(_B(0), ((*(_PUC(asm_pc)-1))= _CKD1(D)))
# define _D4(D)		(_L(0), ((*(_PUL(asm_pc)-1))= _OFF4(D)))

#endif /* !__STRICT_ANSI__ */

#if (_ASM_SAFETY==0)
# define _M(M)		(M)
# define _r(R)		(R)
# define _m(M)		(M)
# define _s(S)		(S)
# define _i(I)		(I)
# define _b(B)		(B)
#else
# define _M(M)		(((M)>3) ? ASMFAIL("internal error: mod = " #M) : (M))
# define _r(R)		(((R)>7) ? ASMFAIL("internal error: reg = " #R) : (R))
# define _m(M)		(((M)>7) ? ASMFAIL("internal error: r/m = " #M) : (M))
# define _s(S)		(((S)>3) ? ASMFAIL("internal error: memory scale = " #S) : (S))
# define _i(I)		(((I)>7) ? ASMFAIL("internal error: memory index = " #I) : (I))
# define _b(B)		(((B)>7) ? ASMFAIL("internal error: memory base = "  #B) : (B))
#endif

#define _Mrm(Md,R,M)	_B((_M(Md)<<6)|(_r(R)<<3)|_m(M))
#define _SIB(Sc,I, B)	_B((_s(Sc)<<6)|(_i(I)<<3)|_b(B))

#define _SCL(S)		((((S)==1) ? _b00 : \
			 (((S)==2) ? _b01 : \
			 (((S)==4) ? _b10 : \
			 (((S)==8) ? _b11 : ASMFAIL("illegal scale: " #S))))))

/* realignment via N-byte no-ops */

#define __ALIGN(N)	(((N)==15) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x90)) : \
			 ((N)==14) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00)) : \
			 ((N)==13) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0xb6),_B(0x00),_B(0x00),_B(0x00),_B(0x00)) : \
			 ((N)==12) ? (_B(0x8d),_B(0xb6),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0xb6),_B(0x00),_B(0x00),_B(0x00),_B(0x00)) : \
			 ((N)==11) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0x74),_B(0x26),_B(0x00)) : \
			 ((N)==10) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x8d),_B(0x76),_B(0x00)) : \
			 ((N)==	9) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x89),_B(0xf6)) : \
			 ((N)==	8) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00),_B(0x90)) : \
			 ((N)==	7) ? (_B(0x8d),_B(0xb4),_B(0x26),_B(0x00),_B(0x00),_B(0x00),_B(0x00)) : \
			 ((N)==	6) ? (_B(0x8d),_B(0xb6),_B(0x00),_B(0x00),_B(0x00),_B(0x00)) : \
			 ((N)==	5) ? (_B(0x90),_B(0x8d),_B(0x74),_B(0x26),_B(0x00)) : \
			 ((N)==	4) ? (_B(0x8d),_B(0x74),_B(0x26),_B(0x00)) : \
			 ((N)==	3) ? (_B(0x8d),_B(0x76),_B(0x00)) : \
			 ((N)==	2) ? (_B(0x89),_B(0xf6)) : \
			 ((N)==	1) ? (_B(0x90)) : \
			 ((N)== 0) ? 0 : \
			 ASMFAIL(".align argument too large"))

/* memory subformats */

#define _r_D(	R, D	  )	(_Mrm(_b00,_rN(R),_b101 )		             ,_L(D))
#define _r_0B(	R,   B    )	(_Mrm(_b00,_rN(R),_r4(B))			           )
#define _r_0BIS(R,   B,I,S)	(_Mrm(_b00,_rN(R),_b100 ),_SIB(_SCL(S),_r4(I),_r4(B))      )
#define _r_1B(	R, D,B    )	(_Mrm(_b01,_rN(R),_r4(B))		             ,_B(D))
#define _r_1BIS(R, D,B,I,S)	(_Mrm(_b01,_rN(R),_b100 ),_SIB(_SCL(S),_r4(I),_r4(B)),_B(D))
#define _r_4B(	R, D,B    )	(_Mrm(_b10,_rN(R),_r4(B))		             ,_L(D))
#define _r_4BIS(R, D,B,I,S)	(_Mrm(_b10,_rN(R),_b100 ),_SIB(_SCL(S),_r4(I),_r4(B)),_L(D))
 
#define _r_DB(  R, D,B    )	(((_s0P(D)&&((B)!=_EBP)) ? _r_0B  (R,  B    ) : (_s1P(D) ? _r_1B(  R,D,B    ) : _r_4B(  R,D,B    ))))
#define _r_DBIS(R, D,B,I,S)	( (_s0P(D)               ? _r_0BIS(R,  B,I,S) : (_s1P(D) ? _r_1BIS(R,D,B,I,S) : _r_4BIS(R,D,B,I,S))))

#define _r_X(   R, D,B,I,S)	(_r0P(I) ? (_r0P(B)   ? _r_D   (R,D            )   : \
				           (_ESP==(B) ? _r_DBIS(R,D,_ESP,_ESP,1)   : \
						        _r_DB  (R,D,   B       ))) : \
				 (((I)!=_ESP)         ? _r_DBIS(R,D,   B,   I,S)   : \
						        ASMFAIL("illegal index register: %esp")))

/* instruction formats */

/*       _format                                                     Opcd         ModR/M dN(rB,rI,Sc)     imm... */
	
#define  _d16()                                    (		  _B(0x66       )                                 )
#define   _O(        OP                         )  (		  _B(  OP       )                                 )
#define   _Or(       OP,R                       )  (		  _B( (OP)|_r(R))                                 )
#define  _OO(        OP                         )  ( _B((OP)>>8), _B( (OP)      )                                 )
#define  _OOr(       OP,R                       )  ( _B((OP)>>8), _B( (OP)|_r(R))                                 )
#define   _Os(       OP,B                       )  (    _s1P(B) ? _B(((OP)|_b10)) : _B(OP)                        )
#define     _sW(                             W  )  (		                       _s1P(W) ? _B(W):_W(W)      )
#define     _sL(                             L  )  (		                       _s1P(L) ? _B(L):_L(L)      )
#define   _O_W(      OP                     ,W  )  (        _O      (  OP  )                          ,_W(W)      )
#define   _O_D1(     OP                     ,D  )  (        _O      (  OP  )                         ,_D1(D)      )
#define   _O_D4(     OP                     ,D  )  (        _O      (  OP  )                         ,_D4(D)      )
#define  _OO_D4(     OP                     ,D  )  (       _OO      (  OP  )                         ,_D4(D)      )
#define   _Os_sW(    OP                     ,W  )  (        _Os     (  OP,W)                         ,_sW(W)      )
#define   _Os_sL(    OP                     ,L  )  (        _Os     (  OP,L)                         ,_sL(L)      )
#define   _O_W_B(    OP                     ,W,B)  (        _O      (  OP  )                          ,_W(W),_B(B))
#define   _Or_B(     OP,R                   ,B  )  (        _Or     (  OP,R)                          ,_B(B)      )
#define   _Or_W(     OP,R                   ,W  )  (        _Or     (  OP,R)                          ,_W(W)      )
#define   _Or_L(     OP,R                   ,L  )  (        _Or     (  OP,R)                          ,_L(L)      )
#define   _O_Mrm(    OP  ,MO,R,M                )  (        _O      (  OP  ),_Mrm(MO,R,M            )             )
#define  _OO_Mrm(    OP  ,MO,R,M                )  (       _OO      (  OP  ),_Mrm(MO,R,M            )             )
#define   _O_Mrm_B(  OP  ,MO,R,M            ,B  )  (        _O      (  OP  ),_Mrm(MO,R,M            ) ,_B(B)      )
#define   _O_Mrm_W(  OP  ,MO,R,M            ,W  )  (        _O      (  OP  ),_Mrm(MO,R,M            ) ,_W(W)      )
#define   _O_Mrm_L(  OP  ,MO,R,M            ,L  )  (        _O      (  OP  ),_Mrm(MO,R,M            ) ,_L(L)      )
#define  _OO_Mrm_B(  OP  ,MO,R,M            ,B  )  (       _OO      (  OP  ),_Mrm(MO,R,M            ) ,_B(B)      )
#define   _Os_Mrm_sW(OP  ,MO,R,M            ,W  )  (        _Os     (  OP,W),_Mrm(MO,R,M            ),_sW(W)      )
#define   _Os_Mrm_sL(OP  ,MO,R,M            ,L  )  (        _Os     (  OP,L),_Mrm(MO,R,M            ),_sL(L)      )
#define   _O_r_X(    OP     ,R  ,MD,MB,MI,MS    )  (        _O      (  OP  ),_r_X(   R  ,MD,MB,MI,MS)             )
#define  _OO_r_X(    OP     ,R  ,MD,MB,MI,MS    )  (       _OO      (  OP  ),_r_X(   R  ,MD,MB,MI,MS)             )
#define   _O_r_X_B(  OP     ,R  ,MD,MB,MI,MS,B  )  (        _O      (  OP  ),_r_X(   R  ,MD,MB,MI,MS) ,_B(B)      )
#define   _O_r_X_W(  OP     ,R  ,MD,MB,MI,MS,W  )  (        _O      (  OP  ),_r_X(   R  ,MD,MB,MI,MS) ,_W(W)      )
#define   _O_r_X_L(  OP     ,R  ,MD,MB,MI,MS,L  )  (        _O      (  OP  ),_r_X(   R  ,MD,MB,MI,MS) ,_L(L)      )
#define  _OO_r_X_B(  OP     ,R  ,MD,MB,MI,MS,B  )  (       _OO      (  OP  ),_r_X(   R  ,MD,MB,MI,MS) ,_B(B)      )
#define   _Os_r_X_sW(OP     ,R  ,MD,MB,MI,MS,W  )  (        _Os     (  OP,W),_r_X(   R  ,MD,MB,MI,MS),_sW(W)      )
#define   _Os_r_X_sL(OP     ,R  ,MD,MB,MI,MS,L  )  (        _Os     (  OP,L),_r_X(   R  ,MD,MB,MI,MS),_sL(L)      )
#define   _O_X_B(    OP         ,MD,MB,MI,MS,B  )  (        _O_r_X_B(  OP           ,0  ,MD,MB,MI,MS     ,B)      )
#define   _O_X_W(    OP         ,MD,MB,MI,MS,W  )  (        _O_r_X_W(  OP           ,0  ,MD,MB,MI,MS     ,W)      )
#define   _O_X_L(    OP         ,MD,MB,MI,MS,L  )  (        _O_r_X_L(  OP           ,0  ,MD,MB,MI,MS     ,L)      )
#define  _wO(        OP                         )  (_d16(), _O(        OP                                  )      )
#define  _wOr(       OP,R                       )  (_d16(), _Or(       OP,R                                )      )
#define  _wOr_W(     OP,R                   ,W  )  (_d16(), _Or_W(     OP,R                              ,W)      )
#define  _wOs_sW(    OP                     ,W  )  (_d16(), _Os_sW(    OP                                ,W)      )
#define  _wO_Mrm(    OP  ,MO,R,M                )  (_d16(), _O_Mrm(    OP        ,MO,R,M                   )      )
#define _wOO_Mrm(    OP  ,MO,R,M                )  (_d16(),_OO_Mrm(    OP        ,MO,R,M                   )      )
#define  _wO_Mrm_B(  OP  ,MO,R,M            ,B  )  (_d16(), _O_Mrm_B(  OP        ,MO,R,M                 ,B)      )
#define _wOO_Mrm_B(  OP  ,MO,R,M            ,B  )  (_d16(),_OO_Mrm_B(  OP        ,MO,R,M                 ,B)      )
#define  _wO_Mrm_W(  OP  ,MO,R,M            ,W  )  (_d16(), _O_Mrm_W(  OP        ,MO,R,M                 ,W)      )
#define  _wOs_Mrm_sW(OP  ,MO,R,M            ,W  )  (_d16(), _Os_Mrm_sW(OP        ,MO,R,M                 ,W)      )
#define  _wO_X_W(    OP         ,MD,MB,MI,MS,W  )  (_d16(), _O_X_W(    OP               ,MD,MB,MI,MS     ,W)      )
#define  _wO_r_X(    OP     ,R  ,MD,MB,MI,MS    )  (_d16(), _O_r_X(    OP           ,R  ,MD,MB,MI,MS       )      )
#define _wOO_r_X(    OP     ,R  ,MD,MB,MI,MS    )  (_d16(),_OO_r_X(    OP           ,R  ,MD,MB,MI,MS       )      )
#define  _wO_r_X_B(  OP     ,R  ,MD,MB,MI,MS,B  )  (_d16(), _O_r_X_B(  OP           ,R  ,MD,MB,MI,MS     ,B)      )
#define _wOO_r_X_B(  OP     ,R  ,MD,MB,MI,MS,B  )  (_d16(),_OO_r_X_B(  OP           ,R  ,MD,MB,MI,MS     ,B)      )
#define  _wO_r_X_W(  OP     ,R  ,MD,MB,MI,MS,W  )  (_d16(), _O_r_X_W(  OP           ,R  ,MD,MB,MI,MS     ,W)      )
#define  _wOs_r_X_sW(OP     ,R  ,MD,MB,MI,MS,W  )  (_d16(), _Os_r_X_sW(OP           ,R  ,MD,MB,MI,MS     ,W)      )

/* +++ fully-qualified intrinsic instructions */

/*					_format		 Opcd		,Mod ,r	    ,m		,mem=dsp+sib	,imm...	*/
	
#define ADCBrr(RS, RD)			_O_Mrm		(0x10		,_b11,_r1(RS),_r1(RD)				)
#define ADCBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x12		     ,_r1(RD)		,MD,MB,MI,MS		)
#define ADCBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x10		     ,_r1(RS)		,MD,MB,MI,MS		)
#define ADCBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b010  ,_r1(RD)			,_s1(IM))
#define ADCBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b010		,MD,MB,MI,MS	,_s1(IM))
	
#define ADCWrr(RS, RD)			_wO_Mrm		(0x11		,_b11,_r2(RS),_r2(RD)				)
#define ADCWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x13		     ,_r2(RD)		,MD,MB,MI,MS		)
#define ADCWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x11		     ,_r2(RS)		,MD,MB,MI,MS		)
#define ADCWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b010  ,_r2(RD)			,_s2(IM))
#define ADCWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b010		,MD,MB,MI,MS	,_s2(IM))
	
#define ADCLrr(RS, RD)			_O_Mrm		(0x11		,_b11,_r4(RS),_r4(RD)				)
#define ADCLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x13		     ,_r4(RD)		,MD,MB,MI,MS		)
#define ADCLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x11		     ,_r4(RS)		,MD,MB,MI,MS		)
#define ADCLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b010  ,_r4(RD)			,IM	)
#define ADCLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b010		,MD,MB,MI,MS	,IM	)
	
	
#define ADDBrr(RS, RD)			_O_Mrm		(0x00		,_b11,_r1(RS),_r1(RD)				)
#define ADDBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x02		     ,_r1(RD)		,MD,MB,MI,MS		)
#define ADDBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x00		     ,_r1(RS)		,MD,MB,MI,MS		)
#define ADDBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b000  ,_r1(RD)			,_s1(IM))
#define ADDBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b000		,MD,MB,MI,MS	,_s1(IM))
	
#define ADDWrr(RS, RD)			_wO_Mrm		(0x01		,_b11,_r2(RS),_r2(RD)				)
#define ADDWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x03		     ,_r2(RD)		,MD,MB,MI,MS		)
#define ADDWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x01		     ,_r2(RS)		,MD,MB,MI,MS		)
#define ADDWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b000  ,_r2(RD)			,_s2(IM))
#define ADDWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b000		,MD,MB,MI,MS	,_s2(IM))
	
#define ADDLrr(RS, RD)			_O_Mrm		(0x01		,_b11,_r4(RS),_r4(RD)				)
#define ADDLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x03		     ,_r4(RD)		,MD,MB,MI,MS		)
#define ADDLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x01		     ,_r4(RS)		,MD,MB,MI,MS		)
#define ADDLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b000  ,_r4(RD)			,IM	)
#define ADDLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b000		,MD,MB,MI,MS	,IM	)
	
	
#define ANDBrr(RS, RD)			_O_Mrm		(0x20		,_b11,_r1(RS),_r1(RD)				)
#define ANDBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x22		     ,_r1(RD)		,MD,MB,MI,MS		)
#define ANDBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x20		     ,_r1(RS)		,MD,MB,MI,MS		)
#define ANDBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b100  ,_r1(RD)			,_s1(IM))
#define ANDBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b100		,MD,MB,MI,MS	,_s1(IM))
	
#define ANDWrr(RS, RD)			_wO_Mrm		(0x21		,_b11,_r2(RS),_r2(RD)				)
#define ANDWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x23		     ,_r2(RD)		,MD,MB,MI,MS		)
#define ANDWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x21		     ,_r2(RS)		,MD,MB,MI,MS		)
#define ANDWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b100  ,_r2(RD)			,_s2(IM))
#define ANDWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b100		,MD,MB,MI,MS	,_s2(IM))
	
#define ANDLrr(RS, RD)			_O_Mrm		(0x21		,_b11,_r4(RS),_r4(RD)				)
#define ANDLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x23		     ,_r4(RD)		,MD,MB,MI,MS		)
#define ANDLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x21		     ,_r4(RS)		,MD,MB,MI,MS		)
#define ANDLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b100  ,_r4(RD)			,IM	)
#define ANDLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b100		,MD,MB,MI,MS	,IM	)


#define BSWAPLr(R)			_OOr		(0x0fc8,_r4(R)							)


#define BTWir(IM,RD)			_wOO_Mrm_B	(0x0fba		,_b11,_b100  ,_r2(RD)			,_u1(IM))
#define BTWim(IM,MD,MB,MI,MS)		_wOO_r_X_B	(0x0fba		     ,_b100		,MD,MB,MI,MS	,_u1(IM))
#define BTWrr(RS,RD)			_wOO_Mrm	(0x0fa3		,_b11,_r2(RS),_r2(RD)				)
#define BTWrm(RS,MD,MB,MI,MS)		_wOO_r_X	(0x0fa3		     ,_r2(RS)		,MD,MB,MI,MS		)

#define BTLir(IM,RD)			_OO_Mrm_B	(0x0fba		,_b11,_b100  ,_r4(RD)			,_u1(IM))
#define BTLim(IM,MD,MB,MI,MS)		_OO_r_X_B	(0x0fba		     ,_b100		,MD,MB,MI,MS	,_u1(IM))
#define BTLrr(RS,RD)			_OO_Mrm		(0x0fa3		,_b11,_r4(RS),_r4(RD)				)
#define BTLrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fa3		     ,_r4(RS)		,MD,MB,MI,MS		)


#define BTCWir(IM,RD)			_wOO_Mrm_B	(0x0fba		,_b11,_b111  ,_r2(RD)			,_u1(IM))
#define BTCWim(IM,MD,MB,MI,MS)		_wOO_r_X_B	(0x0fba		     ,_b111		,MD,MB,MI,MS	,_u1(IM))
#define BTCWrr(RS,RD)			_wOO_Mrm	(0x0fbb		,_b11,_r2(RS),_r2(RD)				)
#define BTCWrm(RS,MD,MB,MI,MS)		_wOO_r_X	(0x0fbb		     ,_r2(RS)		,MD,MB,MI,MS		)

#define BTCLir(IM,RD)			_OO_Mrm_B	(0x0fba		,_b11,_b111  ,_r4(RD)			,_u1(IM))
#define BTCLim(IM,MD,MB,MI,MS)		_OO_r_X_B	(0x0fba		     ,_b111		,MD,MB,MI,MS	,_u1(IM))
#define BTCLrr(RS,RD)			_OO_Mrm		(0x0fbb		,_b11,_r4(RS),_r4(RD)				)
#define BTCLrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fbb		     ,_r4(RS)		,MD,MB,MI,MS		)


#define BTRWir(IM,RD)			_wOO_Mrm_B	(0x0fba		,_b11,_b110  ,_r2(RD)			,_u1(IM))
#define BTRWim(IM,MD,MB,MI,MS)		_wOO_r_X_B	(0x0fba		     ,_b110		,MD,MB,MI,MS	,_u1(IM))
#define BTRWrr(RS,RD)			_wOO_Mrm	(0x0fb3		,_b11,_r2(RS),_r2(RD)				)
#define BTRWrm(RS,MD,MB,MI,MS)		_wOO_r_X	(0x0fb3		     ,_r2(RS)		,MD,MB,MI,MS		)

#define BTRLir(IM,RD)			_OO_Mrm_B	(0x0fba		,_b11,_b110  ,_r4(RD)			,_u1(IM))
#define BTRLim(IM,MD,MB,MI,MS)		_OO_r_X_B	(0x0fba		     ,_b110		,MD,MB,MI,MS	,_u1(IM))
#define BTRLrr(RS,RD)			_OO_Mrm		(0x0fb3		,_b11,_r4(RS),_r4(RD)				)
#define BTRLrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fb3		     ,_r4(RS)		,MD,MB,MI,MS		)


#define BTSWir(IM,RD)			_wOO_Mrm_B	(0x0fba		,_b11,_b101  ,_r2(RD)			,_u1(IM))
#define BTSWim(IM,MD,MB,MI,MS)		_wOO_r_X_B	(0x0fba		     ,_b101		,MD,MB,MI,MS	,_u1(IM))
#define BTSWrr(RS,RD)			_wOO_Mrm	(0x0fab		,_b11,_r2(RS),_r2(RD)				)
#define BTSWrm(RS,MD,MB,MI,MS)		_wOO_r_X	(0x0fab		     ,_r2(RS)		,MD,MB,MI,MS		)

#define BTSLir(IM,RD)			_OO_Mrm_B	(0x0fba		,_b11,_b101  ,_r4(RD)			,_u1(IM))
#define BTSLim(IM,MD,MB,MI,MS)		_OO_r_X_B	(0x0fba		     ,_b101		,MD,MB,MI,MS	,_u1(IM))
#define BTSLrr(RS,RD)			_OO_Mrm		(0x0fab		,_b11,_r4(RS),_r4(RD)				)
#define BTSLrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fab		     ,_r4(RS)		,MD,MB,MI,MS		)


#define CALLm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D4 (0xe8			,(int)(MD)		) : \
					((            _r0P(MI)) ? _O_Mrm(0xff,_b11,_b010,_r4(MB)			) : \
								  _O_r_X(0xff,     _b010	,(int)(MD),MB,MI,MS)	))

#define CBW()				_O		(0x98								)
#define CLC()				_O		(0xf8								)
#define CLD()				_O		(0xfc								)
#define CLTD()				_O		(0x99								)
#define CMC()				_O		(0xf5								)


#define CMPBrr(RS, RD)			_O_Mrm		(0x38		,_b11,_r1(RS),_r1(RD)				)
#define CMPBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x3a		     ,_r1(RD)		,MD,MB,MI,MS		)
#define CMPBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x38		     ,_r1(RS)		,MD,MB,MI,MS		)
#define CMPBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b111  ,_r1(RD)			,_s1(IM))
#define CMPBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b111		,MD,MB,MI,MS	,_s1(IM))
	
#define CMPWrr(RS, RD)			_wO_Mrm		(0x39		,_b11,_r2(RS),_r2(RD)				)
#define CMPWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x3b		     ,_r2(RD)		,MD,MB,MI,MS		)
#define CMPWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x39		     ,_r2(RS)		,MD,MB,MI,MS		)
#define CMPWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b111  ,_r2(RD)			,_s2(IM))
#define CMPWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b111		,MD,MB,MI,MS	,_s2(IM))
	
#define CMPLrr(RS, RD)			_O_Mrm		(0x39		,_b11,_r4(RS),_r4(RD)				)
#define CMPLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x3b		     ,_r4(RD)		,MD,MB,MI,MS		)
#define CMPLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x39		     ,_r4(RS)		,MD,MB,MI,MS		)
#define CMPLir(IM, RD)			_O_Mrm_L	(0x81		,_b11,_b111  ,_r4(RD)			,IM	)
#define CMPLim(IM, MD, MB, MI, MS)	_O_r_X_L	(0x81		     ,_b111		,MD,MB,MI,MS	,IM	)
	

#define CWD()				_O		(0x99								)


#define CMPXCHGBrr(RS,RD)		_OO_Mrm		(0x0fb0		,_b11,_r1(RS),_r1(RD)				)
#define CMPXCHGBrm(RS,MD,MB,MI,MS)	_OO_r_X		(0x0fb0		     ,_r1(RS)		,MD,MB,MI,MS		)

#define CMPXCHGWrr(RS,RD)		_wOO_Mrm	(0x0fb1		,_b11,_r2(RS),_r2(RD)				)
#define CMPXCHGWrm(RS,MD,MB,MI,MS)	_wOO_r_X	(0x0fb1		     ,_r2(RS)		,MD,MB,MI,MS		)

#define CMPXCHGLrr(RS,RD)		_OO_Mrm		(0x0fb1		,_b11,_r4(RS),_r4(RD)				)
#define CMPXCHGLrm(RS,MD,MB,MI,MS)	_OO_r_X		(0x0fb1		     ,_r4(RS)		,MD,MB,MI,MS		)


#define DECBr(RD)			_O_Mrm		(0xfe		,_b11,_b001  ,_r1(RD)				)
#define DECBm(MD,MB,MI,MS)		_O_r_X		(0xfe		     ,_b001		,MD,MB,MI,MS		)

#define DECWr(RD)			_wOr		(0x48,_r2(RD)							)
#define DECWm(MD,MB,MI,MS)		_wO_r_X		(0xff		     ,_b001		,MD,MB,MI,MS		)

#define DECLr(RD)			_Or		(0x48,_r4(RD)							)
#define DECLm(MD,MB,MI,MS)		_O_r_X		(0xff		     ,_b001		,MD,MB,MI,MS		)


#define DIVBrr(RS,RD)		( _rAL (RD) +	_O_Mrm	(0xf6		,_b11,_b110  ,_r1(RS)				) )
#define DIVBmr(MD,MB,MI,MS,RD)	( _rAL (RD) +	_O_r_X	(0xf6		     ,_b110		,MD,MB,MI,MS		) )

#define DIVWrr(RS,RD)		( _rAX (RD) +	_wO_Mrm(0xf7		,_b11,_b110  ,_r2(RS)				) )
#define DIVWmr(MD,MB,MI,MS,RD)	( _rAX (RD) +	_wO_r_X(0xf7		     ,_b110		,MD,MB,MI,MS		) )

#define DIVLrr(RS,RD)		( _rEAX(RD) +	_O_Mrm	(0xf7		,_b11,_b110  ,_r4(RS)				) )
#define DIVLmr(MD,MB,MI,MS,RD)	( _rEAX(RD) +	_O_r_X	(0xf7		     ,_b110		,MD,MB,MI,MS		) )


#define ENTERii(W, B)			_O_W_B		(0xc8						  ,_s2(W),_s1(B))
#define HLT()				_O		(0xf4								)


#define IDIVBrr(RS,RD)		( _rAL (RD) +	_O_Mrm	(0xf6		,_b11,_b111  ,_r1(RS)				) )
#define IDIVBmr(MD,MB,MI,MS,RD)	( _rAL (RD) +	_O_r_X	(0xf6		     ,_b111		,MD,MB,MI,MS		) )

#define IDIVWrr(RS,RD)		( _rAX (RD) +	_wO_Mrm	(0xf7		,_b11,_b111  ,_r2(RS)				) )
#define IDIVWmr(MD,MB,MI,MS,RD)	( _rAX (RD) +	_wO_r_X	(0xf7		     ,_b111		,MD,MB,MI,MS		) )

#define IDIVLrr(RS,RD)		( _rEAX(RD) +	_O_Mrm	(0xf7		,_b11,_b111  ,_r4(RS)				) )
#define IDIVLmr(MD,MB,MI,MS,RD)	( _rEAX(RD) +	_O_r_X	(0xf7		     ,_b111		,MD,MB,MI,MS		) )


#define IMULWrr(RS,RD)			_wOO_Mrm	(0x0faf		,_b11,_r2(RS),_r2(RD)				)
#define IMULWmr(MD,MB,MI,MS,RD)		_wOO_r_X	(0x0faf		     ,_r2(RD)		,MD,MB,MI,MS		)
#define IMULWirr(IM,RS,RD)		_wOs_Mrm_sW	(0x69		,_b11,_r2(RS),_r2(RD)			,IM	)
#define IMULWimr(IM,MD,MB,MI,MS,RD)	_wOs_r_X_sW	(0x69		     ,_r2(RD)		,MD,MB,MI,MS	,IM	)

#define IMULLrr(RS,RD)			_OO_Mrm		(0x0faf		,_b11,_r4(RD),_r4(RS)				)
#define IMULLmr(MD,MB,MI,MS,RD)		_OO_r_X		(0x0faf		     ,_r4(RD)		,MD,MB,MI,MS		)
#define IMULLirr(IM,RS,RD)		_Os_Mrm_sL	(0x69		,_b11,_r4(RS),_r4(RD)			,IM	)
#define IMULLimr(IM,MD,MB,MI,MS,RD)	_Os_r_X_sL	(0x69		     ,_r4(RD)		,MD,MB,MI,MS	,IM	)


#define INCBr(RD)			_O_Mrm		(0xfe		,_b11,_b000  ,_r1(RD)				)
#define INCBm(MD,MB,MI,MS)		_O_r_X		(0xfe		     ,_b000		,MD,MB,MI,MS		)

#define INCWr(RD)			_wOr		(0x40,_r2(RD)							)
#define INCWm(MD,MB,MI,MS)		_wO_r_X		(0xff		     ,_b000		,MD,MB,MI,MS		)

#define INCLr(RD)			_Or		(0x40,_r4(RD)							)
#define INCLm(MD,MB,MI,MS)		_O_r_X		(0xff		     ,_b000		,MD,MB,MI,MS		)


#define INVD()				_OO		(0x0f08								)
#define INVLPGm(MD, MB, MI, MS)		_OO_r_X		(0x0f01		     ,_b111		,MD,MB,MI,MS		)

#define INTi(I)                         (_O(0xcd), _B(I))

#define JCXZm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe3			,MD			) : \
								  ASMFAIL("illegal mode in jcxz"))

#define JECXZm JCXZm

#define JCCSim(CC,D,B,I,S)		((_r0P(B) && _r0P(I)) ? _O_D1	(0x70|(CC)		,(int)(D)		) : \
							        ASMFAIL("illegal mode in conditional jump"))

#define JOSm(D,B,I,S)			JCCSim(0x0,D,B,I,S)
#define JNOSm(D,B,I,S)			JCCSim(0x1,D,B,I,S)
#define JBSm(D,B,I,S)			JCCSim(0x2,D,B,I,S)
#define JNAESm(D,B,I,S)			JCCSim(0x2,D,B,I,S)
#define JNBSm(D,B,I,S)			JCCSim(0x3,D,B,I,S)
#define JAESm(D,B,I,S)			JCCSim(0x3,D,B,I,S)
#define JESm(D,B,I,S)			JCCSim(0x4,D,B,I,S)
#define JZSm(D,B,I,S)			JCCSim(0x4,D,B,I,S)
#define JNESm(D,B,I,S)			JCCSim(0x5,D,B,I,S)
#define JNZSm(D,B,I,S)			JCCSim(0x5,D,B,I,S)
#define JBESm(D,B,I,S)			JCCSim(0x6,D,B,I,S)
#define JNASm(D,B,I,S)			JCCSim(0x6,D,B,I,S)
#define JNBESm(D,B,I,S)			JCCSim(0x7,D,B,I,S)
#define JASm(D,B,I,S)			JCCSim(0x7,D,B,I,S)
#define JSSm(D,B,I,S)			JCCSim(0x8,D,B,I,S)
#define JNSSm(D,B,I,S)			JCCSim(0x9,D,B,I,S)
#define JPSm(D,B,I,S)			JCCSim(0xa,D,B,I,S)
#define JPESm(D,B,I,S)			JCCSim(0xa,D,B,I,S)
#define JNPSm(D,B,I,S)			JCCSim(0xb,D,B,I,S)
#define JPOSm(D,B,I,S)			JCCSim(0xb,D,B,I,S)
#define JLSm(D,B,I,S)			JCCSim(0xc,D,B,I,S)
#define JNGESm(D,B,I,S)			JCCSim(0xc,D,B,I,S)
#define JNLSm(D,B,I,S)			JCCSim(0xd,D,B,I,S)
#define JGESm(D,B,I,S)			JCCSim(0xd,D,B,I,S)
#define JLESm(D,B,I,S)			JCCSim(0xe,D,B,I,S)
#define JNGSm(D,B,I,S)			JCCSim(0xe,D,B,I,S)
#define JNLESm(D,B,I,S)			JCCSim(0xf,D,B,I,S)
#define JGSm(D,B,I,S)			JCCSim(0xf,D,B,I,S)

#define JCCim(CC,D,B,I,S)		((_r0P(B) && _r0P(I)) ? _OO_D4	(0x0f80|(CC)		,(int)(D)		) : \
							        ASMFAIL("illegal mode in conditional jump"))

#define JOm(D,B,I,S)			JCCim(0x0,D,B,I,S)
#define JNOm(D,B,I,S)			JCCim(0x1,D,B,I,S)
#define JBm(D,B,I,S)			JCCim(0x2,D,B,I,S)
#define JNAEm(D,B,I,S)			JCCim(0x2,D,B,I,S)
#define JNBm(D,B,I,S)			JCCim(0x3,D,B,I,S)
#define JAEm(D,B,I,S)			JCCim(0x3,D,B,I,S)
#define JEm(D,B,I,S)			JCCim(0x4,D,B,I,S)
#define JZm(D,B,I,S)			JCCim(0x4,D,B,I,S)
#define JNEm(D,B,I,S)			JCCim(0x5,D,B,I,S)
#define JNZm(D,B,I,S)			JCCim(0x5,D,B,I,S)
#define JBEm(D,B,I,S)			JCCim(0x6,D,B,I,S)
#define JNAm(D,B,I,S)			JCCim(0x6,D,B,I,S)
#define JNBEm(D,B,I,S)			JCCim(0x7,D,B,I,S)
#define JAm(D,B,I,S)			JCCim(0x7,D,B,I,S)
#define JSm(D,B,I,S)			JCCim(0x8,D,B,I,S)
#define JNSm(D,B,I,S)			JCCim(0x9,D,B,I,S)
#define JPm(D,B,I,S)			JCCim(0xa,D,B,I,S)
#define JPEm(D,B,I,S)			JCCim(0xa,D,B,I,S)
#define JNPm(D,B,I,S)			JCCim(0xb,D,B,I,S)
#define JPOm(D,B,I,S)			JCCim(0xb,D,B,I,S)
#define JLm(D,B,I,S)			JCCim(0xc,D,B,I,S)
#define JNGEm(D,B,I,S)			JCCim(0xc,D,B,I,S)
#define JNLm(D,B,I,S)			JCCim(0xd,D,B,I,S)
#define JGEm(D,B,I,S)			JCCim(0xd,D,B,I,S)
#define JLEm(D,B,I,S)			JCCim(0xe,D,B,I,S)
#define JNGm(D,B,I,S)			JCCim(0xe,D,B,I,S)
#define JNLEm(D,B,I,S)			JCCim(0xf,D,B,I,S)
#define JGm(D,B,I,S)			JCCim(0xf,D,B,I,S)


#define JMPSm(D,B,I,S)			((_r0P(B) && _r0P(I)) ? _O_D1	(0xeb			,(int)(D)		) : \
							        ASMFAIL("illegal mode in short jump"))

#define JMPm(D,B,I,S)			((_r0P(B) && _r0P(I)) ? _O_D4	(0xe9			,(int)(D)		) : \
					           ((_r0P(I)) ? _O_Mrm	(0xff	,_b11,_b100,_r4(B)			) : \
							        _O_r_X	(0xff	     ,_b100	,(int)(D),B,I,S		) ))


#define LAHF()				_O		(0x9f								)
#define LEALmr(MD, MB, MI, MS, RD)	_O_r_X		(0x8d		     ,_r4(RD)		,MD,MB,MI,MS		)
#define LEAVE()				_O		(0xc9								)

	
#define LMSWr(RS)			_OO_Mrm		(0x0f01		,_b11,_b110,_r4(RS)				)
#define LMSWm(MD,MB,MI,MS)		_OO_r_X		(0x0f01		     ,_b110		,MD,MB,MI,MS		)

#define LOOPm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe2			,MD			) : \
								  ASMFAIL("illegal mode in loop"))

#define LOOPEm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe1			,MD			) : \
								  ASMFAIL("illegal mode in loope"))

#define LOOPZm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe1			,MD			) : \
								  ASMFAIL("illegal mode in loopz"))

#define LOOPNEm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe0			,MD			) : \
								  ASMFAIL("illegal mode in loopne"))

#define LOOPNZm(MD,MB,MI,MS)		((_r0P(MB) && _r0P(MI)) ? _O_D1	(0xe0			,MD			) : \
								  ASMFAIL("illegal mode in loopnz"))


#define MOVBrr(RS, RD)			_O_Mrm		(0x80		,_b11,_r1(RS),_r1(RD)				)
#define MOVBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x8a		     ,_r1(RD)		,MD,MB,MI,MS		)
#define MOVBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x88		     ,_r1(RS)		,MD,MB,MI,MS		)
#define MOVBir(IM,  R)			_Or_B		(0xb0,_r1(R)						,_s1(IM))
#define MOVBim(IM, MD, MB, MI, MS)	_O_X_B		(0xc6					,MD,MB,MI,MS	,_s1(IM))

#define MOVSB()				_O		(0xa4								)
#define MOVSL()				_O		(0xa5								)

#define MOVWrr(RS, RD)			_wO_Mrm		(0x89		,_b11,_r2(RS),_r2(RD)				)
#define MOVWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x8b		     ,_r2(RD)		,MD,MB,MI,MS		)
#define MOVWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x89		     ,_r2(RS)		,MD,MB,MI,MS		)
#define MOVWir(IM,  R)			_wOr_W		(0xb8,_r2(R)						,_s2(IM))
#define MOVWim(IM, MD, MB, MI, MS)	_wO_X_W		(0xc7					,MD,MB,MI,MS	,_s2(IM))

#define MOVLrr(RS, RD)			_O_Mrm		(0x89		,_b11,_r4(RS),_r4(RD)				)
#define MOVLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x8b		     ,_r4(RD)		,MD,MB,MI,MS		)
#define MOVLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x89		     ,_r4(RS)		,MD,MB,MI,MS		)
#define MOVLir(IM,  R)			_Or_L		(0xb8,_r4(R)						,IM	)
#define MOVLim(IM, MD, MB, MI, MS)	_O_X_L		(0xc7					,MD,MB,MI,MS	,IM	)


#define MULBrr(RS,RD)		( _rAL (RD) +	_O_Mrm	(0xf6		,_b11,_b100  ,_r1(RS)				) )
#define MULBmr(MD,MB,MI,MS,RD)	( _rAL (RD) +	_O_r_X	(0xf6		     ,_b100		,MD,MB,MI,MS		) )

#define MULWrr(RS,RD)		( _rAX (RD) +	_wO_Mrm	(0xf7		,_b11,_b100  ,_r2(RS)				) )
#define MULWmr(MD,MB,MI,MS,RD)	( _rAX (RD) +	_wO_r_X	(0xf7		     ,_b100		,MD,MB,MI,MS		) )

#define MULLrr(RS,RD)		( _rEAX(RD) +	_O_Mrm	(0xf7		,_b11,_b100  ,_r4(RS)				) )
#define MULLmr(MD,MB,MI,MS,RD)	( _rEAX(RD) +	_O_r_X	(0xf7		     ,_b100		,MD,MB,MI,MS		) )


#define NEGBr(RD)			_O_Mrm		(0xf6		,_b11,_b011  ,_r1(RD)				)
#define NEGBm(MD,MB,MI,MS)		_O_r_X		(0xf6		     ,_b011		,MD,MB,MI,MS		)

#define NEGWr(RD)			_wO_Mrm		(0xf7		,_b11,_b011  ,_r2(RD)				)
#define NEGWm(MD,MB,MI,MS)		_wO_r_X		(0xf7		     ,_b011		,MD,MB,MI,MS		)

#define NEGLr(RD)			_O_Mrm		(0xf7		,_b11,_b011  ,_r4(RD)				)
#define NEGLm(MD,MB,MI,MS)		_O_r_X		(0xf7		     ,_b011		,MD,MB,MI,MS		)


#define NOP()				_O		(0x90								)


#define NOTBr(RD)			_O_Mrm		(0xf6		,_b11,_b010  ,_r1(RD)				)
#define NOTBm(MD,MB,MI,MS)		_O_r_X		(0xf6		     ,_b010		,MD,MB,MI,MS		)

#define NOTWr(RD)			_wO_Mrm		(0xf7		,_b11,_b010  ,_r2(RD)				)
#define NOTWm(MD,MB,MI,MS)		_wO_r_X		(0xf7		     ,_b010		,MD,MB,MI,MS		)

#define NOTLr(RD)			_O_Mrm		(0xf7		,_b11,_b010  ,_r4(RD)				)
#define NOTLm(MD,MB,MI,MS)		_O_r_X		(0xf7		     ,_b010		,MD,MB,MI,MS		)


#define ORBrr(RS, RD)			_O_Mrm		(0x08		,_b11,_r1(RS),_r1(RD)				)
#define ORBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x0a		     ,_r1(RD)		,MD,MB,MI,MS		)
#define ORBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x08		     ,_r1(RS)		,MD,MB,MI,MS		)
#define ORBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b001  ,_r1(RD)			,_s1(IM))
#define ORBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b001		,MD,MB,MI,MS	,_s1(IM))
	
#define ORWrr(RS, RD)			_wO_Mrm		(0x09		,_b11,_r2(RS),_r2(RD)				)
#define ORWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x0b		     ,_r2(RD)		,MD,MB,MI,MS		)
#define ORWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x09		     ,_r2(RS)		,MD,MB,MI,MS		)
#define ORWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b001  ,_r2(RD)			,_s2(IM))
#define ORWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b001		,MD,MB,MI,MS	,_s2(IM))
	
#define ORLrr(RS, RD)			_O_Mrm		(0x09		,_b11,_r4(RS),_r4(RD)				)
#define ORLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x0b		     ,_r4(RD)		,MD,MB,MI,MS		)
#define ORLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x09		     ,_r4(RS)		,MD,MB,MI,MS		)
#define ORLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b001  ,_r4(RD)			,IM	)
#define ORLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b001		,MD,MB,MI,MS	,IM	)


#define POPWr(RD)			_wOr		(0x58,_r2(RD)							)
#define POPWm(MD,MB,MI,MS)		_wO_r_X		(0x8f		     ,_b000		,MD,MB,MI,MS		)

#define POPLr(RD)			_Or		(0x58,_r4(RD)							)
#define POPLm(MD,MB,MI,MS)		_O_r_X		(0x8f		     ,_b000		,MD,MB,MI,MS		)


#define POPA()				_wO		(0x61								)
#define POPAD()				_O		(0x61								)

#define POPF()				_wO		(0x9d								)
#define POPFD()				_O		(0x9d								)


#define PUSHWr(R)			_wOr		(0x50,_r2(R)							)
#define PUSHWm(MD,MB,MI,MS)		_wO_r_X		(0xff,		     ,_b110		,MD,MB,MI,MS		)
#define PUSHWi(IM)			_wOs_sW		(0x68							,IM	)

#define PUSHLr(R)			_Or		(0x50,_r4(R)							)
#define PUSHLm(MD,MB,MI,MS)		_O_r_X		(0xff		     ,_b110		,MD,MB,MI,MS		)
#define PUSHLi(IM)			_Os_sL		(0x68							,IM	)


#define PUSHA()				_wO		(0x60								)
#define PUSHAD()			_O		(0x60								)

#define PUSHF()				_O		(0x9c								)
#define PUSHFD()			_wO		(0x9c								)

#define REP()				_O		(0xf2								)
#define RET()				_O		(0xc3								)
#define RETi(IM)			_O_W		(0xc2							,_s2(IM))


#define ROLBir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd0	,_b11,_b000,_r1(RD)				) : \
						_O_Mrm_B	(0xc0	,_b11,_b000,_r1(RD)			,_u1(IM) ) )
#define ROLBim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd0	     ,_b000		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc0	     ,_b000		,MD,MB,MI,MS	,_u1(IM) ) )
#define ROLBrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd2	,_b11,_b000,_r1(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define ROLBrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd2	     ,_b000		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define ROLWir(IM,RD)		(((IM)==1) ?	_wO_Mrm	(0xd1	,_b11,_b000,_r2(RD)				) : \
						_wO_Mrm_B	(0xc1	,_b11,_b000,_r2(RD)			,_u1(IM) ) )
#define ROLWim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_wO_r_X	(0xd1	     ,_b000		,MD,MB,MI,MS		) : \
						_wO_r_X_B	(0xc1	     ,_b000		,MD,MB,MI,MS	,_u1(IM) ) )
#define ROLWrr(RS,RD)		(((RS)==_CL) ?	_wO_Mrm	(0xd3	,_b11,_b000,_r2(RD)				) : \
						ASMFAIL	("source register must be CL"					) )
#define ROLWrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_wO_r_X	(0xd3	     ,_b000		,MD,MB,MI,MS		) : \
						ASMFAIL	("source register must be CL"					) )

#define ROLLir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd1	,_b11,_b000,_r4(RD)				) : \
						_O_Mrm_B	(0xc1	,_b11,_b000,_r4(RD)			,_u1(IM) ) )
#define ROLLim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd1	     ,_b000		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc1	     ,_b000		,MD,MB,MI,MS	,_u1(IM) ) )
#define ROLLrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd3	,_b11,_b000,_r4(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define ROLLrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd3	     ,_b000		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )


#define RORBir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd0	,_b11,_b001,_r1(RD)				) : \
						_O_Mrm_B	(0xc0	,_b11,_b001,_r1(RD)			,_u1(IM) ) )
#define RORBim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd0	     ,_b001		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc0	     ,_b001		,MD,MB,MI,MS	,_u1(IM) ) )
#define RORBrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd2	,_b11,_b001,_r1(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define RORBrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd2	     ,_b001		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define RORWir(IM,RD)		(((IM)==1) ?	_wO_Mrm	(0xd1	,_b11,_b001,_r2(RD)				) : \
						_wO_Mrm_B	(0xc1	,_b11,_b001,_r2(RD)			,_u1(IM) ) )
#define RORWim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_wO_r_X	(0xd1	     ,_b001		,MD,MB,MI,MS		) : \
						_wO_r_X_B	(0xc1	     ,_b001		,MD,MB,MI,MS	,_u1(IM) ) )
#define RORWrr(RS,RD)		(((RS)==_CL) ?	_wO_Mrm	(0xd3	,_b11,_b001,_r2(RD)				) : \
						ASMFAIL	("source register must be CL"					) )
#define RORWrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_wO_r_X	(0xd3	     ,_b001		,MD,MB,MI,MS		) : \
						ASMFAIL	("source register must be CL"					) )

#define RORLir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd1	,_b11,_b001,_r4(RD)				) : \
						_O_Mrm_B	(0xc1	,_b11,_b001,_r4(RD)			,_u1(IM) ) )
#define RORLim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd1	     ,_b001		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc1	     ,_b001		,MD,MB,MI,MS	,_u1(IM) ) )
#define RORLrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd3	,_b11,_b001,_r4(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define RORLrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd3	     ,_b001		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )


#define SAHF()					_O	(0x9e								)


#define SALBir	SHLBir  
#define SALBim	SHLBim  
#define SALBrr	SHLBrr  
#define SALBrm	SHLBrm  
#define SALWir	SHLWir  
#define SALWim	SHLWim  
#define SALWrr	SHLWrr  
#define SALWrm	SHLWrm  
#define SALLir	SHLLir  
#define SALLim	SHLLim  
#define SALLrr	SHLLrr  
#define SALLrm	SHLLrm  


#define SARBir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd0	,_b11,_b111,_r1(RD)				) : \
						_O_Mrm_B	(0xc0	,_b11,_b111,_r1(RD)			,_u1(IM) ) )
#define SARBim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd0	     ,_b111		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc0	     ,_b111		,MD,MB,MI,MS	,_u1(IM) ) )
#define SARBrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd2	,_b11,_b111,_r1(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SARBrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd2	     ,_b111		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define SARWir(IM,RD)		(((IM)==1) ?	_wO_Mrm	(0xd1	,_b11,_b111,_r2(RD)				) : \
						_wO_Mrm_B	(0xc1	,_b11,_b111,_r2(RD)			,_u1(IM) ) )
#define SARWim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_wO_r_X	(0xd1	     ,_b111		,MD,MB,MI,MS		) : \
						_wO_r_X_B	(0xc1	     ,_b111		,MD,MB,MI,MS	,_u1(IM) ) )
#define SARWrr(RS,RD)		(((RS)==_CL) ?	_wO_Mrm	(0xd3	,_b11,_b111,_r2(RD)				) : \
						ASMFAIL	("source register must be CL"					) )
#define SARWrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_wO_r_X	(0xd3	     ,_b111		,MD,MB,MI,MS		) : \
						ASMFAIL	("source register must be CL"					) )

#define SARLir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd1	,_b11,_b111,_r4(RD)				) : \
						_O_Mrm_B	(0xc1	,_b11,_b111,_r4(RD)			,_u1(IM) ) )
#define SARLim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd1	     ,_b111		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc1	     ,_b111		,MD,MB,MI,MS	,_u1(IM) ) )
#define SARLrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd3	,_b11,_b111,_r4(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SARLrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd3	     ,_b111		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )


#define SBBBrr(RS, RD)			_O_Mrm		(0x18		,_b11,_r1(RS),_r1(RD)				)
#define SBBBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x1a		     ,_r1(RD)		,MD,MB,MI,MS		)
#define SBBBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x18		     ,_r1(RS)		,MD,MB,MI,MS		)
#define SBBBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b011  ,_r1(RD)			,_s1(IM))
#define SBBBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b011		,MD,MB,MI,MS	,_s1(IM))
	
#define SBBWrr(RS, RD)			_wO_Mrm	(0x19		,_b11,_r2(RS),_r2(RD)				)
#define SBBWmr(MD, MB, MI, MS, RD)	_wO_r_X	(0x1b		     ,_r2(RD)		,MD,MB,MI,MS		)
#define SBBWrm(RS, MD, MB, MI, MS)	_wO_r_X	(0x19		     ,_r2(RS)		,MD,MB,MI,MS		)
#define SBBWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b011  ,_r2(RD)			,_s2(IM))
#define SBBWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b011		,MD,MB,MI,MS	,_s2(IM))
	
#define SBBLrr(RS, RD)			_O_Mrm		(0x19		,_b11,_r4(RS),_r4(RD)				)
#define SBBLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x1b		     ,_r4(RD)		,MD,MB,MI,MS		)
#define SBBLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x19		     ,_r4(RS)		,MD,MB,MI,MS		)
#define SBBLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b011  ,_r4(RD)			,IM	)
#define SBBLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b011		,MD,MB,MI,MS	,IM	)


#define SETCCir(CC,RD)			_OO_Mrm		(0x0f90|(CC)	,_b11,_b000,_r1(RD)				)

#define SETOr(RD)			SETCCir(0x0,RD)
#define SETNOr(RD)			SETCCir(0x1,RD)
#define SETBr(RD)			SETCCir(0x2,RD)
#define SETNAEr(RD)			SETCCir(0x2,RD)
#define SETNBr(RD)			SETCCir(0x3,RD)
#define SETAEr(RD)			SETCCir(0x3,RD)
#define SETEr(RD)			SETCCir(0x4,RD)
#define SETZr(RD)			SETCCir(0x4,RD)
#define SETNEr(RD)			SETCCir(0x5,RD)
#define SETNZr(RD)			SETCCir(0x5,RD)
#define SETBEr(RD)			SETCCir(0x6,RD)
#define SETNAr(RD)			SETCCir(0x6,RD)
#define SETNBEr(RD)			SETCCir(0x7,RD)
#define SETAr(RD)			SETCCir(0x7,RD)
#define SETSr(RD)			SETCCir(0x8,RD)
#define SETNSr(RD)			SETCCir(0x9,RD)
#define SETPr(RD)			SETCCir(0xa,RD)
#define SETPEr(RD)			SETCCir(0xa,RD)
#define SETNPr(RD)			SETCCir(0xb,RD)
#define SETPOr(RD)			SETCCir(0xb,RD)
#define SETLr(RD)			SETCCir(0xc,RD)
#define SETNGEr(RD)			SETCCir(0xc,RD)
#define SETNLr(RD)			SETCCir(0xd,RD)
#define SETGEr(RD)			SETCCir(0xd,RD)
#define SETLEr(RD)			SETCCir(0xe,RD)
#define SETNGr(RD)			SETCCir(0xe,RD)
#define SETNLEr(RD)			SETCCir(0xf,RD)
#define SETGr(RD)			SETCCir(0xf,RD)

#define SETCCim(CC,MD,MB,MI,MS)		_OO_r_X		(0x0f90|(CC)	     ,_b000		,MD,MB,MI,MS		)

#define SETOm(D,B,I,S)			SETCCim(0x0,D,B,I,S)
#define SETNOm(D,B,I,S)			SETCCim(0x1,D,B,I,S)
#define SETBm(D,B,I,S)			SETCCim(0x2,D,B,I,S)
#define SETNAEm(D,B,I,S)		SETCCim(0x2,D,B,I,S)
#define SETNBm(D,B,I,S)			SETCCim(0x3,D,B,I,S)
#define SETAEm(D,B,I,S)			SETCCim(0x3,D,B,I,S)
#define SETEm(D,B,I,S)			SETCCim(0x4,D,B,I,S)
#define SETZm(D,B,I,S)			SETCCim(0x4,D,B,I,S)
#define SETNEm(D,B,I,S)			SETCCim(0x5,D,B,I,S)
#define SETNZm(D,B,I,S)			SETCCim(0x5,D,B,I,S)
#define SETBEm(D,B,I,S)			SETCCim(0x6,D,B,I,S)
#define SETNAm(D,B,I,S)			SETCCim(0x6,D,B,I,S)
#define SETNBEm(D,B,I,S)		SETCCim(0x7,D,B,I,S)
#define SETAm(D,B,I,S)			SETCCim(0x7,D,B,I,S)
#define SETSm(D,B,I,S)			SETCCim(0x8,D,B,I,S)
#define SETNSm(D,B,I,S)			SETCCim(0x9,D,B,I,S)
#define SETPm(D,B,I,S)			SETCCim(0xa,D,B,I,S)
#define SETPEm(D,B,I,S)			SETCCim(0xa,D,B,I,S)
#define SETNPm(D,B,I,S)			SETCCim(0xb,D,B,I,S)
#define SETPOm(D,B,I,S)			SETCCim(0xb,D,B,I,S)
#define SETLm(D,B,I,S)			SETCCim(0xc,D,B,I,S)
#define SETNGEm(D,B,I,S)		SETCCim(0xc,D,B,I,S)
#define SETNLm(D,B,I,S)			SETCCim(0xd,D,B,I,S)
#define SETGEm(D,B,I,S)			SETCCim(0xd,D,B,I,S)
#define SETLEm(D,B,I,S)			SETCCim(0xe,D,B,I,S)
#define SETNGm(D,B,I,S)			SETCCim(0xe,D,B,I,S)
#define SETNLEm(D,B,I,S)		SETCCim(0xf,D,B,I,S)
#define SETGm(D,B,I,S)			SETCCim(0xf,D,B,I,S)


#define SHLBir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd0	,_b11,_b100,_r1(RD)				) : \
						_O_Mrm_B	(0xc0	,_b11,_b100,_r1(RD)			,_u1(IM) ) )
#define SHLBim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd0	     ,_b100		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc0	     ,_b100		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHLBrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd2	,_b11,_b100,_r1(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHLBrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd2	     ,_b100		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define SHLWir(IM,RD)		(((IM)==1) ?	_wO_Mrm		(0xd1	,_b11,_b100,_r2(RD)				) : \
						_wO_Mrm_B	(0xc1	,_b11,_b100,_r2(RD)			,_u1(IM) ) )
#define SHLWim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_wO_r_X		(0xd1	     ,_b100		,MD,MB,MI,MS		) : \
						_wO_r_X_B	(0xc1	     ,_b100		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHLWrr(RS,RD)		(((RS)==_CL) ?	_wO_Mrm		(0xd3	,_b11,_b100,_r2(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHLWrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_wO_r_X		(0xd3	     ,_b100		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"					) )

#define SHLLir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd1	,_b11,_b100,_r4(RD)				) : \
						_O_Mrm_B	(0xc1	,_b11,_b100,_r4(RD)			,_u1(IM) ) )
#define SHLLim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd1	     ,_b100		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc1	     ,_b100		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHLLrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd3	,_b11,_b100,_r4(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHLLrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd3	     ,_b100		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )


#define SHRBir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd0	,_b11,_b101,_r1(RD)				) : \
						_O_Mrm_B	(0xc0	,_b11,_b101,_r1(RD)			,_u1(IM) ) )
#define SHRBim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd0	     ,_b101		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc0	     ,_b101		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHRBrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd2	,_b11,_b101,_r1(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHRBrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd2	     ,_b101		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define SHRWir(IM,RD)		(((IM)==1) ?	_wO_Mrm		(0xd1	,_b11,_b101,_r2(RD)				) : \
						_wO_Mrm_B	(0xc1	,_b11,_b101,_r2(RD)			,_u1(IM) ) )
#define SHRWim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_wO_r_X		(0xd1	     ,_b101		,MD,MB,MI,MS		) : \
						_wO_r_X_B	(0xc1	     ,_b101		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHRWrr(RS,RD)		(((RS)==_CL) ?	_wO_Mrm		(0xd3	,_b11,_b101,_r2(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHRWrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_wO_r_X		(0xd3	     ,_b101		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )

#define SHRLir(IM,RD)		(((IM)==1) ?	_O_Mrm		(0xd1	,_b11,_b101,_r4(RD)				) : \
						_O_Mrm_B	(0xc1	,_b11,_b101,_r4(RD)			,_u1(IM) ) )
#define SHRLim(IM,MD,MB,MS,MI)	(((IM)==1) ?	_O_r_X		(0xd1	     ,_b101		,MD,MB,MI,MS		) : \
						_O_r_X_B	(0xc1	     ,_b101		,MD,MB,MI,MS	,_u1(IM) ) )
#define SHRLrr(RS,RD)		(((RS)==_CL) ?	_O_Mrm		(0xd3	,_b11,_b101,_r4(RD)				) : \
						ASMFAIL		("source register must be CL"				) )
#define SHRLrm(RS,MD,MB,MS,MI)	(((RS)==_CL) ?	_O_r_X		(0xd3	     ,_b101		,MD,MB,MI,MS		) : \
						ASMFAIL		("source register must be CL"				) )


#define STC()				_O		(0xf9								)
#define STD()				_O		(0xfd								)

#define SUBBrr(RS, RD)			_O_Mrm		(0x28		,_b11,_r1(RS),_r1(RD)				)
#define SUBBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x2a		     ,_r1(RD)		,MD,MB,MI,MS		)
#define SUBBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x28		     ,_r1(RS)		,MD,MB,MI,MS		)
#define SUBBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b101  ,_r1(RD)			,_s1(IM))
#define SUBBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b101		,MD,MB,MI,MS	,_s1(IM))
	
#define SUBWrr(RS, RD)			_wO_Mrm		(0x29		,_b11,_r2(RS),_r2(RD)				)
#define SUBWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x2b		     ,_r2(RD)		,MD,MB,MI,MS		)
#define SUBWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x29		     ,_r2(RS)		,MD,MB,MI,MS		)
#define SUBWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b101  ,_r2(RD)			,_s2(IM))
#define SUBWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b101		,MD,MB,MI,MS	,_s2(IM))
	
#define SUBLrr(RS, RD)			_O_Mrm		(0x29		,_b11,_r4(RS),_r4(RD)				)
#define SUBLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x2b		     ,_r4(RD)		,MD,MB,MI,MS		)
#define SUBLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x29		     ,_r4(RS)		,MD,MB,MI,MS		)
#define SUBLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b101  ,_r4(RD)			,IM	)
#define SUBLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b101		,MD,MB,MI,MS	,IM	)


#define TESTBrr(RS, RD)			_O_Mrm		(0x84		,_b11,_r1(RS),_r1(RD)				)
#define TESTBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x84		     ,_r1(RS)		,MD,MB,MI,MS		)
#define TESTBir(IM, RD)			_O_Mrm_B	(0xf6		,_b11,_b000  ,_r1(RD)			,_u1(IM))
#define TESTBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0xf6		     ,_b000		,MD,MB,MI,MS	,_u1(IM))

#define TESTWrr(RS, RD)			_wO_Mrm		(0x85		,_b11,_r2(RS),_r2(RD)				)
#define TESTWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x85		     ,_r2(RS)		,MD,MB,MI,MS		)
#define TESTWir(IM, RD)			_wO_Mrm_W	(0xf7		,_b11,_b000  ,_r2(RD)			,_u2(IM))
#define TESTWim(IM, MD, MB, MI, MS)	_wO_r_X_W	(0xf7		     ,_b000		,MD,MB,MI,MS	,_u2(IM))

#define TESTLrr(RS, RD)			_O_Mrm		(0x85		,_b11,_r4(RS),_r4(RD)				)
#define TESTLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x85		     ,_r4(RS)		,MD,MB,MI,MS		)
#define TESTLir(IM, RD)			_O_Mrm_L	(0xf7		,_b11,_b000  ,_r4(RD)			,IM	)
#define TESTLim(IM, MD, MB, MI, MS)	_O_r_X_L	(0xf7		     ,_b000		,MD,MB,MI,MS	,IM	)


#define XADDBrr(RS,RD)			_OO_Mrm		(0x0fc0		,_b11,_r1(RS),_r1(RD)				)
#define XADDBrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fc0		     ,_r1(RS)		,MD,MB,MI,MS		)

#define XADDWrr(RS,RD)			_wOO_Mrm	(0x0fc1		,_b11,_r2(RS),_r2(RD)				)
#define XADDWrm(RS,MD,MB,MI,MS)		_wOO_r_X	(0x0fc1		     ,_r2(RS)		,MD,MB,MI,MS		)

#define XADDLrr(RS,RD)			_OO_Mrm		(0x0fc1		,_b11,_r4(RS),_r4(RD)				)
#define XADDLrm(RS,MD,MB,MI,MS)		_OO_r_X		(0x0fc1		     ,_r4(RS)		,MD,MB,MI,MS		)


#define XCHGBrr(RS,RD)			_O_Mrm		(0x86		,_b11,_r1(RS),_r1(RD)				)
#define XCHGBrm(RS,MD,MB,MI,MS)		_O_r_X		(0x86		     ,_r1(RS)		,MD,MB,MI,MS		)

#define XCHGWrr(RS,RD)			_wO_Mrm		(0x87		,_b11,_r2(RS),_r2(RD)				)
#define XCHGWrm(RS,MD,MB,MI,MS)		_wO_r_X		(0x87		     ,_r2(RS)		,MD,MB,MI,MS		)

#define XCHGLrr(RS,RD)			_O_Mrm		(0x87		,_b11,_r4(RS),_r4(RD)				)
#define XCHGLrm(RS,MD,MB,MI,MS)		_O_r_X		(0x87		     ,_r4(RS)		,MD,MB,MI,MS		)


#define XORBrr(RS, RD)			_O_Mrm		(0x30		,_b11,_r1(RS),_r1(RD)				)
#define XORBmr(MD, MB, MI, MS, RD)	_O_r_X		(0x32		     ,_r1(RD)		,MD,MB,MI,MS		)
#define XORBrm(RS, MD, MB, MI, MS)	_O_r_X		(0x30		     ,_r1(RS)		,MD,MB,MI,MS		)
#define XORBir(IM, RD)			_O_Mrm_B	(0x80		,_b11,_b110  ,_r1(RD)			,_s1(IM))
#define XORBim(IM, MD, MB, MI, MS)	_O_r_X_B	(0x80		     ,_b110		,MD,MB,MI,MS	,_s1(IM))

#define XORWrr(RS, RD)			_wO_Mrm		(0x31		,_b11,_r2(RS),_r2(RD)				)
#define XORWmr(MD, MB, MI, MS, RD)	_wO_r_X		(0x33		     ,_r2(RD)		,MD,MB,MI,MS		)
#define XORWrm(RS, MD, MB, MI, MS)	_wO_r_X		(0x31		     ,_r2(RS)		,MD,MB,MI,MS		)
#define XORWir(IM, RD)			_wOs_Mrm_sW	(0x81		,_b11,_b110  ,_r2(RD)			,_s2(IM))
#define XORWim(IM, MD, MB, MI, MS)	_wOs_r_X_sW	(0x81		     ,_b110		,MD,MB,MI,MS	,_s2(IM))

#define XORLrr(RS, RD)			_O_Mrm		(0x31		,_b11,_r4(RS),_r4(RD)				)
#define XORLmr(MD, MB, MI, MS, RD)	_O_r_X		(0x33		     ,_r4(RD)		,MD,MB,MI,MS		)
#define XORLrm(RS, MD, MB, MI, MS)	_O_r_X		(0x31		     ,_r4(RS)		,MD,MB,MI,MS		)
#define XORLir(IM, RD)			_Os_Mrm_sL	(0x81		,_b11,_b110  ,_r4(RD)			,IM	)
#define XORLim(IM, MD, MB, MI, MS)	_Os_r_X_sL	(0x81		     ,_b110		,MD,MB,MI,MS	,IM	)


/*** unqualified/implicitly-sized instructions ***/


#define BSWAPr(R)			BSWAPLr(R)
#define PUSHr(R)			((_r4P(R) ? PUSHLr(R) : _r2P(R) ? PUSHWr(R) : ASMFAIL("illegal register size")))
#define POPr(R)				((_r4P(R) ? POPLr(R) : _r2P(R) ? POPWr(R) : ASMFAIL("illegal register size")))

/* lots of others that I can't be bothered to define */


/* +++ pseudo-ops */

#define _ALIGNm(D,X,Y,Z)		(__ALIGN(((D) - (_UL(asm_pc) + (D) - 1) / (D)) * (D)), ((_UL(asm_pc) % (D)) == 0 ? 0 : ASMFAIL("internal error: .align")))



/*** References:										*/
/*												*/
/* [1] "Intel Architecture Software Developer's Manual Volume 1: Basic Architecture",		*/
/*     Intel Corporation 1997.									*/
/*												*/
/* [2] "Intel Architecture Software Developer's Manual Volume 2: Instruction Set Reference",	*/
/*     Intel Corporation 1997.									*/

#endif /* __ccg_asm_i386_h */
