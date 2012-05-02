#ifndef __YMDELTAT_H_
#define __YMDELTAT_H_


#include "MsxTypes.h"

#define YM_DELTAT_SHIFT    (16)

#ifndef OSD_CPU_H
#define OSD_CPU_H
typedef unsigned char	UINT8;   /* unsigned  8bit */
typedef unsigned short	UINT16;  /* unsigned 16bit */
typedef unsigned int	UINT32;  /* unsigned 32bit */
typedef signed char	INT8;    /* signed  8bit   */
typedef signed short	INT16;   /* signed 16bit   */
typedef signed int	INT32;   /* signed 32bit   */
#endif

/* adpcm type A and type B struct */
typedef struct deltat_adpcm_state {
	UINT8 *memory;
	UINT32 memory_size;
    void* OPL;
    DoubleT freqbase;
	INT32 *output_pointer; /* pointer of output pointers */
	int output_range;

	UINT8 reg[16];
	UINT8 portstate,portcontrol;
	int portshift;
	int memread; /* first two bytes of mem->cpu transfer are dummy */

	UINT8 flag;          /* port state         */
	UINT8 eos;           /* end of sample flag */
	UINT8 flagMask;      /* arrived flag mask  */
	UINT8 now_data;
	UINT32 now_addr;
	UINT32 now_step;
	UINT32 step;
	UINT32 start;
	UINT32 end;
	UINT32 read_pointer;  /* used for memory -> sample bank */
	UINT32 write_pointer; /* used for memory -> sample bank */
	UINT32 delta;
	INT32 volume;
	INT32 *pan;        /* &output_pointer[pan] */
	INT32 /*adpcmm,*/ adpcmx, adpcmd;
	INT32 adpcml;			/* hiro-shi!! */

	/* leveling and re-sampling state for DELTA-T */
	INT32 volume_w_step;   /* volume with step rate */
	INT32 next_leveling;   /* leveling value        */
	INT32 sample_step;     /* step of re-sampling   */

	UINT8 arrivedFlag;    /* flag of arrived end address */
}YM_DELTAT;

/* static state */
extern UINT8 *ym_deltat_memory;       /* memory pointer */

/* before YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT); */
#define YM_DELTAT_DECODE_PRESET(DELTAT) {ym_deltat_memory = DELTAT->memory;}

void YM_DELTAT_ADPCM_LoadState(YM_DELTAT *DELTAT);
void YM_DELTAT_ADPCM_SaveState(YM_DELTAT *DELTAT);

UINT8 YM_DELTAT_ADPCM_Peek(YM_DELTAT *DELTAT);
UINT8 YM_DELTAT_ADPCM_Peek2(YM_DELTAT *DELTAT);
UINT8 YM_DELTAT_ADPCM_Read(YM_DELTAT *DELTAT);
UINT8 YM_DELTAT_ADPCM_Read2(YM_DELTAT *DELTAT);
void  YM_DELTAT_ADPCM_Write(YM_DELTAT *DELTAT,int r,int v);
void  YM_DELTAT_ADPCM_Reset(YM_DELTAT *DELTAT,int pan);

#endif

void YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT);

/* DELTA-T particle adjuster */
#define YM_DELTAT_DELTA_MAX (24576)
#define YM_DELTAT_DELTA_MIN (127)
#define YM_DELTAT_DELTA_DEF (127)

#define YM_DELTAT_DECODE_RANGE 32768
#define YM_DELTAT_DECODE_MIN (-(YM_DELTAT_DECODE_RANGE))
#define YM_DELTAT_DECODE_MAX ((YM_DELTAT_DECODE_RANGE)-1)

extern const INT32 ym_deltat_decode_tableB1[];
extern const INT32 ym_deltat_decode_tableB2[];

#define YM_DELTAT_Limit(val,max,min)	\
{										\
	if ( val > max ) val = max;			\
	else if ( val < min ) val = min;	\
}
