// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.

#ifndef __YM2413_2_HH__
#define __YM2413_2_HH__

#include <imagine/util/builtins.h>

//using namespace std;


typedef unsigned long  EmuTime;
typedef unsigned char  byte;
typedef unsigned short word;

extern "C" {
#include "AudioMixer.h"
}


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

#ifndef OPENMSX_YM2413BASE
#define OPENMSX_YM2413BASE

class OpenYM2413Base : public SoundDevice
{
public:
    OpenYM2413Base() {}
    virtual ~OpenYM2413Base() {}
		
	virtual void reset(const EmuTime &time) = 0;
	virtual void writeReg(byte r, byte v, const EmuTime &time) = 0;
	virtual byte peekReg(byte r) = 0;
	
	virtual int* updateBuffer(int length) = 0;
	virtual void setSampleRate(int sampleRate, int Oversampling) = 0;
	virtual void setInternalVolume(short newVolume) = 0;

    virtual void loadState() = 0;
    virtual void saveState() = 0;
};

#endif

// Size of Sintable ( 8 -- 18 can be used, but 9 recommended.)
static const int PG_BITS = 9;
static const int PG_WIDTH = 1 << PG_BITS;

// Phase increment counter
static const int DP_BITS = 18;
static const int DP_WIDTH = 1 << DP_BITS;
static const int DP_BASE_BITS = DP_BITS - PG_BITS;

// Dynamic range (Accuracy of sin table)
static const int DB_BITS = 8;
static const DoubleT DB_STEP = 48.0 / (1 << DB_BITS);
static const int DB_MUTE = 1 << DB_BITS;

// Dynamic range of envelope
static const DoubleT EG_STEP = 0.375;
static const int EG_BITS = 7;
static const int EG_MUTE = 1 << EG_BITS;

// Dynamic range of total level
static const DoubleT TL_STEP = 0.75;
static const int TL_BITS = 6;
static const int TL_MUTE = 1 << TL_BITS;

// Dynamic range of sustine level
static const DoubleT SL_STEP = 3.0;
static const int SL_BITS = 4;
static const int SL_MUTE = 1 << SL_BITS;

// Bits for liner value
static const int DB2LIN_AMP_BITS = 8;
static const int SLOT_AMP_BITS = DB2LIN_AMP_BITS;

// Bits for envelope phase incremental counter
static const int EG_DP_BITS = 22;
static const int EG_DP_WIDTH = 1 << EG_DP_BITS;

// Bits for Pitch and Amp modulator 
static const int PM_PG_BITS = 8;
static const int PM_PG_WIDTH = 1 << PM_PG_BITS;
static const int PM_DP_BITS = 16;
static const int PM_DP_WIDTH = 1 << PM_DP_BITS;
static const int AM_PG_BITS = 8;
static const int AM_PG_WIDTH = 1 << AM_PG_BITS;
static const int AM_DP_BITS = 16;
static const int AM_DP_WIDTH = 1 << AM_DP_BITS;

// PM table is calcurated by PM_AMP * pow(2,PM_DEPTH*sin(x)/1200) 
static const int PM_AMP_BITS = 8;
static const int PM_AMP = 1 << PM_AMP_BITS;

// PM speed(Hz) and depth(cent) 
static const DoubleT PM_SPEED = 6.4;
static const DoubleT PM_DEPTH = 13.75;

// AM speed(Hz) and depth(dB)
static const DoubleT AM_SPEED = 3.6413;
static const DoubleT AM_DEPTH = 4.875;


static const int NULL_PATCH_IDX = 19 * 2;

class OpenYM2413_2 : public OpenYM2413Base
{
	struct Patch {
		Patch();
		Patch(int n, const byte* data);
		
		bool AM;
        bool PM;
        bool EG;
		byte KR; // 0-1
		byte ML; // 0-15
		byte KL; // 0-3
		byte TL; // 0-63
		byte FB; // 0-7
		byte WF; // 0-1
		byte AR; // 0-15
		byte DR; // 0-15
		byte SL; // 0-15
		byte RR; // 0-15
	};
	
	class Slot {
	public:
		Slot(bool type);
		void reset(bool type);

		inline void slotOn();
		inline void slotOn2();
		inline void slotOff();
		inline void setPatch(int idx);
		inline void setVolume(int volume);
		inline void calc_phase(int lfo_pm);
		inline void calc_envelope(int lfo_am);
		inline int calc_slot_car(int fm);
		inline int calc_slot_mod();
		inline int calc_slot_tom();
		inline int calc_slot_snare(bool noise);
		inline int calc_slot_cym(unsigned int pgout_hh);
		inline int calc_slot_hat(int pgout_cym, bool noise);
		inline void updatePG();
		inline void updateTLL();
		inline void updateRKS();
		inline void updateWF();
		inline void updateEG();
		inline void updateAll();
		inline static int wave2_4pi(int e);
		inline static int wave2_8pi(int e);
		inline static int EG2DB(int d);
		/*constexpr*/	static int SL2EG(int d)
		{
			return d * (int)(SL_STEP / EG_STEP);
		}
	
		Patch* patches;
        int patchIdx;
		bool type;		// 0 : modulator 1 : carrier 
		bool slot_on_flag;

		// OUTPUT
		int feedback;
		int output[5];	// Output value of slot 

		// for Phase Generator (PG)
		word* sintbl;		// Wavetable
        int sintblIdx;
		unsigned int phase;	// Phase 
		unsigned int dphase;	// Phase increment amount 
		unsigned int pgout;	// output

		// for Envelope Generator (EG)
		int fnum;		// F-Number
		int block;		// Block
		int volume;		// Current volume
		int sustine;		// Sustine 1 = ON, 0 = OFF
		int tll;		// Total Level + Key scale level
		int rks;		// Key scale offset (Rks)
		int eg_mode;		// Current state
		unsigned int eg_phase;	// Phase
		unsigned int eg_dphase;	// Phase increment amount
		unsigned egout;		// output
	};
	friend class Slot;
	
	class Channel {
	public:
		Channel();
		void reset();
		inline void setPatch(int num);
		inline void setSustine(bool sustine);
		inline void setVol(int volume);
		inline void setFnumber(int fnum);
		inline void setBlock(int block);
		inline void keyOn();
		inline void keyOff();

		Patch* patches;
		int patch_number;
		Slot mod, car;
	};

public:
	OpenYM2413_2(const char *name, short volume, const EmuTime& time);
	virtual ~OpenYM2413_2();

	virtual void reset(const EmuTime& time);
	virtual void writeReg(byte reg, byte value, const EmuTime& time);
    virtual byte peekReg(byte r) { return reg[r]; }

	// SoundDevice
	virtual const char * getName() const;
	virtual const char * getDescription() const;
	virtual void setInternalVolume(short newVolume);
	virtual int* updateBuffer(int length);
	virtual void setSampleRate(int newSampleRate, int Oversampling);
    
    virtual void loadState();
    virtual void saveState();

private:
	inline int calcSample();

	void checkMute();
	bool checkMuteHelper();

	static void makeAdjustTable();
	static void makeSinTable();
	static int lin2db(DoubleT d);
	static void makePmTable();
	static void makeAmTable();
	static void makeDphaseTable(int sampleRate);
	static void makeTllTable();
	static void makeDphaseARTable(int sampleRate);
	static void makeDphaseDRTable(int sampleRate);
	static void makeRksTable();
	static void makeDB2LinTable();
	
	inline void keyOn_BD();
	inline void keyOn_SD();
	inline void keyOn_TOM();
	inline void keyOn_HH();
	inline void keyOn_CYM();
	inline void keyOff_BD();
	inline void keyOff_SD();
	inline void keyOff_TOM();
	inline void keyOff_HH();
	inline void keyOff_CYM();
	inline void update_rhythm_mode();
	inline void update_key_status();
	inline void update_noise();
	inline void update_ampm();

	inline static int TL2EG(int d);
	inline static unsigned int DB_POS(DoubleT x);
	inline static unsigned int DB_NEG(DoubleT x);

	// Debuggable
	virtual unsigned getSize() const;
	//virtual const std::string& getDescription() const;  // also in SoundDevice!!
	virtual byte read(unsigned address);
	virtual void write(unsigned address, byte value);

    int filter(int input);
private:
	int maxVolume;

	// Register
	byte reg[0x40];

	// Pitch Modulator
	unsigned int pm_phase;
	int lfo_pm;

	// Amp Modulator
	unsigned int am_phase;
	int lfo_am;

	// Noise Generator
	int noise_seed;

        int in[5];

	// CHECK check with orig code header file line 98-104
	
	// Channel & Slot
	Channel ch[9];

	// Voice Data
	Patch patches[19 * 2 + 1];

	// dB to linear table (used by Slot)
	static short dB2LinTab[(DB_MUTE + DB_MUTE) * 2];

	// WaveTable for each envelope amp
	static word fullsintable[PG_WIDTH];
	static word halfsintable[PG_WIDTH];

	static unsigned int dphaseNoiseTable[512][8];

	static word* waveform[2];

	// LFO Table
	static int pmtable[PM_PG_WIDTH];
	static int amtable[AM_PG_WIDTH];

	// Noise and LFO
	static unsigned int pm_dphase;
	static unsigned int am_dphase;

	// Liner to Log curve conversion table (for Attack rate).
	static word AR_ADJUST_TABLE[1 << EG_BITS];

	// Definition of envelope mode
	enum { READY, ATTACK, DECAY, SUSHOLD, SUSTINE, RELEASE, SETTLE, FINISH };

    // Phase incr table for Attack
    static unsigned int dphaseARTable[16][16];
    // Phase incr table for Decay and Release
    static unsigned int dphaseDRTable[16][16];

    // KSL + TL Table
    static unsigned int tllTable[16][8][1 << TL_BITS][4];
    static int rksTable[2][8][2];

    // Phase incr table for PG 
    static unsigned int dphaseTable[512][8][16];

	const char * name;

    int buffer[AUDIO_MONO_BUFFER_SIZE];
};

#endif

