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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifndef THUMBULATOR_HXX
#define THUMBULATOR_HXX

class Cartridge;

#include "bspf.hxx"
#include "Console.hxx"

#ifdef RETRON77
  #define UNSAFE_OPTIMIZATIONS
  #define NO_THUMB_STATS
#endif

#define ROMADDMASK 0x7FFFF
#define RAMADDMASK 0x7FFF

#define ROMSIZE (ROMADDMASK+1)      // 512KB
#define RAMSIZE (RAMADDMASK+1)      // 32KB

#define CPSR_N (1u<<31)
#define CPSR_Z (1u<<30)
#define CPSR_C (1u<<29)
#define CPSR_V (1u<<28)

class Thumbulator
{
  public:
    // control cartridge specific features of the Thumbulator class,
    // such as the start location for calling custom code
    enum class ConfigureFor {
      BUS,      // cartridges of type BUS
      CDF,      // cartridges of type CDF
      CDF1,     // cartridges of type CDF version 1
      CDFJ,     // cartridges of type CDFJ
      CDFJplus, // cartridges of type CDFJ+
      DPCplus   // cartridges of type DPC+
    };

    struct Stats {
    #ifndef NO_THUMB_STATS
      uInt32 fetches{0}, reads{0}, writes{0};
    #endif
    };

    Thumbulator(const uInt16* rom_ptr, uInt16* ram_ptr, uInt32 rom_size,
                const uInt32 c_base, const uInt32 c_start, const uInt32 c_stack,
                bool traponfatal, Thumbulator::ConfigureFor configurefor,
                Cartridge* cartridge);

    /**
      Run the ARM code, and return when finished.  A runtime_error exception is
      thrown in case of any fatal errors/aborts (if enabled), containing the
      actual error, and the contents of the registers at that point in time.

      @return  The results of any debugging output (if enabled),
               otherwise an empty string
    */
    string run();
    string run(uInt32 cycles);
    const Stats& stats() const { return _stats; }

#ifndef UNSAFE_OPTIMIZATIONS
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
#endif

    /**
      Inform the Thumbulator class about the console currently in use,
      which is used to accurately determine how many 6507 cycles have
      passed while ARM code is being executed.
    */
    void setConsoleTiming(ConsoleTiming timing);

  private:

    enum class Op : uInt8 {
      invalid,
      adc,
      add1, add2, add3, add4, add5, add6, add7,
      and_,
      asr1, asr2,
      b1, b2,
      bic,
      bkpt,
      blx1, blx2,
      bx,
      cmn,
      cmp1, cmp2, cmp3,
      cps,
      cpy,
      eor,
      ldmia,
      ldr1, ldr2, ldr3, ldr4,
      ldrb1, ldrb2,
      ldrh1, ldrh2,
      ldrsb,
      ldrsh,
      lsl1, lsl2,
      lsr1, lsr2,
      mov1, mov2, mov3,
      mul,
      mvn,
      neg,
      orr,
      pop,
      push,
      rev,
      rev16,
      revsh,
      ror,
      sbc,
      setend,
      stmia,
      str1, str2, str3,
      strb1, strb2,
      strh1, strh2,
      sub1, sub2, sub3, sub4,
      swi,
      sxtb,
      sxth,
      tst,
      uxtb,
      uxth
    };

  private:
    uInt32 read_register(uInt32 reg);
    void write_register(uInt32 reg, uInt32 data);
    uInt32 fetch16(uInt32 addr);
    uInt32 read16(uInt32 addr);
    uInt32 read32(uInt32 addr);
#ifndef UNSAFE_OPTIMIZATIONS
    bool isProtected(uInt32 addr);
#endif
    void write16(uInt32 addr, uInt32 data);
    void write32(uInt32 addr, uInt32 data);
    void updateTimer(uInt32 cycles);

    static Op decodeInstructionWord(uint16_t inst);

    void do_zflag(uInt32 x);
    void do_nflag(uInt32 x);
    void do_cflag(uInt32 a, uInt32 b, uInt32 c);
    void do_vflag(uInt32 a, uInt32 b, uInt32 c);
    void do_cflag_bit(uInt32 x);
    void do_vflag_bit(uInt32 x);

#ifndef UNSAFE_OPTIMIZATIONS
    // Throw a runtime_error exception containing an error referencing the
    // given message and variables
    // Note that the return value is never used in these methods
    int fatalError(const char* opcode, uInt32 v1, const char* msg);
    int fatalError(const char* opcode, uInt32 v1, uInt32 v2, const char* msg);

    void dump_counters();
    void dump_regs();
#endif
    int execute();
    int reset();

  private:
    const uInt16* rom{nullptr};
    uInt32 romSize{0};
    uInt32 cBase{0};
    uInt32 cStart{0};
    uInt32 cStack{0};
    const unique_ptr<Op[]> decodedRom;  // NOLINT
    uInt16* ram{nullptr};
    std::array<uInt32, 16> reg_norm; // normal execution mode, do not have a thread mode
    uInt32 cpsr{0}, mamcr{0};
    bool handler_mode{false};
    uInt32 systick_ctrl{0}, systick_reload{0}, systick_count{0}, systick_calibrate{0};
  #ifndef UNSAFE_OPTIMIZATIONS
    uInt32 instructions{0};
  #endif
    Stats _stats;

    // For emulation of LPC2103's timer 1, used for NTSC/PAL/SECAM detection.
    // Register names from documentation:
    // http://www.nxp.com/documents/user_manual/UM10161.pdf
    uInt32 T1TCR{0};  // Timer 1 Timer Control Register
    uInt32 T1TC{0};   // Timer 1 Timer Counter
    double timing_factor{0.0};

#ifndef UNSAFE_OPTIMIZATIONS
    ostringstream statusMsg;

    static bool trapOnFatal;
#endif

    ConfigureFor configuration;

    Cartridge* myCartridge;

  private:
    // Following constructors and assignment operators not supported
    Thumbulator() = delete;
    Thumbulator(const Thumbulator&) = delete;
    Thumbulator(Thumbulator&&) = delete;
    Thumbulator& operator=(const Thumbulator&) = delete;
    Thumbulator& operator=(Thumbulator&&) = delete;
};

#endif  // THUMBULATOR_HXX
