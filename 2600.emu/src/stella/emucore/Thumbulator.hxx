//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Thumbulator.hxx 3290 2016-02-27 19:58:20Z stephena $
//============================================================================

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifdef THUMB_SUPPORT

#ifndef THUMBULATOR_HXX
#define THUMBULATOR_HXX

#include "bspf.hxx"

#define ROMADDMASK 0x7FFF
#define RAMADDMASK 0x1FFF

#define ROMSIZE (ROMADDMASK+1)
#define RAMSIZE (RAMADDMASK+1)

#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
#define CPSR_C (1<<29)
#define CPSR_V (1<<28)

class Thumbulator
{
  public:
    Thumbulator(const uInt16* rom, uInt16* ram, bool traponfatal);

    /**
      Run the ARM code, and return when finished.  A runtime_error exception is
      thrown in case of any fatal errors/aborts (if enabled), containing the
      actual error, and the contents of the registers at that point in time.

      @return  The results of any debugging output (if enabled),
               otherwise an empty string
    */
    string run();

    /**
      Normally when a fatal error is encountered, the ARM emulation
      immediately throws an exception and exits.  This method allows execution
      to continue, and simply log the error.

      Note that this is meant for developers only, and should normally be
      always enabled.  It can be used to temporarily ignore illegal reads
      and writes, but a ROM which consistently performs these operations
      should be fixed, as it can cause crashes on real hardware.

      @param enable  Enable (the default) or disable exceptions on fatal errors
    */
    static void trapFatalErrors(bool enable) { trapOnFatal = enable; }

  private:
    uInt32 read_register(uInt32 reg);
    void write_register(uInt32 reg, uInt32 data);
    uInt32 fetch16(uInt32 addr);
    uInt32 fetch32(uInt32 addr);
    uInt32 read16(uInt32 addr);
    uInt32 read32(uInt32 addr);
    void write16(uInt32 addr, uInt32 data);
    void write32(uInt32 addr, uInt32 data);

    void do_zflag(uInt32 x);
    void do_nflag(uInt32 x);
    void do_cflag(uInt32 a, uInt32 b, uInt32 c);
    void do_vflag(uInt32 a, uInt32 b, uInt32 c);
    void do_cflag_bit(uInt32 x);
    void do_vflag_bit(uInt32 x);

    // Throw a runtime_error exception containing an error referencing the
    // given message and variables
    // Note that the return value is never used in these methods
    int fatalError(const char* opcode, uInt32 v1, const char* msg);
    int fatalError(const char* opcode, uInt32 v1, uInt32 v2, const char* msg);

    void dump_counters();
    void dump_regs();
    int execute();
    int reset();

  private:
    const uInt16* rom;
    uInt16* ram;

    uInt32 reg_norm[16]; // normal execution mode, do not have a thread mode
    uInt32 cpsr, mamcr;
    bool handler_mode;
    uInt32 systick_ctrl, systick_reload, systick_count, systick_calibrate;
    uInt64 instructions, fetches, reads, writes, systick_ints;

    ostringstream statusMsg;

    static bool trapOnFatal;

  private:
    // Following constructors and assignment operators not supported
    Thumbulator() = delete;
    Thumbulator(const Thumbulator&) = delete;
    Thumbulator(Thumbulator&&) = delete;
    Thumbulator& operator=(const Thumbulator&) = delete;
    Thumbulator& operator=(Thumbulator&&) = delete;
};

#endif  // THUMBULATOR_HXX

#endif
