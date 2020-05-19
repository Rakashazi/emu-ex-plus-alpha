// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.

#ifndef __YMF262_HH__
#define __YMF262_HH__

#include <string>
extern "C" {
#include "Board.h"
}



typedef unsigned long  EmuTime;
typedef unsigned char  byte;
typedef unsigned short word;

extern "C" {
#include "AudioMixer.h"
}


class TimerCallback
{
	public:
		virtual void callback(byte value) = 0;
};

extern void moonsoundTimerSet(void* ref, int timer, int count);
extern void moonsoundTimerStart(void* ref, int timer, int start, byte timerRef);

template<int freq, byte flag>
class Timer
{
	public:
        Timer(TimerCallback *cb, void* reference) {
            ref = reference;
            id = 12500 / freq;
        }
		virtual ~Timer() {}
		void setValue(byte value) {
            moonsoundTimerSet(ref, id, id * (256 - value));
        }
		void setStart(bool start, const EmuTime &time) {
            moonsoundTimerStart(ref, id, start, flag);
        }

	private:
        void* ref;
        int id;
};


class IRQHelper 
{
public:
    IRQHelper() {}
    static void set() {
        boardSetInt(0x8);
    }
    static void reset() {
        boardClearInt(0x8);
    }
};

#ifndef OPENMSX_SOUNDDEVICE
#define OPENMSX_SOUNDDEVICE

class SoundDevice
{
	public:
        SoundDevice() : internalMuted(true) {}
		void setVolume(short newVolume) {
	        setInternalVolume(newVolume);
        }

	protected:
		virtual void setInternalVolume(short newVolume) = 0;
        void setInternalMute(bool muted) { internalMuted = muted; }
        bool isInternalMuted() const { return internalMuted; }
	public:
		virtual void setSampleRate(int newSampleRate, int Oversampling) = 0;
		virtual int* updateBuffer(int length) = 0;

	private:
		bool internalMuted;
};

#endif

class YMF262Slot
{
	public:
		YMF262Slot();
		inline int volume_calc(byte LFO_AM);
		inline void FM_KEYON(byte key_set);
		inline void FM_KEYOFF(byte key_clr);

		byte ar;	// attack rate: AR<<2
		byte dr;	// decay rate:  DR<<2
		byte rr;	// release rate:RR<<2
		byte KSR;	// key scale rate
		byte ksl;	// keyscale level
		byte ksr;	// key scale rate: kcode>>KSR
		byte mul;	// multiple: mul_tab[ML]

		// Phase Generator 
		unsigned int Cnt;	// frequency counter
		unsigned int Incr;	// frequency counter step
		byte FB;	// feedback shift value
		int op1_out[2];	// slot1 output for feedback
		byte CON;	// connection (algorithm) type

		// Envelope Generator 
		byte eg_type;	// percussive/non-percussive mode 
		byte state;	// phase type
		unsigned int TL;	// total level: TL << 2
		int TLL;	// adjusted now TL
		int volume;	// envelope counter
		int sl;		// sustain level: sl_tab[SL]

		unsigned int eg_m_ar;// (attack state)
		byte eg_sh_ar;	// (attack state)
		byte eg_sel_ar;	// (attack state)
		unsigned int eg_m_dr;// (decay state)
		byte eg_sh_dr;	// (decay state)
		byte eg_sel_dr;	// (decay state)
		unsigned int eg_m_rr;// (release state)
		byte eg_sh_rr;	// (release state)
		byte eg_sel_rr;	// (release state)

		byte key;	// 0 = KEY OFF, >0 = KEY ON

		// LFO 
		byte  AMmask;	// LFO Amplitude Modulation enable mask 
		byte vib;	// LFO Phase Modulation enable flag (active high)

		// waveform select 
		byte waveform_number;
		unsigned int wavetable;

		int connect;	// slot output pointer
};

class YMF262Channel
{
	public:
		YMF262Channel();
		void chan_calc(byte LFO_AM);
		void chan_calc_ext(byte LFO_AM);
		void CALC_FCSLOT(YMF262Slot &slot);

		YMF262Slot slots[2];

		int block_fnum;	// block+fnum
		int fc;		// Freq. Increment base
		int ksl_base;	// KeyScaleLevel Base step
		byte kcode;	// key code (for key scaling)

		// there are 12 2-operator channels which can be combined in pairs
		// to form six 4-operator channel, they are:
		//  0 and 3,
		//  1 and 4,
		//  2 and 5,
		//  9 and 12,
		//  10 and 13,
		//  11 and 14
		byte extended;	// set to 1 if this channel forms up a 4op channel with another channel(only used by first of pair of channels, ie 0,1,2 and 9,10,11) 
};

// Bitmask for register 0x04 
static const int R04_ST1          = 0x01;	// Timer1 Start
static const int R04_ST2          = 0x02;	// Timer2 Start
static const int R04_MASK_T2      = 0x20;	// Mask Timer2 flag 
static const int R04_MASK_T1      = 0x40;	// Mask Timer1 flag 
static const int R04_IRQ_RESET    = 0x80;	// IRQ RESET 

// Bitmask for status register 
static const int STATUS_T2      = R04_MASK_T2;
static const int STATUS_T1      = R04_MASK_T1;

class YMF262 : public SoundDevice, public TimerCallback
{
	public:
		YMF262(short volume, const EmuTime &time, void* ref);
		virtual ~YMF262();
		
		virtual void reset(const EmuTime &time);
		void writeReg(int r, byte v, const EmuTime &time);
		byte peekReg(int reg);
		byte readReg(int reg);
		byte peekStatus();
		byte readStatus();
		
		virtual void setInternalVolume(short volume);
		virtual void setSampleRate(int sampleRate, int Oversampling);
		virtual int* updateBuffer(int length);

		void callback(byte flag);

        void loadState();
        void saveState();

	private:
		void writeRegForce(int r, byte v, const EmuTime &time);
		void init_tables(void);
		void setStatus(byte flag);
		void resetStatus(byte flag);
		void changeStatusMask(byte flag);
		void advance_lfo();
		void advance();
		void chan_calc_rhythm(bool noise);
		void set_mul(byte sl, byte v);
		void set_ksl_tl(byte sl, byte v);
		void set_ar_dr(byte sl, byte v);
		void set_sl_rr(byte sl, byte v);
		void update_channels(YMF262Channel &ch);
		void checkMute();
		bool checkMuteHelper();

        int buffer[AUDIO_MONO_BUFFER_SIZE];
		IRQHelper irq;
		Timer<12500, STATUS_T1> timer1;	//  80us
		Timer< 3125, STATUS_T2> timer2;	// 320us

        int oplOversampling;

        YMF262Channel channels[18];	// OPL3 chips have 18 channels

		byte reg[512];

        unsigned int pan[18*4];		// channels output masks (0xffffffff = enable); 4 masks per one channel 

		unsigned int eg_cnt;		// global envelope generator counter
		unsigned int eg_timer;		// global envelope generator counter works at frequency = chipclock/288 (288=8*36) 
		unsigned int eg_timer_add;		// step of eg_timer

		unsigned int fn_tab[1024];		// fnumber->increment counter

		// LFO 
		byte LFO_AM;
		byte LFO_PM;
		
		byte lfo_am_depth;
		byte lfo_pm_depth_range;
		unsigned int lfo_am_cnt;
		unsigned int lfo_am_inc;
		unsigned int lfo_pm_cnt;
		unsigned int lfo_pm_inc;

		unsigned int noise_rng;		// 23 bit noise shift register
		unsigned int noise_p;		// current noise 'phase'
		unsigned int noise_f;		// current noise period

		bool OPL3_mode;			// OPL3 extension enable flag
		byte rhythm;			// Rhythm mode
		byte nts;			// NTS (note select)

		byte status;			// status flag
		byte status2;
		byte statusMask;		// status mask

		int chanout[20];		// 18 channels + two phase modulation
		short maxVolume;
};

#endif

