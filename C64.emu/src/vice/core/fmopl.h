#ifndef VICE_FMOPL_H
#define VICE_FMOPL_H

/* select output bits size of output : 8 or 16 */
#define OPL_SAMPLE_BITS 16

/* compiler dependence */
typedef unsigned char UINT8;     /* unsigned  8bit */
typedef unsigned short UINT16;   /* unsigned 16bit */
typedef unsigned int UINT32;     /* unsigned 32bit */
typedef signed char INT8;        /* signed  8bit   */
typedef signed short INT16;      /* signed 16bit   */
typedef signed int INT32;        /* signed 32bit   */

typedef INT16 OPLSAMPLE;

typedef struct {
    UINT32 ar;          /* attack rate: AR<<2           */
    UINT32 dr;          /* decay rate:  DR<<2           */
    UINT32 rr;          /* release rate:RR<<2           */
    UINT8 KSR;          /* key scale rate               */
    UINT8 ksl;          /* keyscale level               */
    UINT8 ksr;          /* key scale rate: kcode>>KSR   */
    UINT8 mul;          /* multiple: mul_tab[ML]        */

    /* Phase Generator */
    UINT32 Cnt;         /* frequency counter            */
    UINT32 Incr;                /* frequency counter step       */
    UINT8 FB;           /* feedback shift value         */
    INT32 *connect1;    /* slot1 output pointer         */
    INT32 op1_out[2];   /* slot1 output for feedback    */
    UINT8 CON;          /* connection (algorithm) type  */

    /* Envelope Generator */
    UINT8 eg_type;      /* percussive/non-percussive mode */
    UINT8 state;                /* phase type                   */
    UINT32 TL;          /* total level: TL << 2         */
    INT32 TLL;          /* adjusted now TL              */
    INT32 volume;               /* envelope counter             */
    UINT32 sl;          /* sustain level: sl_tab[SL]    */
    UINT8 eg_sh_ar;     /* (attack state)               */
    UINT8 eg_sel_ar;    /* (attack state)               */
    UINT8 eg_sh_dr;     /* (decay state)                */
    UINT8 eg_sel_dr;    /* (decay state)                */
    UINT8 eg_sh_rr;     /* (release state)              */
    UINT8 eg_sel_rr;    /* (release state)              */
    UINT32 key;         /* 0 = KEY OFF, >0 = KEY ON     */

    /* LFO */
    UINT32 AMmask;              /* LFO Amplitude Modulation enable mask */
    UINT8 vib;          /* LFO Phase Modulation enable flag (active high)*/

    /* waveform select */
    UINT16 wavetable;
} OPL_SLOT;

typedef struct {
    OPL_SLOT SLOT[2];
    /* phase generator state */
    UINT32 block_fnum;  /* block+fnum                   */
    UINT32 fc;          /* Freq. Increment base         */
    UINT32 ksl_base;    /* KeyScaleLevel Base step      */
    UINT8 kcode;                /* key code (for key scaling)   */
} OPL_CH;

/* OPL state */
typedef struct fm_opl_f {
    /* FM channel slots */
    OPL_CH P_CH[9];                     /* OPL/OPL2 chips have 9 channels*/

    UINT32 eg_cnt;                      /* global envelope generator counter    */
    UINT32 eg_timer;                    /* global envelope generator counter works at frequency = chipclock/72 */
    UINT32 eg_timer_add;                /* step of eg_timer                     */
    UINT32 eg_timer_overflow;           /* envelope generator timer overlfows every 1 sample (on real chip) */

    UINT8 rhythm;                               /* Rhythm mode                  */

    UINT32 fn_tab[1024];                /* fnumber->increment counter   */

    /* LFO */
    UINT8 lfo_am_depth;
    UINT8 lfo_pm_depth_range;
    UINT32 lfo_am_cnt;
    UINT32 lfo_am_inc;
    UINT32 lfo_pm_cnt;
    UINT32 lfo_pm_inc;

    UINT32 noise_rng;                           /* 23 bit noise shift register  */
    UINT32 noise_p;                             /* current noise 'phase'        */
    UINT32 noise_f;                             /* current noise period         */

    UINT8 wavesel;                              /* waveform select enable flag  */

    UINT32 T[2];                                        /* timer counters               */
    UINT8 st[2];                                        /* timer enable                 */

    UINT8 type;                                 /* chip type                    */
    UINT8 address;                              /* address register             */
    UINT8 status;                                       /* status flag                  */
    UINT8 statusmask;                           /* status mask                  */
    UINT8 mode;                                 /* Reg.08 : CSM,notesel,etc.    */

    UINT32 clock;                                       /* master clock  (Hz)           */
    UINT32 rate;                                        /* sampling rate (Hz)           */
    double freqbase;                            /* frequency base               */
} FM_OPL;

/*
 * Initialize YM3812 emulator.
 *
 * 'num' is the number of virtual YM3526's to allocate
 * 'clock' is the chip clock in Hz
 * 'rate' is sampling rate
 */
extern FM_OPL *ym3812_init(UINT32 clock, UINT32 rate);

extern void ym3812_shutdown(FM_OPL *chip);
extern void ym3812_reset_chip(FM_OPL *chip);
extern int ym3812_write(FM_OPL *chip, int a, int v);
extern unsigned char ym3812_read(FM_OPL *chip, int a);
extern unsigned char ym3812_peek(FM_OPL *chip, int a);
extern int ym3812_timer_over(FM_OPL *chip, int c);

/*
 * Generate samples for one of the YM3812's
 *
 * 'which' is the virtual YM3812 number
 * '*buffer' is the output buffer pointer
 * 'length' is the number of samples that should be generated
 */
extern void ym3812_update_one(FM_OPL *chip, OPLSAMPLE *buffer, int length);

/*
 * Initialize YM3526 emulator.
 *
 * 'num' is the number of virtual YM3526's to allocate
 * 'clock' is the chip clock in Hz
 * 'rate' is sampling rate
 */
extern FM_OPL *ym3526_init(UINT32 clock, UINT32 rate);

extern void ym3526_shutdown(FM_OPL *chip);
extern void ym3526_reset_chip(FM_OPL *chip);
extern int ym3526_write(FM_OPL *chip, int a, int v);
extern unsigned char ym3526_read(FM_OPL *chip, int a);
extern unsigned char ym3526_peek(FM_OPL *chip, int a);
extern int ym3526_timer_over(FM_OPL *chip, int c);

struct snapshot_s;
extern int ym3526_snapshot_read_module(struct snapshot_s *s);
extern int ym3526_snapshot_write_module(struct snapshot_s *s);

/*
 * Generate samples for one of the YM3526's
 *
 * 'which' is the virtual YM3526 number
 * '*buffer' is the output buffer pointer
 * 'length' is the number of samples that should be generated
 */
extern void ym3526_update_one(FM_OPL *chip, OPLSAMPLE *buffer, int length);


extern int connect1_is_output0(int *connect);
extern void set_connect1(FM_OPL *chip, int x, int y, int output0);

#endif /* VICE_FMOPL_H */
