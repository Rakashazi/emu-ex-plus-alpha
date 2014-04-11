#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#ifndef _GENERATOR_H_
#define _GENERATOR_H_


/* VERSION set by autoconf */
/* PACKAGE set by autoconf */

//#include "SDL_types.h"
#include <imagine/util/ansiTypes.h>
#include <imagine/logger/logger.h>
/*#define uint8     Uint8
#define uint16    Uint16
#define uint32    Uint32
#define sint8     Sint8
#define sint16    Sint16
#define sint32    Sint32*/

#define GEN_RAMLENGTH 64*1024

#define LEN_IPCLISTTABLE 16*1024

//#define GENERATOR_JIT

/*
 * LOCENDIANxx takes data that came from a big endian source and converts it
 * into the local endian.  On a big endian machine the data will already be
 * loaded correctly, however on a little endian machine the processor will
 * have loaded the data assuming little endian data, so we need to swap the
 * byte ordering.
 *
 * LOCENDIANxxL takes data that came from a little endian source and
 * converts it into the local endian.  This means that on a little endian
 * machine the data will already be loaded correctly, however on a big
 * endian machine the processor will have loaded the data assuming big endian
 * data, so we need to swap the byte ordering.
 *
 * Both LOCENDIANxx and LOCENDIANxxL can be used in reverse - i.e. when
 * you have data in local endian that you need to write in big (LOCENDIANxx)
 * or little (LOCENDIANxxL) endian.
 *
 */
#ifdef WORDS_BIGENDIAN
#define LOCENDIAN16(y) (y)
#define LOCENDIAN32(y) (y)
#define BYTES_HIGHFIRST
#else
#define LOCENDIAN16(y) ((((y)>>8)&0x00FF)+((((y)<<8)&0xFF00)))
#define LOCENDIAN32(y) ( (((y)>>24) & 0x000000FF) + \
                         (((y)>>8)  & 0x0000FF00) + \
                         (((y)<<8)  & 0x00FF0000) + \
                         (((y)<<24) & 0xFF000000) )
#endif

typedef enum {
  tp_src, tp_dst
} t_type;

typedef enum {
  sz_none, sz_byte, sz_word, sz_long
} t_size;

typedef enum {
  dt_Dreg, dt_Areg, dt_Aind, dt_Ainc, dt_Adec, dt_Adis, dt_Aidx,
  dt_AbsW, dt_AbsL, dt_Pdis, dt_Pidx,
  dt_ImmB, dt_ImmW, dt_ImmL, dt_ImmS,
  dt_Imm3, dt_Imm4, dt_Imm8, dt_Imm8s, dt_ImmV,
  dt_Ill
} t_datatype;

typedef enum {
  ea_Dreg, ea_Areg, ea_Aind, ea_Ainc, ea_Adec, ea_Adis, ea_Aidx,
  ea_AbsW, ea_AbsL, ea_Pdis, ea_Pidx, ea_Imm
} t_eatypes;

typedef enum {
  i_ILLG,
  i_OR, i_ORSR,
  i_AND, i_ANDSR,
  i_EOR, i_EORSR,
  i_SUB, i_SUBA, i_SUBX,
  i_ADD, i_ADDA, i_ADDX,
  i_MULU, i_MULS,
  i_CMP, i_CMPA,
  i_BTST, i_BCHG, i_BCLR, i_BSET,
  i_MOVE, i_MOVEA,
  i_MOVEPMR, i_MOVEPRM,
  i_MOVEFSR, i_MOVETSR,
  i_MOVEMRM, i_MOVEMMR,
  i_MOVETUSP, i_MOVEFUSP,
  i_NEG, i_NEGX, i_CLR, i_NOT,
  i_ABCD, i_SBCD, i_NBCD,
  i_SWAP,
  i_PEA, i_LEA,
  i_EXT, i_EXG,
  i_TST, i_TAS, i_CHK,
  i_TRAPV, i_TRAP, i_RESET, i_NOP, i_STOP,
  i_LINK, i_UNLK,
  i_RTE, i_RTS, i_RTR,
  i_JSR, i_JMP, i_Scc, i_SF, i_DBcc, i_DBRA, i_Bcc, i_BSR,
  i_DIVU, i_DIVS,
  i_ASR, i_LSR, i_ROXR, i_ROR,
  i_ASL, i_LSL, i_ROXL, i_ROL,
  i_LINE10, i_LINE15
} t_mnemonic;

typedef struct {
  uint16 mask;              /* mask of bits that are static */
  uint16 bits;              /* bit values corresponding to bits in mask */
  t_mnemonic mnemonic;      /* instruction mnemonic */
  struct {
    int priv:1;             /* instruction is privileged if set */
    int endblk:1;           /* instruction ends a block if set */
    int imm_notzero:1;      /* immediate data cannot be 0 (if applicable) */
    int used:5;             /* bitmap of XNZVC flags inspected */
    int set:5;              /* bitmap of XNZVC flags altered */
  } flags;
  t_size size;              /* size of instruction */
  t_datatype stype;         /* type of source */
  t_datatype dtype;         /* type of destination */
  unsigned int sbitpos:4;   /* bit pos of imm data or reg part of EA */
  unsigned int dbitpos:4;   /* reg part of EA */
  unsigned int immvalue;    /* if stype is ImmS this is the value */
  unsigned int cc;          /* condition code if mnemonic is Scc/Dbcc/Bcc */
  unsigned int funcnum;     /* function number for this instruction */
  unsigned int wordlen;     /* length in words of this instruction */
  unsigned int clocks;      /* number of external clock periods */
} t_iib;                    /* instruction information block */

#define IIB_FLAG_X 1<<0
#define IIB_FLAG_N 1<<1
#define IIB_FLAG_Z 1<<2
#define IIB_FLAG_V 1<<3
#define IIB_FLAG_C 1<<4

typedef struct {
  t_mnemonic mnemonic;
  const char *name;
} t_mnemonic_table;

extern t_mnemonic_table mnemonic_table[];

extern const char *condition_table[];

typedef union {
  struct {
#ifndef BYTES_HIGHFIRST
    unsigned int c:1;
    unsigned int v:1;
    unsigned int z:1;
    unsigned int n:1;
    unsigned int x:1;
    unsigned int :3;
    unsigned int i0:1;
    unsigned int i1:1;
    unsigned int i2:1;
    unsigned int :2;
    unsigned int s:1;
    unsigned int :1;
    unsigned int t:1;
#else
    unsigned int t:1;
    unsigned int :1;
    unsigned int s:1;
    unsigned int :2;
    unsigned int i2:1;
    unsigned int i1:1;
    unsigned int i0:1;
    unsigned int :3;
    unsigned int x:1;
    unsigned int n:1;
    unsigned int z:1;
    unsigned int v:1;
    unsigned int c:1;
#endif
  } sr_struct;
  uint16 sr_int;
} t_sr;

typedef struct {
  uint32 pc;
  uint32 sp;
  t_sr sr;
  uint16 stop;
  uint32 regs[16];
  uint16 pending;
} t_regs;

#define SR_CFLAG (1<<0)
#define SR_VFLAG (1<<1)
#define SR_ZFLAG (1<<2)
#define SR_NFLAG (1<<3)
#define SR_XFLAG (1<<4)
#define SR_SFLAG (1<<13)
#define SR_TFLAG (1<<15)

/* Steve Snake / Charles MacDonald (msgid <3C427237.3CEC@value.net>) -
 * TAS on Genesis 1 and 2 (but not 3) do not write back with TAS */
//#define BROKEN_TAS

#ifndef GENERATOR_LOGGING
#  define LOG_DEBUG3(x)   /* */
#  define LOG_DEBUG2(x)   /* */
#  define LOG_DEBUG1(x)   /* */
#  define LOG_USER(x)     /* */
#  define LOG_VERBOSE(x)  /* */
#  define LOG_NORMAL(x)   /* */
#  define LOG_CRITICAL(x) /* */
#  define LOG_REQUEST(x)  /* */
#else
#  define LOG_DEBUG3(x)   /* ui_log_debug3 ## x */
#  define LOG_DEBUG2(x)   /* ui_log_debug2 ## x */
#  define LOG_DEBUG1(x)   /* ui_log_debug1 ## x */
#  define LOG_USER(x)     ui_log_user ## x
#  define LOG_VERBOSE(x)  ui_log_verbose ## x
#  define LOG_NORMAL(x)   ui_log_normal ## x
#  define LOG_CRITICAL(x) ui_log_critical ## x
#  define LOG_REQUEST(x)  ui_log_request ## x
#endif

extern const char *condition_table[];
extern t_mnemonic_table mnemonic_table[];

#endif
