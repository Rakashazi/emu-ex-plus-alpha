// $Id: OpenMsxY8950Adpcm.h,v 1.2 2006-06-17 21:42:32 vincent_van_dam Exp $

#ifndef __Y8950ADPCM_HH__
#define __Y8950ADPCM_HH__

#include "MsxTypes.h"

using namespace std;


typedef unsigned long  EmuTime;
typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned __int64 uint64;
class Y8950;

class Y8950Adpcm
{
public:
	Y8950Adpcm(Y8950& y8950, const string& name, int sampleRam);
	virtual ~Y8950Adpcm();
	
	void reset(const EmuTime& time);
	void setSampleRate(int sr);
	bool muted();
	void writeReg(byte rg, byte data, const EmuTime& time);
	byte readReg(byte rg);
	int calcSample();
    
    void pushTime(const EmuTime &time);

    void loadState();
    void saveState();

private:
	// Debuggable
	virtual unsigned getSize() const;
	virtual const string& getDescription() const;
	virtual byte read(unsigned address);
	virtual void write(unsigned address, byte value);
	
	// Schedulable
	virtual void executeUntil(const EmuTime& time, int userData);
	virtual const string& schedName() const;

	void schedule(const EmuTime& time);
    void unschedule(const EmuTime &time);
	int CLAP(int min, int x, int max);
	void restart();

	Y8950& y8950;
	const string name;

	int sampleRate;
	
	int ramSize;
	int startAddr;
	int stopAddr;
	int playAddr;
	int addrMask;
	int memPntr;
	bool romBank;
	byte* ramBank;
	
	bool playing;
	int volume;
	word delta;
	unsigned int nowStep, step;
	int out, output;
	int diff;
	int nextLeveling;
	int sampleStep;
	int volumeWStep;
        
    EmuTime oldTime;
    uint64  sysTime;
    uint64  syncTime;
	
	byte reg7;
	byte reg15;
};


#endif 
