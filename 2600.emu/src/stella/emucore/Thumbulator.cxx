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

#include "bspf.hxx"
#include "Base.hxx"
#include "Cart.hxx"
#include "Thumbulator.hxx"
using Common::Base;

// Uncomment the following to enable specific functionality
// WARNING!!! This slows the runtime to a crawl
// #define THUMB_DISS
//#define THUMB_DBUG

#if defined(THUMB_DISS)
  #define DO_DISS(statement) statement
#else
  #define DO_DISS(statement)
#endif
#if defined(THUMB_DBUG)
  #define DO_DBUG(statement) statement
#else
  #define DO_DBUG(statement)
#endif

#ifdef __BIG_ENDIAN__
  #define CONV_DATA(d)   (((d & 0xFFFF)>>8) | ((d & 0xffff)<<8)) & 0xffff
  #define CONV_RAMROM(d) ((d>>8) | (d<<8)) & 0xffff
#else
  #define CONV_DATA(d)   (d & 0xFFFF)
  #define CONV_RAMROM(d) (d)
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(const uInt16* rom_ptr, uInt16* ram_ptr, uInt32 rom_size,
                         const uInt32 c_base, const uInt32 c_start, const uInt32 c_stack,
                         bool traponfatal, Thumbulator::ConfigureFor configurefor,
                         Cartridge* cartridge)
  : rom{rom_ptr},
    romSize{rom_size},
    cBase{c_base},
    cStart{c_start},
    cStack{c_stack},
    decodedRom{make_unique<Op[]>(romSize / 2)},  // NOLINT
    ram{ram_ptr},
    configuration{configurefor},
    myCartridge{cartridge}
{
  for(uInt32 i = 0; i < romSize / 2; ++i)
    decodedRom[i] = decodeInstructionWord(CONV_RAMROM(rom[i]));

  setConsoleTiming(ConsoleTiming::ntsc);
#ifndef UNSAFE_OPTIMIZATIONS
  trapFatalErrors(traponfatal);
#endif
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Thumbulator::run()
{
  reset();
  for(;;)
  {
    if(execute()) break;
#ifndef UNSAFE_OPTIMIZATIONS
    if(instructions > 500000) // way more than would otherwise be possible
      throw runtime_error("instructions > 500000");
#endif
  }
#if defined(THUMB_DISS) || defined(THUMB_DBUG)
  dump_counters();
  cout << statusMsg.str() << endl;
  return statusMsg.str();
#else
  return "";
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::setConsoleTiming(ConsoleTiming timing)
{
  // this sets how many ticks of the Harmony/Melody clock
  // will occur per tick of the 6507 clock
  constexpr double NTSC   = 70.0 / 1.193182;  // NTSC  6507 clock rate
  constexpr double PAL    = 70.0 / 1.182298;  // PAL   6507 clock rate
  constexpr double SECAM  = 70.0 / 1.187500;  // SECAM 6507 clock rate

  switch(timing)
  {
    case ConsoleTiming::ntsc:   timing_factor = NTSC;   break;
    case ConsoleTiming::secam:  timing_factor = SECAM;  break;
    case ConsoleTiming::pal:    timing_factor = PAL;    break;
    default:  break;  // satisfy compiler
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::updateTimer(uInt32 cycles)
{
  if (T1TCR & 1) // bit 0 controls timer on/off
    T1TC += uInt32(cycles * timing_factor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Thumbulator::run(uInt32 cycles)
{
  updateTimer(cycles);
  return run();
}

#ifndef UNSAFE_OPTIMIZATIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline int Thumbulator::fatalError(const char* opcode, uInt32 v1, const char* msg)
{
  statusMsg << "Thumb ARM emulation fatal error: " << endl
            << opcode << "(" << Base::HEX8 << v1 << "), " << msg << endl;
  dump_regs();
  if(trapOnFatal)
    throw runtime_error(statusMsg.str());
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline int Thumbulator::fatalError(const char* opcode, uInt32 v1, uInt32 v2,
                                   const char* msg)
{
  statusMsg << "Thumb ARM emulation fatal error: " << endl
            << opcode << "(" << Base::HEX8 << v1 << "," << v2 << "), " << msg << endl;
  dump_regs();
    if(trapOnFatal)
    throw runtime_error(statusMsg.str());
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::dump_counters()
{
  cout << endl << endl
       << "instructions " << instructions << endl;
#ifndef NO_THUMB_STATS
  cout << "fetches      " << _stats.fetches << endl
       << "reads        " << _stats.reads << endl
       << "writes       " << _stats.writes << endl
       << "memcycles    " << (_stats.fetches + _stats.reads + _stats.writes) << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::dump_regs()
{
  for (int cnt = 0; cnt <= 12; cnt++)
  {
    statusMsg << "R" << cnt << " = " << Base::HEX8 << reg_norm[cnt] << "  ";
    if((cnt + 1) % 4 == 0)
      statusMsg << endl;
  }
  statusMsg << endl
            << "SP = " << Base::HEX8 << reg_norm[13] << "  "
            << "LR = " << Base::HEX8 << reg_norm[14] << "  "
            << "PC = " << Base::HEX8 << reg_norm[15] << "  "
            << endl;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::fetch16(uInt32 addr)
{
#ifndef NO_THUMB_STATS
  ++_stats.fetches;
#endif

#ifndef UNSAFE_OPTIMIZATIONS
  uInt32 data;
  switch(addr & 0xF0000000)
  {
    case 0x00000000: //ROM
      addr &= ROMADDMASK;
      if(addr < 0x50)
        fatalError("fetch16", addr, "abort");
      addr >>= 1;
      data = CONV_RAMROM(rom[addr]);
      DO_DBUG(statusMsg << "fetch16(" << Base::HEX8 << addr << ")=" << Base::HEX4 << data << endl);
      return data;

    case 0x40000000: //RAM
      addr &= RAMADDMASK;
      addr >>= 1;
      data=CONV_RAMROM(ram[addr]);
      DO_DBUG(statusMsg << "fetch16(" << Base::HEX8 << addr << ")=" << Base::HEX4 << data << endl);
      return data;
  }
  return fatalError("fetch16", addr, "abort");
#else
  addr &= ROMADDMASK;
  addr >>= 1;
  return CONV_RAMROM(rom[addr]);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write16(uInt32 addr, uInt32 data)
{
#ifndef UNSAFE_OPTIMIZATIONS
  if((addr > 0x40007fff) && (addr < 0x50000000))
    fatalError("write16", addr, "abort - out of range");

  if (isProtected(addr)) fatalError("write16", addr, "to driver area");

  if(addr & 1)
    fatalError("write16", addr, "abort - misaligned");
#endif
#ifndef NO_THUMB_STATS
  ++_stats.writes;
#endif

  DO_DBUG(statusMsg << "write16(" << Base::HEX8 << addr << "," << Base::HEX8 << data << ")" << endl);

  switch(addr & 0xF0000000)
  {
    case 0x40000000: //RAM
      addr &= RAMADDMASK;
      addr >>= 1;
      ram[addr] = CONV_DATA(data);
      return;

#ifndef UNSAFE_OPTIMIZATIONS
    case 0xE0000000: //MAMCR
#else
    default:
#endif
      if(addr == 0xE01FC000)
      {
        DO_DBUG(statusMsg << "write16(" << Base::HEX8 << "MAMCR" << "," << Base::HEX8 << data << ") *" << endl);
        mamcr = data;
        return;
      }
  }
#ifndef UNSAFE_OPTIMIZATIONS
  fatalError("write16", addr, data, "abort");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write32(uInt32 addr, uInt32 data)
{
#ifndef UNSAFE_OPTIMIZATIONS
  if(addr & 3)
    fatalError("write32", addr, "abort - misaligned");

  if (isProtected(addr)) fatalError("write32", addr, "to driver area");
#endif
  DO_DBUG(statusMsg << "write32(" << Base::HEX8 << addr << "," << Base::HEX8 << data << ")" << endl);

  switch(addr & 0xF0000000)
  {
#ifndef UNSAFE_OPTIMIZATIONS
    case 0xF0000000: //halt
      dump_counters();
      throw runtime_error("HALT");
#endif

    case 0xE0000000: //periph
      switch(addr)
      {
#ifndef UNSAFE_OPTIMIZATIONS
        case 0xE0000000:
          DO_DISS(statusMsg << "uart: [" << char(data&0xFF) << "]" << endl);
          break;
#endif

        case 0xE0008004:  // T1TCR - Timer 1 Control Register
          T1TCR = data;
          break;

        case 0xE0008008:  // T1TC - Timer 1 Counter
          T1TC = data;
          break;

        case 0xE000E010:
        {
          uInt32 old = systick_ctrl;
          systick_ctrl = data & 0x00010007;
          if(((old & 1) == 0) && (systick_ctrl & 1))
          {
            // timer started, load count
            systick_count = systick_reload;
          }
          break;
        }

        case 0xE000E014:
          systick_reload = data & 0x00FFFFFF;
          break;

        case 0xE000E018:
          systick_count = data & 0x00FFFFFF;
          break;

        case 0xE000E01C:
          systick_calibrate = data & 0x00FFFFFF;
          break;

        default:
          break;
      }
      return;

    case 0xD0000000: //debug
#ifndef UNSAFE_OPTIMIZATIONS
      switch(addr & 0xFF)
      {
        case 0x00:
          statusMsg << "[" << Base::HEX8 << read_register(14) << "]["
                    << addr << "] " << data << endl;
          return;

        case 0x10:
          statusMsg << Base::HEX8 << data << endl;
          return;

        case 0x20:
          statusMsg << Base::HEX8 << data << endl;
          return;

        default:
          break;
      }
#endif
      return;

#ifndef UNSAFE_OPTIMIZATIONS
    case 0x40000000: //RAM
#else
    default:
#endif
      write16(addr+0, (data >>  0) & 0xFFFF);
      write16(addr+2, (data >> 16) & 0xFFFF);
      return;
  }
#ifndef UNSAFE_OPTIMIZATIONS
  fatalError("write32", addr, data, "abort");
#endif
}

#ifndef UNSAFE_OPTIMIZATIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Thumbulator::isProtected(uInt32 addr)
{
  if (addr < 0x40000000) return false;
  addr -= 0x40000000;

  switch (configuration) {
    case ConfigureFor::DPCplus:
      return (addr < 0x0c00) && (addr > 0x0028);

    case ConfigureFor::CDF:
      return  (addr < 0x0800) && (addr > 0x0028) && !((addr >= 0x06e0) && (addr < (0x0e60 + 284)));

    case ConfigureFor::CDF1:
      return  (addr < 0x0800) && (addr > 0x0028) && !((addr >= 0x00a0) && (addr < (0x00a0 + 284)));

    case ConfigureFor::CDFJ:
    case ConfigureFor::CDFJplus:
      return  (addr < 0x0800) && (addr > 0x0028) && !((addr >= 0x0098) && (addr < (0x0098 + 292)));

    case ConfigureFor::BUS:
      return  (addr < 0x06d8) && (addr > 0x0028);
  }

  return false;
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read16(uInt32 addr)
{
  uInt32 data;
#ifndef UNSAFE_OPTIMIZATIONS
  if((addr > 0x40007fff) && (addr < 0x50000000))
    fatalError("read16", addr, "abort - out of range");
  else if((addr > 0x0007ffff) && (addr < 0x10000000))
    fatalError("read16", addr, "abort - out of range");
  if(addr & 1)
    fatalError("read16", addr, "abort - misaligned");
#endif
#ifndef NO_THUMB_STATS
  ++_stats.reads;
#endif

  switch(addr & 0xF0000000)
  {
    case 0x00000000: //ROM
      addr &= ROMADDMASK;
      addr >>= 1;
      data = CONV_RAMROM(rom[addr]);
      DO_DBUG(statusMsg << "read16(" << Base::HEX8 << addr << ")=" << Base::HEX4 << data << endl);
      return data;

    case 0x40000000: //RAM
      addr &= RAMADDMASK;
      addr >>= 1;
      data = CONV_RAMROM(ram[addr]);
      DO_DBUG(statusMsg << "read16(" << Base::HEX8 << addr << ")=" << Base::HEX4 << data << endl);
      return data;

#ifndef UNSAFE_OPTIMIZATIONS
    case 0xE0000000: //MAMCR
      if(addr == 0xE01FC000)
#else
    default:
#endif
      {
        DO_DBUG(statusMsg << "read16(" << "MAMCR" << addr << ")=" << mamcr << " *");
        return mamcr;
      }
  }
#ifndef UNSAFE_OPTIMIZATIONS
  return fatalError("read16", addr, "abort");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read32(uInt32 addr)
{
#ifndef UNSAFE_OPTIMIZATIONS
  if(addr & 3)
    fatalError("read32", addr, "abort - misaligned");
#endif

  uInt32 data;
  switch(addr & 0xF0000000)
  {
    case 0x00000000: //ROM
    case 0x40000000: //RAM
      data = read16(addr+0);
      data |= (uInt32(read16(addr+2))) << 16;
      DO_DBUG(statusMsg << "read32(" << Base::HEX8 << addr << ")=" << Base::HEX8 << data << endl);
      return data;

#ifndef UNSAFE_OPTIMIZATIONS
    case 0xE0000000:
#else
    default:
#endif
    {
      switch(addr)
      {
        case 0xE0008004:  // T1TCR - Timer 1 Control Register
          data = T1TCR;
          return data;

        case 0xE0008008:  // T1TC - Timer 1 Counter
          data = T1TC;
          return data;

        case 0xE000E010:
          data = systick_ctrl;
          systick_ctrl &= (~0x00010000);
          return data;

        case 0xE000E014:
          data = systick_reload;
          return data;

        case 0xE000E018:
          data = systick_count;
          return data;

#ifndef UNSAFE_OPTIMIZATIONS
        case 0xE000E01C:
#else
        default:
#endif
          data = systick_calibrate;
          return data;
      }
    }
  }
#ifndef UNSAFE_OPTIMIZATIONS
  return fatalError("read32", addr, "abort");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read_register(uInt32 reg)
{
  reg &= 0xF;

  uInt32 data = reg_norm[reg];
  DO_DBUG(statusMsg << "read_register(" << dec << reg << ")=" << Base::HEX8 << data << endl);
#ifndef UNSAFE_OPTIMIZATIONS
  if(reg == 15)
  {
    if(data & 1)
    {
      DO_DBUG(statusMsg << "pc has lsbit set 0x" << Base::HEX8 << data << endl);
    }
    data &= ~1;
  }
#endif
  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write_register(uInt32 reg, uInt32 data)
{
  reg &= 0xF;

  DO_DBUG(statusMsg << "write_register(" << dec << reg << "," << Base::HEX8 << data << ")" << endl);
//#ifndef UNSAFE_OPTIMIZATIONS // this fails when combined with read_register UNSAFE_OPTIMIZATIONS
  if(reg == 15) data &= ~1;
//#endif
  reg_norm[reg] = data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_zflag(uInt32 x)
{
  if(x == 0) cpsr |= CPSR_Z;  else cpsr &= ~CPSR_Z;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_nflag(uInt32 x)
{
  if(x & 0x80000000) cpsr |= CPSR_N;  else cpsr &= ~CPSR_N;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_cflag(uInt32 a, uInt32 b, uInt32 c)
{
  uInt32 rc;

  rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
  rc = (rc >> 31) + (a >> 31) + (b >> 31);      //carry out
  if(rc & 2)
    cpsr |= CPSR_C;
  else
    cpsr &= ~CPSR_C;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_vflag(uInt32 a, uInt32 b, uInt32 c)
{
  uInt32 rc, rd;

  rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
  rc >>= 31; //carry in in lsbit
  rd = (rc & 1) + ((a >> 31) & 1) + ((b >> 31) & 1); //carry out
  rd >>= 1; //carry out in lsbit
  rc = (rc^rd) & 1; //if carry in != carry out then signed overflow
  if(rc)
    cpsr |= CPSR_V;
  else
    cpsr &= ~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_cflag_bit(uInt32 x)
{
  if(x) cpsr |= CPSR_C;  else cpsr &= ~CPSR_C;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_vflag_bit(uInt32 x)
{
  if(x) cpsr |= CPSR_V;  else cpsr &= ~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Op Thumbulator::decodeInstructionWord(uint16_t inst) {
  //ADC
  if((inst & 0xFFC0) == 0x4140) return Op::adc;

  //ADD(1) small immediate two registers
  if((inst & 0xFE00) == 0x1C00 && (inst >> 6) & 0x7) return Op::add1;

  //ADD(2) big immediate one register
  if((inst & 0xF800) == 0x3000) return Op::add2;

  //ADD(3) three registers
  if((inst & 0xFE00) == 0x1800) return Op::add3;

  //ADD(4) two registers one or both high no flags
  if((inst & 0xFF00) == 0x4400) return Op::add4;

  //ADD(5) rd = pc plus immediate
  if((inst & 0xF800) == 0xA000) return Op::add5;

  //ADD(6) rd = sp plus immediate
  if((inst & 0xF800) == 0xA800) return Op::add6;

  //ADD(7) sp plus immediate
  if((inst & 0xFF80) == 0xB000) return Op::add7;

  //AND
  if((inst & 0xFFC0) == 0x4000) return Op::and_;

  //ASR(1) two register immediate
  if((inst & 0xF800) == 0x1000) return Op::asr1;

  //ASR(2) two register
  if((inst & 0xFFC0) == 0x4100) return Op::asr2;

  //B(1) conditional branch
  if((inst & 0xF000) == 0xD000) return Op::b1;

  //B(2) unconditional branch
  if((inst & 0xF800) == 0xE000) return Op::b2;

  //BIC
  if((inst & 0xFFC0) == 0x4380) return Op::bic;

  //BKPT
  if((inst & 0xFF00) == 0xBE00) return Op::bkpt;

  //BL/BLX(1)
  if((inst & 0xE000) == 0xE000) return Op::blx1;

  //BLX(2)
  if((inst & 0xFF87) == 0x4780) return Op::blx2;

  //BX
  if((inst & 0xFF87) == 0x4700) return Op::bx;

  //CMN
  if((inst & 0xFFC0) == 0x42C0) return Op::cmn;

  //CMP(1) compare immediate
  if((inst & 0xF800) == 0x2800) return Op::cmp1;

  //CMP(2) compare register
  if((inst & 0xFFC0) == 0x4280) return Op::cmp2;

  //CMP(3) compare high register
  if((inst & 0xFF00) == 0x4500) return Op::cmp3;

  //CPS
  if((inst & 0xFFE8) == 0xB660) return Op::cps;

  //CPY copy high register
  if((inst & 0xFFC0) == 0x4600) return Op::cpy;

  //EOR
  if((inst & 0xFFC0) == 0x4040) return Op::eor;

  //LDMIA
  if((inst & 0xF800) == 0xC800) return Op::ldmia;

  //LDR(1) two register immediate
  if((inst & 0xF800) == 0x6800) return Op::ldr1;

  //LDR(2) three register
  if((inst & 0xFE00) == 0x5800) return Op::ldr2;

  //LDR(3)
  if((inst & 0xF800) == 0x4800) return Op::ldr3;

  //LDR(4)
  if((inst & 0xF800) == 0x9800) return Op::ldr4;

  //LDRB(1)
  if((inst & 0xF800) == 0x7800) return Op::ldrb1;

  //LDRB(2)
  if((inst & 0xFE00) == 0x5C00) return Op::ldrb2;

  //LDRH(1)
  if((inst & 0xF800) == 0x8800) return Op::ldrh1;

  //LDRH(2)
  if((inst & 0xFE00) == 0x5A00) return Op::ldrh2;

  //LDRSB
  if((inst & 0xFE00) == 0x5600) return Op::ldrsb;

  //LDRSH
  if((inst & 0xFE00) == 0x5E00) return Op::ldrsh;

  //LSL(1)
  if((inst & 0xF800) == 0x0000) return Op::lsl1;

  //LSL(2) two register
  if((inst & 0xFFC0) == 0x4080) return Op::lsl2;

  //LSR(1) two register immediate
  if((inst & 0xF800) == 0x0800) return Op::lsr1;

  //LSR(2) two register
  if((inst & 0xFFC0) == 0x40C0) return Op::lsr2;

  //MOV(1) immediate
  if((inst & 0xF800) == 0x2000) return Op::mov1;

  //MOV(2) two low registers
  if((inst & 0xFFC0) == 0x1C00) return Op::mov2;

  //MOV(3)
  if((inst & 0xFF00) == 0x4600) return Op::mov3;

  //MUL
  if((inst & 0xFFC0) == 0x4340) return Op::mul;

  //MVN
  if((inst & 0xFFC0) == 0x43C0) return Op::mvn;

  //NEG
  if((inst & 0xFFC0) == 0x4240) return Op::neg;

  //ORR
  if((inst & 0xFFC0) == 0x4300) return Op::orr;

  //POP
  if((inst & 0xFE00) == 0xBC00) return Op::pop;

  //PUSH
  if((inst & 0xFE00) == 0xB400) return Op::push;

  //REV
  if((inst & 0xFFC0) == 0xBA00) return Op::rev;

  //REV16
  if((inst & 0xFFC0) == 0xBA40) return Op::rev16;

  //REVSH
  if((inst & 0xFFC0) == 0xBAC0) return Op::revsh;

  //ROR
  if((inst & 0xFFC0) == 0x41C0) return Op::ror;

  //SBC
  if((inst & 0xFFC0) == 0x4180) return Op::sbc;

  //SETEND
  if((inst & 0xFFF7) == 0xB650) return Op::setend;

  //STMIA
  if((inst & 0xF800) == 0xC000) return Op::stmia;

  //STR(1)
  if((inst & 0xF800) == 0x6000) return Op::str1;

  //STR(2)
  if((inst & 0xFE00) == 0x5000) return Op::str2;

  //STR(3)
  if((inst & 0xF800) == 0x9000) return Op::str3;

  //STRB(1)
  if((inst & 0xF800) == 0x7000) return Op::strb1;

  //STRB(2)
  if((inst & 0xFE00) == 0x5400) return Op::strb2;

  //STRH(1)
  if((inst & 0xF800) == 0x8000) return Op::strh1;

  //STRH(2)
  if((inst & 0xFE00) == 0x5200) return Op::strh2;

  //SUB(1)
  if((inst & 0xFE00) == 0x1E00) return Op::sub1;

  //SUB(2)
  if((inst & 0xF800) == 0x3800) return Op::sub2;

  //SUB(3)
  if((inst & 0xFE00) == 0x1A00) return Op::sub3;

  //SUB(4)
  if((inst & 0xFF80) == 0xB080) return Op::sub4;

  //SWI
  if((inst & 0xFF00) == 0xDF00) return Op::swi;

  //SXTB
  if((inst & 0xFFC0) == 0xB240) return Op::sxtb;

  //SXTH
  if((inst & 0xFFC0) == 0xB200) return Op::sxth;

  //TST
  if((inst & 0xFFC0) == 0x4200) return Op::tst;

  //UXTB
  if((inst & 0xFFC0) == 0xB2C0) return Op::uxtb;

  //UXTH
  if((inst & 0xFFC0) == 0xB280) return Op::uxth;

  return Op::invalid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::execute()
{
  uInt32 pc, sp, inst, ra, rb, rc, rm, rd, rn, rs, op;

  pc = read_register(15);

  uInt32 instructionPtr = pc - 2;
  inst = fetch16(instructionPtr);

  pc += 2;
  write_register(15, pc);
  DO_DISS(statusMsg << Base::HEX8 << (pc-5) << ": " << Base::HEX4 << inst << " ");

#ifndef UNSAFE_OPTIMIZATIONS
  ++instructions;
#endif

  Op decodedOp;
#ifndef UNSAFE_OPTIMIZATIONS
  if ((instructionPtr & 0xF0000000) == 0 && instructionPtr < romSize)
    decodedOp = decodedRom[instructionPtr >> 1];
  else
    decodedOp = decodeInstructionWord(inst);
#else
  decodedOp = decodedRom[(instructionPtr & ROMADDMASK) >> 1];
#endif

  switch (decodedOp) {
    //ADC
    case Op::adc: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      DO_DISS(statusMsg << "adc r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra + rb;
      if(cpsr & CPSR_C)
        ++rc;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      if(cpsr & CPSR_C) { do_cflag(ra, rb, 1); do_vflag(ra, rb, 1); }
      else              { do_cflag(ra, rb, 0); do_vflag(ra, rb, 0); }
      return 0;
    }

    //ADD(1) small immediate two registers
    case Op::add1: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rb = (inst >> 6) & 0x7;
      if(rb)
      {
        DO_DISS(statusMsg << "adds r" << dec << rd << ",r" << dec << rn << ","
                          << "#0x" << Base::HEX2 << rb << endl);
        ra = read_register(rn);
        rc = ra + rb;
        //fprintf(stderr,"0x%08X = 0x%08X + 0x%08X\n",rc,ra,rb);
        write_register(rd, rc);
        do_nflag(rc);
        do_zflag(rc);
        do_cflag(ra, rb, 0);
        do_vflag(ra, rb, 0);
        return 0;
      }
      else
      {
        //this is a mov
      }

      break;
    }

    //ADD(2) big immediate one register
    case Op::add2: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      DO_DISS(statusMsg << "adds r" << dec << rd << ",#0x" << Base::HEX2 << rb << endl);
      ra = read_register(rd);
      rc = ra + rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, rb, 0);
      do_vflag(ra, rb, 0);
      return 0;
    }

    //ADD(3) three registers
    case Op::add3: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "adds r" << dec << rd << ",r" << dec << rn << ",r" << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra + rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, rb, 0);
      do_vflag(ra, rb, 0);
      return 0;
    }

    //ADD(4) two registers one or both high no flags
    case Op::add4: {
      if((inst >> 6) & 3)
      {
        //UNPREDICTABLE
      }
      rd  = (inst >> 0) & 0x7;
      rd |= (inst >> 4) & 0x8;
      rm  = (inst >> 3) & 0xF;
      DO_DISS(statusMsg << "add r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra + rb;
      if(rd == 15)
      {
#ifndef UNSAFE_OPTIMIZATIONS
        if((rc & 1) == 0)
          fatalError("add pc", pc, rc, " produced an arm address");
#endif
        //rc &= ~1; //write_register may do this as well
        rc += 2;  //The program counter is special
      }
      //fprintf(stderr,"0x%08X = 0x%08X + 0x%08X\n",rc,ra,rb);
      write_register(rd, rc);
      return 0;
    }

    //ADD(5) rd = pc plus immediate
    case Op::add5: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      rb <<= 2;
      DO_DISS(statusMsg << "add r" << dec << rd << ",PC,#0x" << Base::HEX2 << rb << endl);
      ra = read_register(15);
      rc = (ra & (~3U)) + rb;
      write_register(rd, rc);
      return 0;
    }

    //ADD(6) rd = sp plus immediate
    case Op::add6: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x7;
      rb <<= 2;
      DO_DISS(statusMsg << "add r" << dec << rd << ",SP,#0x" << Base::HEX2 << rb << endl);
      ra = read_register(13);
      rc = ra + rb;
      write_register(rd, rc);
      return 0;
    }

    //ADD(7) sp plus immediate
    case Op::add7: {
      rb = (inst >> 0) & 0x7F;
      rb <<= 2;
      DO_DISS(statusMsg << "add SP,#0x" << Base::HEX2 << rb << endl);
      ra = read_register(13);
      rc = ra + rb;
      write_register(13, rc);
      return 0;
    }

    //AND
    case Op::and_: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "ands r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra & rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //ASR(1) two register immediate
    case Op::asr1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS(statusMsg << "asrs r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << endl);
      rc = read_register(rm);
      if(rb == 0)
      {
        if(rc & 0x80000000)
        {
          do_cflag_bit(1);
          rc = ~0U;
        }
        else
        {
          do_cflag_bit(0);
          rc = 0;
        }
      }
      else
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        ra = rc & 0x80000000;
        rc >>= rb;
        if(ra) //asr, sign is shifted in
          rc |= (~0U) << (32-rb);
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //ASR(2) two register
    case Op::asr2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS(statusMsg << "asrs r" << dec << rd << ",r" << dec << rs << endl);
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        ra = rc & 0x80000000;
        rc >>= rb;
        if(ra) //asr, sign is shifted in
        {
          rc |= (~0U) << (32-rb);
        }
      }
      else
      {
        if(rc & 0x80000000)
        {
          do_cflag_bit(1);
          rc = (~0U);
        }
        else
        {
          do_cflag_bit(0);
          rc = 0;
        }
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //B(1) conditional branch
    case Op::b1: {
      rb = (inst >> 0) & 0xFF;
      if(rb & 0x80)
        rb |= (~0U) << 8;
      op=(inst >> 8) & 0xF;
      rb <<= 1;
      rb += pc;
      rb += 2;
      switch(op)
      {
        case 0x0: //b eq  z set
          DO_DISS(statusMsg << "beq 0x" << Base::HEX8 << (rb-3) << endl);
          if(cpsr & CPSR_Z)
            write_register(15, rb);
          return 0;

        case 0x1: //b ne  z clear
          DO_DISS(statusMsg << "bne 0x" << Base::HEX8 << (rb-3) << endl);
          if(!(cpsr & CPSR_Z))
            write_register(15, rb);
          return 0;

        case 0x2: //b cs c set
          DO_DISS(statusMsg << "bcs 0x" << Base::HEX8 << (rb-3) << endl);
          if(cpsr & CPSR_C)
            write_register(15, rb);
          return 0;

        case 0x3: //b cc c clear
          DO_DISS(statusMsg << "bcc 0x" << Base::HEX8 << (rb-3) << endl);
          if(!(cpsr & CPSR_C))
            write_register(15, rb);
          return 0;

        case 0x4: //b mi n set
          DO_DISS(statusMsg << "bmi 0x" << Base::HEX8 << (rb-3) << endl);
          if(cpsr & CPSR_N)
            write_register(15, rb);
          return 0;

        case 0x5: //b pl n clear
          DO_DISS(statusMsg << "bpl 0x" << Base::HEX8 << (rb-3) << endl);
          if(!(cpsr & CPSR_N))
            write_register(15, rb);
          return 0;

        case 0x6: //b vs v set
          DO_DISS(statusMsg << "bvs 0x" << Base::HEX8 << (rb-3) << endl);
          if(cpsr & CPSR_V)
            write_register(15,rb);
          return 0;

        case 0x7: //b vc v clear
          DO_DISS(statusMsg << "bvc 0x" << Base::HEX8 << (rb-3) << endl);
          if(!(cpsr & CPSR_V))
            write_register(15, rb);
          return 0;

        case 0x8: //b hi c set z clear
          DO_DISS(statusMsg << "bhi 0x" << Base::HEX8 << (rb-3) << endl);
          if((cpsr & CPSR_C) && (!(cpsr & CPSR_Z)))
            write_register(15, rb);
          return 0;

        case 0x9: //b ls c clear or z set
          DO_DISS(statusMsg << "bls 0x" << Base::HEX8 << (rb-3) << endl);
          if((cpsr & CPSR_Z) || (!(cpsr & CPSR_C)))
            write_register(15, rb);
          return 0;

        case 0xA: //b ge N == V
          DO_DISS(statusMsg << "bge 0x" << Base::HEX8 << (rb-3) << endl);
          if(((cpsr & CPSR_N) && (cpsr & CPSR_V)) ||
             ((!(cpsr & CPSR_N)) && (!(cpsr & CPSR_V))))
            write_register(15, rb);
          return 0;

        case 0xB: //b lt N != V
          DO_DISS(statusMsg << "blt 0x" << Base::HEX8 << (rb-3) << endl);
          if((!(cpsr & CPSR_N) && (cpsr & CPSR_V)) ||
            (((cpsr & CPSR_N)) && !(cpsr & CPSR_V)))
            write_register(15, rb);
          return 0;

        case 0xC: //b gt Z==0 and N == V
          DO_DISS(statusMsg << "bgt 0x" << Base::HEX8 << (rb-3) << endl);
          if(!(cpsr & CPSR_Z))
          {
            if(((cpsr & CPSR_N) && (cpsr & CPSR_V)) ||
               ((!(cpsr & CPSR_N)) && (!(cpsr & CPSR_V))))
              write_register(15, rb);
          }
          return 0;

        case 0xD: //b le Z==1 or N != V
          DO_DISS(statusMsg << "ble 0x" << Base::HEX8 << (rb-3) << endl);
          if((cpsr & CPSR_Z) ||
            (!(cpsr & CPSR_N) && (cpsr & CPSR_V)) ||
            (((cpsr & CPSR_N)) && !(cpsr & CPSR_V)))
              write_register(15, rb);
          return 0;

        case 0xE:
          //undefined instruction
          break;

        case 0xF:
          //swi
          break;

        default:
          break;
      }
      break;
    }

    //B(2) unconditional branch
    case Op::b2: {
      rb = (inst >> 0) & 0x7FF;
      if(rb & (1 << 10))
        rb |= (~0U) << 11;
      rb <<= 1;
      rb += pc;
      rb += 2;
      DO_DISS(statusMsg << "B 0x" << Base::HEX8 << (rb-3) << endl);
      write_register(15, rb);
      return 0;
    }

    //BIC
    case Op::bic: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "bics r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra & (~rb);
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

#ifndef UNSAFE_OPTIMIZATIONS
    //BKPT
    case Op::bkpt: {
      rb = (inst >> 0) & 0xFF;
      statusMsg << "bkpt 0x" << Base::HEX2 << rb << endl;
      return 1;
    }
#endif

    //BL/BLX(1)
    case Op::blx1: {
      if((inst & 0x1800) == 0x1000) //H=b10
      {
        DO_DISS(statusMsg << endl);
        rb = inst & ((1 << 11) - 1);
        if(rb & 1<<10) rb |= (~((1 << 11) - 1)); //sign extend
        rb <<= 12;
        rb += pc;
        write_register(14, rb);
        return 0;
      }
      else if((inst & 0x1800) == 0x1800) //H=b11
      {
        //branch to thumb
        rb = read_register(14);
        rb += (inst & ((1 << 11) - 1)) << 1;
        rb += 2;
        DO_DISS(statusMsg << "bl 0x" << Base::HEX8 << (rb-3) << endl);
        write_register(14, (pc-2) | 1);
        write_register(15, rb);
        return 0;
      }
      else if((inst & 0x1800) == 0x0800) //H=b01
      {
        //fprintf(stderr,"cannot branch to arm 0x%08X 0x%04X\n",pc,inst);
        // fxq: this should exit the code without having to detect it
        rb = read_register(14);
        rb += (inst & ((1 << 11) - 1)) << 1;
        rb &= 0xFFFFFFFC;
        rb += 2;
        DO_DISS(statusMsg << "bl 0x" << Base::HEX8 << (rb-3) << endl);
        write_register(14, (pc-2) | 1);
        write_register(15, rb);
        return 0;
      }

      break;
    }

    //BLX(2)
    case Op::blx2: {
      rm = (inst >> 3) & 0xF;
      DO_DISS(statusMsg << "blx r" << dec << rm << endl);
      rc = read_register(rm);
      //fprintf(stderr,"blx r%u 0x%X 0x%X\n",rm,rc,pc);
      rc += 2;
      if(rc & 1)
      {
        write_register(14, (pc-2) | 1);
        //rc &= ~1;
        write_register(15, rc);
        return 0;
      }
      else
      {
        //fprintf(stderr,"cannot branch to arm 0x%08X 0x%04X\n",pc,inst);
        // fxq: this could serve as exit code
        return 1;
      }
    }

    //BX
    case Op::bx: {
      rm = (inst >> 3) & 0xF;
      DO_DISS(statusMsg << "bx r" << dec << rm << endl);
      rc = read_register(rm);
      rc += 2;
      //fprintf(stderr,"bx r%u 0x%X 0x%X\n",rm,rc,pc);
      if(rc & 1)
      {
        // branch to odd address denotes 16 bit ARM code
        //rc &= ~1;
        write_register(15, rc);
        return 0;
      }
      else
      {
        // branch to even address denotes 32 bit ARM code, which the Thumbulator
        // class does not support. So capture relavent information and hand it
        // off to the Cartridge class for it to handle.

        bool handled = false;

        switch(configuration)
        {
          case ConfigureFor::BUS:
            // this subroutine interface is used in the BUS driver,
            // it starts at address 0x000006d8
            // _SetNote:
            //   ldr     r4, =NoteStore
            //   bx      r4   // bx instruction at 0x000006da
            // _ResetWave:
            //   ldr     r4, =ResetWaveStore
            //   bx      r4   // bx instruction at 0x000006de
            // _GetWavePtr:
            //   ldr     r4, =WavePtrFetch
            //   bx      r4   // bx instruction at 0x000006e2
            // _SetWaveSize:
            //   ldr     r4, =WaveSizeStore
            //   bx      r4   // bx instruction at 0x000006e6

            // address to test for is + 4 due to pipelining

  #define BUS_SetNote     (0x000006da + 4)
  #define BUS_ResetWave   (0x000006de + 4)
  #define BUS_GetWavePtr  (0x000006e2 + 4)
  #define BUS_SetWaveSize (0x000006e6 + 4)

            if      (pc == BUS_SetNote)
            {
              myCartridge->thumbCallback(0, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == BUS_ResetWave)
            {
              myCartridge->thumbCallback(1, read_register(2), 0);
              handled = true;
            }
            else if (pc == BUS_GetWavePtr)
            {
              write_register(2, myCartridge->thumbCallback(2, read_register(2), 0));
              handled = true;
            }
            else if (pc == BUS_SetWaveSize)
            {
              myCartridge->thumbCallback(3, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == 0x0000083a)
            {
              // exiting Custom ARM code, returning to BUS Driver control
            }
            else
            {
  #if 0  // uncomment this for testing
              uInt32 r0 = read_register(0);
              uInt32 r1 = read_register(1);
              uInt32 r2 = read_register(2);
              uInt32 r3 = read_register(3);
              uInt32 r4 = read_register(4);
  #endif
              myCartridge->thumbCallback(255, 0, 0);
            }

            break;

          case ConfigureFor::CDF:
            // this subroutine interface is used in the CDF driver,
            // it starts at address 0x000006e0
            // _SetNote:
            //   ldr     r4, =NoteStore
            //   bx      r4   // bx instruction at 0x000006e2
            // _ResetWave:
            //   ldr     r4, =ResetWaveStore
            //   bx      r4   // bx instruction at 0x000006e6
            // _GetWavePtr:
            //   ldr     r4, =WavePtrFetch
            //   bx      r4   // bx instruction at 0x000006ea
            // _SetWaveSize:
            //   ldr     r4, =WaveSizeStore
            //   bx      r4   // bx instruction at 0x000006ee

            // address to test for is + 4 due to pipelining

          #define CDF_SetNote     (0x000006e2 + 4)
          #define CDF_ResetWave   (0x000006e6 + 4)
          #define CDF_GetWavePtr  (0x000006ea + 4)
          #define CDF_SetWaveSize (0x000006ee + 4)

            if      (pc == CDF_SetNote)
            {
              myCartridge->thumbCallback(0, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == CDF_ResetWave)
            {
              myCartridge->thumbCallback(1, read_register(2), 0);
              handled = true;
            }
            else if (pc == CDF_GetWavePtr)
            {
              write_register(2, myCartridge->thumbCallback(2, read_register(2), 0));
              handled = true;
            }
            else if (pc == CDF_SetWaveSize)
            {
              myCartridge->thumbCallback(3, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == 0x0000083a)
            {
              // exiting Custom ARM code, returning to BUS Driver control
            }
            else
            {
            #if 0  // uncomment this for testing
              uInt32 r0 = read_register(0);
              uInt32 r1 = read_register(1);
              uInt32 r2 = read_register(2);
              uInt32 r3 = read_register(3);
              uInt32 r4 = read_register(4);
            #endif
              myCartridge->thumbCallback(255, 0, 0);
            }

            break;

          case ConfigureFor::CDF1:
          case ConfigureFor::CDFJ:
          case ConfigureFor::CDFJplus:
            // this subroutine interface is used in the CDF driver,
            // it starts at address 0x00000750
            // _SetNote:
            //   ldr     r4, =NoteStore
            //   bx      r4   // bx instruction at 0x000006e2
            // _ResetWave:
            //   ldr     r4, =ResetWaveStore
            //   bx      r4   // bx instruction at 0x000006e6
            // _GetWavePtr:
            //   ldr     r4, =WavePtrFetch
            //   bx      r4   // bx instruction at 0x000006ea
            // _SetWaveSize:
            //   ldr     r4, =WaveSizeStore
            //   bx      r4   // bx instruction at 0x000006ee

            // address to test for is + 4 due to pipelining

  #define CDF1_SetNote     (0x00000752 + 4)
  #define CDF1_ResetWave   (0x00000756 + 4)
  #define CDF1_GetWavePtr  (0x0000075a + 4)
  #define CDF1_SetWaveSize (0x0000075e + 4)

            if      (pc == CDF1_SetNote)
            {
              myCartridge->thumbCallback(0, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == CDF1_ResetWave)
            {
              myCartridge->thumbCallback(1, read_register(2), 0);
              handled = true;
            }
            else if (pc == CDF1_GetWavePtr)
            {
              write_register(2, myCartridge->thumbCallback(2, read_register(2), 0));
              handled = true;
            }
            else if (pc == CDF1_SetWaveSize)
            {
              myCartridge->thumbCallback(3, read_register(2), read_register(3));
              handled = true;
            }
            else if (pc == 0x0000083a)
            {
              // exiting Custom ARM code, returning to BUS Driver control
            }
            else
            {
  #if 0  // uncomment this for testing
              uInt32 r0 = read_register(0);
              uInt32 r1 = read_register(1);
              uInt32 r2 = read_register(2);
              uInt32 r3 = read_register(3);
              uInt32 r4 = read_register(4);
  #endif
              myCartridge->thumbCallback(255, 0, 0);
            }

            break;

          case ConfigureFor::DPCplus:
            // no 32-bit subroutines in DPC+
            break;
        }

        if (handled)
        {
          rc = read_register(14); // lr
          rc += 2;
          //rc &= ~1;
          write_register(15, rc);
          return 0;
        }

        return 1;
      }
    }

    //CMN
    case Op::cmn: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "cmns r" << dec << rn << ",r" << dec << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra + rb;
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, rb, 0);
      do_vflag(ra, rb, 0);
      return 0;
    }

    //CMP(1) compare immediate
    case Op::cmp1: {
      rb = (inst >> 0) & 0xFF;
      rn = (inst >> 8) & 0x07;
      DO_DISS(statusMsg << "cmp r" << dec << rn << ",#0x" << Base::HEX2 << rb << endl);
      ra = read_register(rn);
      rc = ra - rb;
      //fprintf(stderr,"0x%08X 0x%08X\n",ra,rb);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

    //CMP(2) compare register
    case Op::cmp2: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "cmps r" << dec << rn << ",r" << dec << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      //fprintf(stderr,"0x%08X 0x%08X\n",ra,rb);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

    //CMP(3) compare high register
    case Op::cmp3: {
      if(((inst >> 6) & 3) == 0x0)
      {
        //UNPREDICTABLE
      }
      rn = (inst >> 0) & 0x7;
      rn |= (inst >> 4) & 0x8;
      if(rn == 0xF)
      {
        //UNPREDICTABLE
      }
      rm = (inst >> 3) & 0xF;
      DO_DISS(statusMsg << "cmps r" << dec << rn << ",r" << dec << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

#ifndef UNSAFE_OPTIMIZATIONS
    //CPS
    case Op::cps: {
      DO_DISS(statusMsg << "cps TODO" << endl);
      return 1;
    }
#endif

    //CPY copy high register
    case Op::cpy: {
      //same as mov except you can use both low registers
      //going to let mov handle high registers
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "cpy r" << dec << rd << ",r" << dec << rm << endl);
      rc = read_register(rm);
      write_register(rd, rc);
      return 0;
    }

    //EOR
    case Op::eor: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "eors r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra ^ rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //LDMIA
    case Op::ldmia: {
      rn = (inst >> 8) & 0x7;
    #if defined(THUMB_DISS)
      statusMsg << "ldmia r" << dec << rn << "!,{";
      for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
      {
        if(inst&rb)
        {
          if(rc) statusMsg << ",";
          statusMsg << "r" << dec << ra;
          rc++;
        }
      }
      statusMsg << "}" << endl;
    #endif
      sp = read_register(rn);
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          write_register(ra, read32(sp));
          sp += 4;
        }
      }
      //there is a write back exception.
      if((inst & (1 << rn)) == 0)
        write_register(rn, sp);

      return 0;
    }

    //LDR(1) two register immediate
    case Op::ldr1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 2;
      DO_DISS(statusMsg << "ldr r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(rn) + rb;
      rc = read32(rb);
      write_register(rd, rc);
      return 0;
    }

    //LDR(2) three register
    case Op::ldr2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "ldr r" << dec << rd << ",[r" << dec << rn << ",r" << dec << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read32(rb);
      write_register(rd, rc);
      return 0;
    }

    //LDR(3)
    case Op::ldr3: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS(statusMsg << "ldr r" << dec << rd << ",[PC+#0x" << Base::HEX2 << rb << "] ");
      ra = read_register(15);
      ra &= ~3;
      rb += ra;
      DO_DISS(statusMsg << ";@ 0x" << Base::HEX2 << rb << endl);
      rc = read32(rb);
      write_register(rd, rc);
      return 0;
    }

    //LDR(4)
    case Op::ldr4: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS(statusMsg << "ldr r" << dec << rd << ",[SP+#0x" << Base::HEX2 << rb << "]" << endl);
      ra = read_register(13);
      //ra&=~3;
      rb += ra;
      rc = read32(rb);
      write_register(rd, rc);
      return 0;
    }

    //LDRB(1)
    case Op::ldrb1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS(statusMsg << "ldrb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(rn) + rb;
#ifndef UNSAFE_OPTIMIZATIONS
      rc = read16(rb & (~1U));
#else
      rc = read16(rb);
#endif
      if(rb & 1)
      {
        rc >>= 8;
      }
      else
      {
      }
      write_register(rd, rc & 0xFF);
      return 0;
    }

    //LDRB(2)
    case Op::ldrb2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "ldrb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
#ifndef UNSAFE_OPTIMIZATIONS
      rc = read16(rb & (~1U));
#else
      rc = read16(rb);
#endif
      if(rb & 1)
      {
        rc >>= 8;
      }
      write_register(rd, rc & 0xFF);
      return 0;
    }

    //LDRH(1)
    case Op::ldrh1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 1;
      DO_DISS(statusMsg << "ldrh r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(rn) + rb;
      rc = read16(rb);
      write_register(rd, rc & 0xFFFF);
      return 0;
    }

    //LDRH(2)
    case Op::ldrh2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "ldrh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read16(rb);
      write_register(rd, rc & 0xFFFF);
      return 0;
    }

    //LDRSB
    case Op::ldrsb: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "ldrsb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
#ifndef UNSAFE_OPTIMIZATIONS
      rc = read16(rb & (~1U));
#else
      rc = read16(rb);
#endif
      if(rb & 1)
      {
        rc >>= 8;
      }
      rc &= 0xFF;
      if(rc & 0x80)
        rc |= ((~0U) << 8);
      write_register(rd, rc);
      return 0;
    }

    //LDRSH
    case Op::ldrsh: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "ldrsh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read16(rb);
      rc &= 0xFFFF;
      if(rc & 0x8000)
        rc |= ((~0U) << 16);
      write_register(rd, rc);
      return 0;
    }

    //LSL(1)
    case Op::lsl1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS(statusMsg << "lsls r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << endl);
      rc = read_register(rm);
      if(rb == 0)
      {
        //if immed_5 == 0
        //C unaffected
        //result not shifted
      }
      else
      {
        //else immed_5 > 0
        do_cflag_bit(rc & (1 << (32-rb)));
        rc <<= rb;
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //LSL(2) two register
    case Op::lsl2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS(statusMsg << "lsls r" << dec << rd << ",r" << dec << rs << endl);
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (32-rb)));
        rc <<= rb;
      }
      else if(rb == 32)
      {
        do_cflag_bit(rc & 1);
        rc = 0;
      }
      else
      {
        do_cflag_bit(0);
        rc = 0;
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //LSR(1) two register immediate
    case Op::lsr1: {
      rd = (inst >> 0) & 0x07;
      rm = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS(statusMsg << "lsrs r" << dec << rd << ",r" << dec << rm << ",#0x" << Base::HEX2 << rb << endl);
      rc = read_register(rm);
      if(rb == 0)
      {
        do_cflag_bit(rc & 0x80000000);
        rc = 0;
      }
      else
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        rc >>= rb;
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //LSR(2) two register
    case Op::lsr2: {
      rd = (inst >> 0) & 0x07;
      rs = (inst >> 3) & 0x07;
      DO_DISS(statusMsg << "lsrs r" << dec << rd << ",r" << dec << rs << endl);
      rc = read_register(rd);
      rb = read_register(rs);
      rb &= 0xFF;
      if(rb == 0)
      {
      }
      else if(rb < 32)
      {
        do_cflag_bit(rc & (1 << (rb-1)));
        rc >>= rb;
      }
      else if(rb == 32)
      {
        do_cflag_bit(rc & 0x80000000);
        rc = 0;
      }
      else
      {
        do_cflag_bit(0);
        rc = 0;
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //MOV(1) immediate
    case Op::mov1: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      DO_DISS(statusMsg << "movs r" << dec << rd << ",#0x" << Base::HEX2 << rb << endl);
      write_register(rd, rb);
      do_nflag(rb);
      do_zflag(rb);
      return 0;
    }

    //MOV(2) two low registers
    case Op::mov2: {
      rd = (inst >> 0) & 7;
      rn = (inst >> 3) & 7;
      DO_DISS(statusMsg << "movs r" << dec << rd << ",r" << dec << rn << endl);
      rc = read_register(rn);
      //fprintf(stderr,"0x%08X\n",rc);
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag_bit(0);
      do_vflag_bit(0);
      return 0;
    }

    //MOV(3)
    case Op::mov3: {
      rd  = (inst >> 0) & 0x7;
      rd |= (inst >> 4) & 0x8;
      rm  = (inst >> 3) & 0xF;
      DO_DISS(statusMsg << "mov r" << dec << rd << ",r" << dec << rm << endl);
      rc = read_register(rm);
      if((rd == 14) && (rm == 15))
      {
        //printf("mov lr,pc warning 0x%08X\n",pc-2);
        //rc|=1;
      }
      if(rd == 15)
      {
        //rc &= ~1; //write_register may do this as well
        rc += 2;  //The program counter is special
      }
      write_register(rd, rc);
      return 0;
    }

    //MUL
    case Op::mul: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "muls r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra * rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //MVN
    case Op::mvn: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "mvns r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = (~ra);
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //NEG
    case Op::neg: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "negs r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = 0 - ra;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(0, ~ra, 1);
      do_vflag(0, ~ra, 1);
      return 0;
    }

    //ORR
    case Op::orr: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "orrs r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra | rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //POP
    case Op::pop: {
    #if defined(THUMB_DISS)
      statusMsg << "pop {";
      for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
      {
        if(inst&rb)
        {
          if(rc) statusMsg << ",";
          statusMsg << "r" << dec << ra;
          rc++;
        }
      }
      if(inst&0x100)
      {
        if(rc) statusMsg << ",";
        statusMsg << "pc";
      }
      statusMsg << "}" << endl;
    #endif

      sp = read_register(13);
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          write_register(ra, read32(sp));
          sp += 4;
        }
      }
      if(inst & 0x100)
      {
        rc = read32(sp);
        rc += 2;
        write_register(15, rc);
        sp += 4;
      }
      write_register(13, sp);
      return 0;
    }

    //PUSH
    case Op::push: {
    #if defined(THUMB_DISS)
      statusMsg << "push {";
      for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
      {
        if(inst&rb)
        {
          if(rc) statusMsg << ",";
          statusMsg << "r" << dec << ra;
          rc++;
        }
      }
      if(inst&0x100)
      {
        if(rc) statusMsg << ",";
        statusMsg << "lr";
      }
      statusMsg << "}" << endl;
    #endif

      sp = read_register(13);
      //fprintf(stderr,"sp 0x%08X\n",sp);
      for(ra = 0, rb = 0x01, rc = 0; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          ++rc;
        }
      }
      if(inst & 0x100) ++rc;
      rc <<= 2;
      sp -= rc;
      rd = sp;
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          write32(rd, read_register(ra));
          rd += 4;
        }
      }
      if(inst & 0x100)
      {
        rc = read_register(14);
        write32(rd, rc);
        if((rc & 1) == 0)
        {
          // FIXME fprintf(stderr,"push {lr} with an ARM address pc 0x%08X popped 0x%08X\n",pc,rc);
        }
      }
      write_register(13, sp);
      return 0;
    }

    //REV
    case Op::rev: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "rev r" << dec << rd << ",r" << dec << rn << endl);
      ra = read_register(rn);
      rc  = ((ra >>  0) & 0xFF) << 24;
      rc |= ((ra >>  8) & 0xFF) << 16;
      rc |= ((ra >> 16) & 0xFF) <<  8;
      rc |= ((ra >> 24) & 0xFF) <<  0;
      write_register(rd, rc);
      return 0;
    }

    //REV16
    case Op::rev16: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "rev16 r" << dec << rd << ",r" << dec << rn << endl);
      ra = read_register(rn);
      rc  = ((ra >>  0) & 0xFF) <<  8;
      rc |= ((ra >>  8) & 0xFF) <<  0;
      rc |= ((ra >> 16) & 0xFF) << 24;
      rc |= ((ra >> 24) & 0xFF) << 16;
      write_register(rd, rc);
      return 0;
    }

    //REVSH
    case Op::revsh: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "revsh r" << dec << rd << ",r" << dec << rn << endl);
      ra = read_register(rn);
      rc  = ((ra >> 0) & 0xFF) << 8;
      rc |= ((ra >> 8) & 0xFF) << 0;
      if(rc & 0x8000) rc |= 0xFFFF0000;
      else            rc &= 0x0000FFFF;
      write_register(rd, rc);
      return 0;
    }

    //ROR
    case Op::ror: {
      rd = (inst >> 0) & 0x7;
      rs = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "rors r" << dec << rd << ",r" << dec << rs << endl);
      rc = read_register(rd);
      ra = read_register(rs);
      ra &= 0xFF;
      if(ra == 0)
      {
      }
      else
      {
        ra &= 0x1F;
        if(ra == 0)
        {
          do_cflag_bit(rc & 0x80000000);
        }
        else
        {
          do_cflag_bit(rc & (1 << (ra-1)));
          rb = rc << (32-ra);
          rc >>= ra;
          rc |= rb;
        }
      }
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //SBC
    case Op::sbc: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "sbc r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rd);
      rb = read_register(rm);
      rc = ra - rb;
      if(!(cpsr & CPSR_C)) --rc;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      if(cpsr & CPSR_C)
      {
        do_cflag(ra, ~rb, 1);
        do_vflag(ra, ~rb, 1);
      }
      else
      {
        do_cflag(ra, ~rb, 0);
        do_vflag(ra, ~rb, 0);
      }
      return 0;
    }

#ifndef UNSAFE_OPTIMIZATIONS
    //SETEND
    case Op::setend: {
      statusMsg << "setend not implemented" << endl;
      return 1;
    }
#endif

    //STMIA
    case Op::stmia: {
      rn = (inst >> 8) & 0x7;
    #if defined(THUMB_DISS)
      statusMsg << "stmia r" << dec << rn << "!,{";
      for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,++ra)
      {
        if(inst & rb)
        {
          if(rc) statusMsg << ",";
          statusMsg << "r" << dec << ra;
          rc++;
        }
      }
      statusMsg << "}" << endl;
    #endif

      sp = read_register(rn);
      for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ++ra)
      {
        if(inst & rb)
        {
          write32(sp, read_register(ra));
          sp += 4;
        }
      }
      write_register(rn, sp);
      return 0;
    }

    //STR(1)
    case Op::str1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 2;
      DO_DISS(statusMsg << "str r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(rn) + rb;
      rc = read_register(rd);
      write32(rb, rc);
      return 0;
    }

    //STR(2)
    case Op::str2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "str r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);
      write32(rb, rc);
      return 0;
    }

    //STR(3)
    case Op::str3: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      rb <<= 2;
      DO_DISS(statusMsg << "str r" << dec << rd << ",[SP,#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(13) + rb;
      //fprintf(stderr,"0x%08X\n",rb);
      rc = read_register(rd);
      write32(rb, rc);
      return 0;
    }

    //STRB(1)
    case Op::strb1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      DO_DISS(statusMsg << "strb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX8 << rb << "]" << endl);
      rb = read_register(rn) + rb;
      rc = read_register(rd);
#ifndef UNSAFE_OPTIMIZATIONS
      ra = read16(rb & (~1U));
#else
      ra = read16(rb);
#endif
      if(rb & 1)
      {
        ra &= 0x00FF;
        ra |= rc << 8;
      }
      else
      {
        ra &= 0xFF00;
        ra |= rc & 0x00FF;
      }
      write16(rb & (~1U), ra & 0xFFFF);
      return 0;
    }

    //STRB(2)
    case Op::strb2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "strb r" << dec << rd << ",[r" << dec << rn << ",r" << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);
#ifndef UNSAFE_OPTIMIZATIONS
      ra = read16(rb & (~1U));
#else
      ra = read16(rb);
#endif
      if(rb & 1)
      {
        ra &= 0x00FF;
        ra |= rc << 8;
      }
      else
      {
        ra &= 0xFF00;
        ra |= rc & 0x00FF;
      }
      write16(rb & (~1U), ra & 0xFFFF);
      return 0;
    }

    //STRH(1)
    case Op::strh1: {
      rd = (inst >> 0) & 0x07;
      rn = (inst >> 3) & 0x07;
      rb = (inst >> 6) & 0x1F;
      rb <<= 1;
      DO_DISS(statusMsg << "strh r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
      rb = read_register(rn) + rb;
      rc=  read_register(rd);
      write16(rb, rc & 0xFFFF);
      return 0;
    }

    //STRH(2)
    case Op::strh2: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "strh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
      rb = read_register(rn) + read_register(rm);
      rc = read_register(rd);
      write16(rb, rc & 0xFFFF);
      return 0;
    }

    //SUB(1)
    case Op::sub1: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rb = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "subs r" << dec << rd << ",r" << dec << rn << ",#0x" << Base::HEX2 << rb << endl);
      ra = read_register(rn);
      rc = ra - rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

    //SUB(2)
    case Op::sub2: {
      rb = (inst >> 0) & 0xFF;
      rd = (inst >> 8) & 0x07;
      DO_DISS(statusMsg << "subs r" << dec << rd << ",#0x" << Base::HEX2 << rb << endl);
      ra = read_register(rd);
      rc = ra - rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

    //SUB(3)
    case Op::sub3: {
      rd = (inst >> 0) & 0x7;
      rn = (inst >> 3) & 0x7;
      rm = (inst >> 6) & 0x7;
      DO_DISS(statusMsg << "subs r" << dec << rd << ",r" << dec << rn << ",r" << dec << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra - rb;
      write_register(rd, rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra, ~rb, 1);
      do_vflag(ra, ~rb, 1);
      return 0;
    }

    //SUB(4)
    case Op::sub4: {
      rb = inst & 0x7F;
      rb <<= 2;
      DO_DISS(statusMsg << "sub SP,#0x" << Base::HEX2 << rb << endl);
      ra = read_register(13);
      ra -= rb;
      write_register(13, ra);
      return 0;
    }

    //SWI
    case Op::swi: {
      rb = inst & 0xFF;
      DO_DISS(statusMsg << "swi 0x" << Base::HEX2 << rb << endl);

      if(rb == 0xCC)
      {
        write_register(0, cpsr);
        return 0;
      }
      else
      {
#if defined(THUMB_DISS)
        statusMsg << endl << endl << "swi 0x" << Base::HEX2 << rb << endl;
#endif
        return 1;
      }
    }

    //SXTB
    case Op::sxtb: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "sxtb r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = ra & 0xFF;
      if(rc & 0x80)
        rc |= (~0U) << 8;
      write_register(rd, rc);
      return 0;
    }

    //SXTH
    case Op::sxth: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "sxth r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = ra & 0xFFFF;
      if(rc & 0x8000)
        rc |= (~0U) << 16;
      write_register(rd, rc);
      return 0;
    }

    //TST
    case Op::tst: {
      rn = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "tst r" << dec << rn << ",r" << dec << rm << endl);
      ra = read_register(rn);
      rb = read_register(rm);
      rc = ra & rb;
      do_nflag(rc);
      do_zflag(rc);
      return 0;
    }

    //UXTB
    case Op::uxtb: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "uxtb r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = ra & 0xFF;
      write_register(rd, rc);
      return 0;
    }

    //UXTH
    case Op::uxth: {
      rd = (inst >> 0) & 0x7;
      rm = (inst >> 3) & 0x7;
      DO_DISS(statusMsg << "uxth r" << dec << rd << ",r" << dec << rm << endl);
      ra = read_register(rm);
      rc = ra & 0xFFFF;
      write_register(rd, rc);
      return 0;
    }

#ifndef UNSAFE_OPTIMIZATIONS
    case Op::invalid:
      break;
#else
    default:
      break;
#endif
  }

#ifndef UNSAFE_OPTIMIZATIONS
  statusMsg << "invalid instruction " << Base::HEX8 << pc << " " << Base::HEX4 << inst << endl;
#endif
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::reset()
{
  reg_norm.fill(0);

  reg_norm[13] = cStack;              // SP
  reg_norm[14] = cBase;               // LR
  reg_norm[15] = (cStart + 2) | 1;    // PC (+2 for pipeline, lower bit for THUMB)

  cpsr = mamcr = 0;
  handler_mode = false;

  systick_ctrl = 0x00000004;
  systick_reload = 0x00000000;
  systick_count = 0x00000000;
  systick_calibrate = 0x00ABCDEF;

  // fxq: don't care about below so much (maybe to guess timing???)
#ifndef UNSAFE_OPTIMIZATIONS
  instructions = 0;
  statusMsg.str("");
#endif
#ifndef NO_THUMB_STATS
  _stats.fetches = _stats.reads = _stats.writes = 0;
#endif

  return 0;
}

#ifndef UNSAFE_OPTIMIZATIONS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Thumbulator::trapOnFatal = true;
#endif
