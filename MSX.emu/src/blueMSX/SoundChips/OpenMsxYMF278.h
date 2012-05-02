// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.

#ifndef __YMF278_HH__
#define __YMF278_HH__

//using namespace std;


typedef unsigned long  EmuTime;
typedef unsigned char  byte;
typedef unsigned short word;


#ifndef OPENMSX_SOUNDDEVICE
#define OPENMSX_SOUNDDEVICE

extern "C" {
#include "AudioMixer.h"
}

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


class YMF278Slot
{
	public:
		YMF278Slot();
		void reset();
		int compute_rate(int val);
		unsigned int decay_rate(int num, int sample_rate);
		void envelope_next(int sample_rate);
		inline int compute_vib();
		inline int compute_am();
		void set_lfo(int newlfo);

		short wave;		// wavetable number
		short FN;		// f-number
		char OCT;		// octave
		char PRVB;		// pseudo-reverb
		char LD;		// level direct
		char TL;		// total level
		char pan;		// panpot
		char lfo;		// LFO
		char vib;		// vibrato
		char AM;		// AM level

		char AR;
		char D1R;
		int  DL;
		char D2R;
		char RC;   		// rate correction
		char RR;

		int step;               // fixed-point frequency step
		int stepptr;		// fixed-point pointer into the sample
		int pos;
		short sample1, sample2;

		bool active;		// slot keyed on
		byte bits;		// width of the samples
		int startaddr;
		int loopaddr;
		int endaddr;

		byte state;
		int env_vol;
		unsigned int env_vol_step;
		unsigned int env_vol_lim;

		bool lfo_active;
		int lfo_cnt;
		int lfo_step;
		int lfo_max;
};

static const int MASTER_CLK = 33868800;

class YMF278 : public SoundDevice
{
	public:
		YMF278(short volume, int ramSize, void* romData, int romSize,
		       const EmuTime &time);
		virtual ~YMF278();
		void reset(const EmuTime &time);
		void writeRegOPL4(byte reg, byte data, const EmuTime &time);
		byte peekRegOPL4(byte reg, const EmuTime &time);
		byte readRegOPL4(byte reg, const EmuTime &time);
		byte peekStatus(const EmuTime &time);
		byte readStatus(const EmuTime &time);
        void* getRom() { return rom; }	
        void* getRam() { return ram; }	
        int getRomSize() { return endRom; }
        int getRamSize() { return endRam - endRom; }
		virtual void setSampleRate(int sampleRate, int Oversampling);
		virtual void setInternalVolume(short newVolume);
		virtual int* updateBuffer(int length);

        void loadState();
        void saveState();
	
	private:
		byte readMem(unsigned int address);
		void writeMem(unsigned int address, byte value);
		short getSample(YMF278Slot &op);
		void advance();
		void checkMute();
		bool anyActive();
		void keyOnHelper(YMF278Slot& slot);

		int buffer[AUDIO_STEREO_BUFFER_SIZE];
		byte* rom;
		byte* ram;

        int oplOversampling;
        DoubleT freqbase;

		YMF278Slot slots[24];

        int ramSize;
		
		unsigned int eg_cnt;	// global envelope generator counter
		unsigned int eg_timer;	// global envelope generator counter
		unsigned int eg_timer_add;		// step of eg_timer
		unsigned int eg_timer_overflow;	// envelope generator timer overlfows every 1 sample (on real chip)
		
		char wavetblhdr;
		char memmode;
		int memadr;

		int fm_l, fm_r;
		int pcm_l, pcm_r;

		unsigned int endRom;
		unsigned int endRam;

		// precalculated attenuation values with some marging for
		// enveloppe and pan levels
		int volume[256 * 4];

		byte regs[256];

		unsigned long LD_Time;
		unsigned long BUSY_Time;
};

#endif

