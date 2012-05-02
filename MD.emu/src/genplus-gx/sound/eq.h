//---------------------------------------------------------------------------
//
//                                3 Band EQ :)
//
// EQ.H - Header file for 3 band EQ
//
// (c) Neil C / Etanza Systems / 2K6
//
// Shouts / Loves / Moans = etanza at lycos dot co dot uk 
//
// This work is hereby placed in the public domain for all purposes, including
// use in commercial applications.
//
// The author assumes NO RESPONSIBILITY for any problems caused by the use of
// this software.
//
//----------------------------------------------------------------------------

#ifndef __EQ3BAND__
#define __EQ3BAND__

// ------------
//| Structures |
// ------------

typedef struct {
    // Filter #1 (Low band)

	SysDDec lf;      // Frequency
	SysDDec f1p0;      // Poles ...
	SysDDec f1p1;
	SysDDec f1p2;
	SysDDec f1p3;

    // Filter #2 (High band)

	SysDDec hf;      // Frequency
	SysDDec f2p0;      // Poles ...
	SysDDec f2p1;
	SysDDec f2p2;
	SysDDec f2p3;

    // Sample history buffer

	SysDDec sdm1;      // Sample data minus 1
	SysDDec sdm2;      //                   2
	SysDDec sdm3;      //                   3

    // Gain Controls

	SysDDec lg;      // low  gain
	SysDDec mg;      // mid  gain
	SysDDec hg;      // high gain

} EQSTATE;


// ---------
//| Exports |
// ---------

extern void init_3band_state(EQSTATE * es, int lowfreq, int highfreq,
           int mixfreq);
extern SysDDec do_3band(EQSTATE * es, int sample);


#endif        // #ifndef __EQ3BAND__
