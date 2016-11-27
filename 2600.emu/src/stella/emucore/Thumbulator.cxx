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
// $Id: Thumbulator.cxx 3308 2016-05-24 16:55:45Z stephena $
//============================================================================

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifdef THUMB_SUPPORT

#include "bspf.hxx"
#include "Base.hxx"
#include "Thumbulator.hxx"
using Common::Base;

// Uncomment the following to enable specific functionality
// WARNING!!! This slows the runtime to a crawl
//#define THUMB_DISS
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
  #define CONV_DATA(d)   (((d & 0xFFFF)>>8) | ((d & 0xffff)<<8)) & 0xffff;
  #define CONV_RAMROM(d) ((d>>8) | (d<<8)) & 0xffff;
#else
  #define CONV_DATA(d)   (d & 0xFFFF);
  #define CONV_RAMROM(d) (d);
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(const uInt16* rom_ptr, uInt16* ram_ptr, bool traponfatal)
  : rom(rom_ptr),
    ram(ram_ptr)
{
  trapFatalErrors(traponfatal);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Thumbulator::run()
{
  reset();
  for(;;)
  {
    if(execute()) break;
    if(instructions > 500000) // way more than would otherwise be possible
      throw runtime_error("instructions > 500000");
  }
#if defined(THUMB_DISS) || defined(THUMB_DBUG)
  dump_counters();
  cout << statusMsg.str() << endl;
#endif
  return statusMsg.str();
}

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
       << "instructions " << instructions << endl
       << "fetches      " << fetches << endl
       << "reads        " << reads << endl
       << "writes       " << writes << endl
       << "memcycles    " << (fetches+reads+writes) << endl
       << "systick_ints " << systick_ints << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::dump_regs()
{
  for (int cnt = 1; cnt < 14; cnt++)
  {
    statusMsg << "R" << cnt << " = " << Base::HEX8 << reg_norm[cnt-1] << "  ";
    if(cnt % 4 == 0) statusMsg << endl;
  }
  statusMsg << endl
            << "SP = " << Base::HEX8 << reg_norm[13] << "  "
            << "LR = " << Base::HEX8 << reg_norm[14] << "  "
            << "PC = " << Base::HEX8 << reg_norm[15] << "  "
            << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::fetch16(uInt32 addr)
{
  fetches++;

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::fetch32(uInt32 addr)
{
  uInt32 data;
  switch(addr & 0xF0000000)
  {
    case 0x00000000: //ROM
      if(addr < 0x50)
      {
        data = read32(addr);
        DO_DBUG(statusMsg << "fetch32(" << Base::HEX8 << addr << ")=" << Base::HEX8 << data << endl);
        if(addr == 0x00000000) return data;
        if(addr == 0x00000004) return data;
        if(addr == 0x0000003C) return data;
        fatalError("fetch32", addr, "abort");
      }

    case 0x40000000: //RAM
      data = read32(addr);
      DO_DBUG(statusMsg << "fetch32(" << Base::HEX8 << addr << ")=" << Base::HEX8 << data << endl);
      return data;
  }
  return fatalError("fetch32", addr, "abort");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write16(uInt32 addr, uInt32 data)
{
  if((addr > 0x40001fff) && (addr < 0x50000000))
    fatalError("write16", addr, "abort - out of range");
  else if((addr > 0x40000028) && (addr < 0x40000c00))
    fatalError("write16", addr, "to bankswitch code area");
  if(addr & 1)
    fatalError("write16", addr, "abort - misaligned");

  writes++;

  DO_DBUG(statusMsg << "write16(" << Base::HEX8 << addr << "," << Base::HEX8 << data << ")" << endl);

  switch(addr & 0xF0000000)
  {
    case 0x40000000: //RAM
      addr &= RAMADDMASK;
      addr >>= 1;
      ram[addr] = CONV_DATA(data);
      return;

    case 0xE0000000: //MAMCR
      if(addr == 0xE01FC000)
      {
        DO_DBUG(statusMsg << "write16(" << Base::HEX8 << "MAMCR" << "," << Base::HEX8 << data << ") *" << endl);
        mamcr = data;
        return;
      }
  }
  fatalError("write16", addr, data, "abort");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write32(uInt32 addr, uInt32 data)
{
  if(addr & 3)
    fatalError("write32", addr, "abort - misaligned");

  DO_DBUG(statusMsg << "write32(" << Base::HEX8 << addr << "," << Base::HEX8 << data << ")" << endl);

  switch(addr & 0xF0000000)
  {
    case 0xF0000000: //halt
      dump_counters();
      throw runtime_error("HALT");

    case 0xE0000000: //periph
      switch(addr)
      {
        case 0xE0000000:
          DO_DISS(statusMsg << "uart: [" << char(data&0xFF) << "]" << endl);
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
      }
      return;

    case 0xD0000000: //debug
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
      }
      return;
      
    case 0x40000000: //RAM
      write16(addr+0, (data >>  0) & 0xFFFF);
      write16(addr+2, (data >> 16) & 0xFFFF);
      return;
  }
  fatalError("write32", addr, data, "abort");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read16(uInt32 addr)
{
  uInt32 data;

  if((addr > 0x40001fff) && (addr < 0x50000000))
    fatalError("read16", addr, "abort - out of range");
  else if((addr > 0x7fff) && (addr < 0x10000000))
    fatalError("read16", addr, "abort - out of range");
  if(addr & 1)
    fatalError("read16", addr, "abort - misaligned");

  reads++;

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

    case 0xE0000000: //MAMCR
      if(addr == 0xE01FC000)
      {
        DO_DBUG(statusMsg << "read16(" << "MAMCR" << addr << ")=" << mamcr << " *");
        return mamcr;
      }
  }
  return fatalError("read16", addr, "abort");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read32(uInt32 addr)
{
  if(addr & 3)
    fatalError("read32", addr, "abort - misaligned");

  uInt32 data;
  switch(addr & 0xF0000000)
  {
    case 0x00000000: //ROM
    case 0x40000000: //RAM
      data = read16(addr+0);
      data |= (uInt32(read16(addr+2))) << 16;
      DO_DBUG(statusMsg << "read32(" << Base::HEX8 << addr << ")=" << Base::HEX8 << data << endl);
      return data;

    case 0xE0000000:
    {
      switch(addr)
      {
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

        case 0xE000E01C:
          data = systick_calibrate;
          return data;
      }
    }
  }
  return fatalError("read32", addr, "abort");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read_register(uInt32 reg)
{
  reg &= 0xF;

  uInt32 data = reg_norm[reg];
  DO_DBUG(statusMsg << "read_register(" << dec << reg << ")=" << Base::HEX8 << data << endl);
  if(reg == 15)
  {
    if(data & 1)
    {
      DO_DBUG(statusMsg << "pc has lsbit set 0x" << Base::HEX8 << data << endl);
    }
    data &= ~1;
  }
  return data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write_register(uInt32 reg, uInt32 data)
{
  reg &= 0xF;

  DO_DBUG(statusMsg << "write_register(" << dec << reg << "," << Base::HEX8 << data << ")" << endl);
  if(reg == 15) data &= ~1;
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
  if(x & 0x80000000) cpsr|=CPSR_N;  else cpsr&=~CPSR_N;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_cflag(uInt32 a, uInt32 b, uInt32 c)
{
  uInt32 rc;

  cpsr &= ~CPSR_C;
  rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
  rc = (rc >> 31) + (a >> 31) + (b >> 31);      //carry out
  if(rc & 2)
    cpsr |= CPSR_C;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_vflag(uInt32 a, uInt32 b, uInt32 c)
{
  uInt32 rc, rd;

  cpsr &= ~CPSR_V;
  rc = (a & 0x7FFFFFFF) + (b & 0x7FFFFFFF) + c; //carry in
  rc >>= 31; //carry in in lsbit
  rd = (rc & 1) + ((a >> 31) & 1) + ((b >> 31) & 1); //carry out
  rd >>= 1; //carry out in lsbit
  rc = (rc^rd) & 1; //if carry in != carry out then signed overflow
  if(rc)
    cpsr |= CPSR_V;
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
int Thumbulator::execute()
{
  uInt32 pc, sp, inst, ra, rb, rc, rm, rd, rn, rs, op;

  pc = read_register(15);

#if 0  // FIXME SA - not sure if this should be enabled
  if(handler_mode)
  {
    if((pc & 0xF0000000) == 0xF0000000)
    {
      uInt32 sp = read_register(13);
      handler_mode = false;
      write_register(0,  read32(sp)); sp += 4;
      write_register(1,  read32(sp)); sp += 4;
      write_register(2,  read32(sp)); sp += 4;
      write_register(3,  read32(sp)); sp += 4;
      write_register(12, read32(sp)); sp += 4;
      write_register(14, read32(sp)); sp += 4;
      pc = read32(sp); sp += 4;
      cpsr = read32(sp); sp += 4;
      write_register(13, sp);
    }
  }
  if(systick_ctrl & 1)
  {
    if(systick_count)
    {
      systick_count--;
    }
    else
    {
      systick_count = systick_reload;
      systick_ctrl |= 0x00010000;
    }
  }

  if((systick_ctrl & 3) == 3)
  {
    if(systick_ctrl & 0x00010000)
    {
      if(!handler_mode)
      {
        systick_ints++;
        uInt32 sp = read_register(13);
        sp -= 4; write32(sp, cpsr);
        sp -= 4; write32(sp, pc);
        sp -= 4; write32(sp, read_register(14));
        sp -= 4; write32(sp, read_register(12));
        sp -= 4; write32(sp, read_register(3));
        sp -= 4; write32(sp, read_register(2));
        sp -= 4; write32(sp, read_register(1));
        sp -= 4; write32(sp, read_register(0));
        write_register(13, sp);
        pc = fetch32(0x0000003C); //systick vector
        pc += 2;
        //write_register(14, 0xFFFFFF00);
        write_register(14, 0xFFFFFFF9);

        handler_mode = true;
      }
    }
  }
#endif

  inst = fetch16(pc-2);
  pc += 2;
  write_register(15, pc);
  DO_DISS(statusMsg << Base::HEX8 << (pc-5) << ": " << Base::HEX4 << inst << " ");

  instructions++;

  //ADC
  if((inst & 0xFFC0) == 0x4140)
  {
    rd = (inst >> 0) & 0x07;
    rm = (inst >> 3) & 0x07;
    DO_DISS(statusMsg << "adc r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rd);
    rb = read_register(rm);
    rc = ra + rb;
    if(cpsr & CPSR_C)
      rc++;
    write_register(rd, rc);
    do_nflag(rc);
    do_zflag(rc);
    if(cpsr & CPSR_C) { do_cflag(ra, rb, 1); do_vflag(ra, rb, 1); }
    else              { do_cflag(ra, rb, 0); do_vflag(ra, rb, 0); }
    return 0;
  }

  //ADD(1) small immediate two registers
  if((inst & 0xFE00) == 0x1C00)
  {
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
  }

  //ADD(2) big immediate one register
  if((inst & 0xF800) == 0x3000)
  {
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
  if((inst & 0xFE00) == 0x1800)
  {
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
  if((inst & 0xFF00) == 0x4400)
  {
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
      if((rc & 1) == 0)
        fatalError("add pc", pc, rc, " produced an arm address");

      rc &= ~1; //write_register may do this as well
      rc += 2;  //The program counter is special
    }
    //fprintf(stderr,"0x%08X = 0x%08X + 0x%08X\n",rc,ra,rb);
    write_register(rd, rc);
    return 0;
  }

  //ADD(5) rd = pc plus immediate
  if((inst & 0xF800) == 0xA000)
  {
    rb = (inst >> 0) & 0xFF;
    rd = (inst >> 8) & 0x7;
    rb <<= 2;
    DO_DISS(statusMsg << "add r" << dec << rd << ",PC,#0x" << Base::HEX2 << rb << endl);
    ra = read_register(15);
    rc = (ra & (~3u)) + rb;
    write_register(rd, rc);
    return 0;
  }

  //ADD(6) rd = sp plus immediate
  if((inst & 0xF800) == 0xA800)
  {
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
  if((inst & 0xFF80) == 0xB000)
  {
    rb = (inst >> 0) & 0x7F;
    rb <<= 2;
    DO_DISS(statusMsg << "add SP,#0x" << Base::HEX2 << rb << endl);
    ra = read_register(13);
    rc = ra + rb;
    write_register(13, rc);
    return 0;
  }

  //AND
  if((inst & 0xFFC0) == 0x4000)
  {
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
  if((inst & 0xF800) == 0x1000)
  {
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
        rc = ~0;
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
        rc |= (~0u) << (32-rb);
    }
    write_register(rd, rc);
    do_nflag(rc);
    do_zflag(rc);
    return 0;
  }

  //ASR(2) two register
  if((inst & 0xFFC0) == 0x4100)
  {
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
        rc |= (~0u) << (32-rb);
      }
    }
    else
    {
      if(rc & 0x80000000)
      {
        do_cflag_bit(1);
        rc = (~0u);
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
  if((inst & 0xF000) == 0xD000)
  {
    rb = (inst >> 0) & 0xFF;
    if(rb & 0x80)
      rb |= (~0u) << 8;
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
        ra = 0;
        if(  (cpsr & CPSR_N)  &&   (cpsr & CPSR_V) ) ra++;
        if((!(cpsr & CPSR_N)) && (!(cpsr & CPSR_V))) ra++;
        if(ra)
          write_register(15, rb);
        return 0;

      case 0xB: //b lt N != V
        DO_DISS(statusMsg << "blt 0x" << Base::HEX8 << (rb-3) << endl);
        ra = 0;
        if((!(cpsr & CPSR_N)) && (cpsr & CPSR_V)) ra++;
        if((!(cpsr & CPSR_V)) && (cpsr & CPSR_N)) ra++;
        if(ra)
          write_register(15, rb);
        return 0;

      case 0xC: //b gt Z==0 and N == V
        DO_DISS(statusMsg << "bgt 0x" << Base::HEX8 << (rb-3) << endl);
        ra = 0;
        if(  (cpsr & CPSR_N)  &&   (cpsr & CPSR_V) ) ra++;
        if((!(cpsr & CPSR_N)) && (!(cpsr & CPSR_V))) ra++;
        if(cpsr & CPSR_Z) ra = 0;
        if(ra)
          write_register(15, rb);
        return 0;

      case 0xD: //b le Z==1 or N != V
        DO_DISS(statusMsg << "ble 0x" << Base::HEX8 << (rb-3) << endl);
        ra = 0;
        if((!(cpsr & CPSR_N)) && (cpsr & CPSR_V)) ra++;
        if((!(cpsr & CPSR_V)) && (cpsr & CPSR_N)) ra++;
        if(cpsr & CPSR_Z) ra++;
        if(ra)
          write_register(15, rb);
        return 0;

      case 0xE:
        //undefined instruction
        break;

      case 0xF:
        //swi
        break;
    }
  }

  //B(2) unconditional branch
  if((inst & 0xF800) == 0xE000)
  {
    rb = (inst >> 0) & 0x7FF;
    if(rb & (1 << 10))
      rb |= (~0u) << 11;
    rb <<= 1;
    rb += pc;
    rb += 2;
    DO_DISS(statusMsg << "B 0x" << Base::HEX8 << (rb-3) << endl);
    write_register(15, rb);
    return 0;
  }

  //BIC
  if((inst & 0xFFC0) == 0x4380)
  {
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

  //BKPT
  if((inst & 0xFF00) == 0xBE00)
  {
    rb = (inst >> 0) & 0xFF;
    statusMsg << "bkpt 0x" << Base::HEX2 << rb << endl;
    return 1;
  }

  //BL/BLX(1)
  if((inst & 0xE000) == 0xE000) //BL,BLX
  {
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
      rb += (inst & ((1 << 11) - 1)) << 1;;
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
      rb += (inst & ((1 << 11) - 1)) << 1;;
      rb &= 0xFFFFFFFC;
      rb += 2;
      DO_DISS(statusMsg << "bl 0x" << Base::HEX8 << (rb-3) << endl);
      write_register(14, (pc-2) | 1);
      write_register(15, rb);
      return 0;
    }
  }

  //BLX(2)
  if((inst & 0xFF87) == 0x4780)
  {
    rm = (inst >> 3) & 0xF;
    DO_DISS(statusMsg << "blx r" << dec << rm << endl);
    rc = read_register(rm);
    //fprintf(stderr,"blx r%u 0x%X 0x%X\n",rm,rc,pc);
    rc += 2;
    if(rc & 1)
    {
      write_register(14, (pc-2) | 1);
      rc &= ~1;
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
  if((inst & 0xFF87) == 0x4700)
  {
    rm = (inst >> 3) & 0xF;
    DO_DISS(statusMsg << "bx r" << dec << rm << endl);
    rc = read_register(rm);
    rc += 2;
    //fprintf(stderr,"bx r%u 0x%X 0x%X\n",rm,rc,pc);
    if(rc & 1)
    {
      rc &= ~1;
      write_register(15, rc);
      return 0;
    }
    else
    {
      //fprintf(stderr,"cannot branch to arm 0x%08X 0x%04X\n",pc,inst);
      // fxq: or maybe this one??
      return 1;
    }
  }

  //CMN
  if((inst & 0xFFC0) == 0x42C0)
  {
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
  if((inst & 0xF800) == 0x2800)
  {
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
  if((inst & 0xFFC0) == 0x4280)
  {
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
  if((inst & 0xFF00) == 0x4500)
  {
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

  //CPS
  if((inst & 0xFFE8) == 0xB660)
  {
    DO_DISS(statusMsg << "cps TODO" << endl);
    return 1;
  }

  //CPY copy high register
  if((inst & 0xFFC0) == 0x4600)
  {
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
  if((inst & 0xFFC0) == 0x4040)
  {
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
  if((inst & 0xF800) == 0xC800)
  {
    rn = (inst >> 8) & 0x7;
  #if defined(THUMB_DISS)
    statusMsg << "ldmia r" << dec << rn << "!,{";
    for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
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
    for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ra++)
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
  if((inst & 0xF800) == 0x6800)
  {
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
  if((inst & 0xFE00) == 0x5800)
  {
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
  if((inst & 0xF800) == 0x4800)
  {
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
  if((inst & 0xF800) == 0x9800)
  {
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
  if((inst & 0xF800) == 0x7800)
  {
    rd = (inst >> 0) & 0x07;
    rn = (inst >> 3) & 0x07;
    rb = (inst >> 6) & 0x1F;
    DO_DISS(statusMsg << "ldrb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
    rb = read_register(rn) + rb;
    rc = read16(rb & (~1u));
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
  if((inst & 0xFE00) == 0x5C00)
  {
    rd = (inst >> 0) & 0x7;
    rn = (inst >> 3) & 0x7;
    rm = (inst >> 6) & 0x7;
    DO_DISS(statusMsg << "ldrb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
    rb = read_register(rn) + read_register(rm);
    rc = read16(rb & (~1u));
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

  //LDRH(1)
  if((inst & 0xF800) == 0x8800)
  {
    rd = (inst >> 0) & 0x07;
    rn = (inst >> 3) & 0x07;
    rb = (inst >> 6) & 0x1F;
    rb <<= 1;
    DO_DISS(statusMsg << "ldrh r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX2 << rb << "]" << endl);
    rb=read_register(rn) + rb;
    rc = read16(rb);
    write_register(rd, rc & 0xFFFF);
    return 0;
  }

  //LDRH(2)
  if((inst & 0xFE00) == 0x5A00)
  {
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
  if((inst & 0xFE00) == 0x5600)
  {
    rd = (inst >> 0) & 0x7;
    rn = (inst >> 3) & 0x7;
    rm = (inst >> 6) & 0x7;
    DO_DISS(statusMsg << "ldrsb r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
    rb = read_register(rn) + read_register(rm);
    rc = read16(rb & (~1u));
    if(rb & 1)
    {
      rc >>= 8;
    }
    else
    {
    }
    rc &= 0xFF;
    if(rc & 0x80)
      rc |= ((~0u) << 8);
    write_register(rd, rc);
    return 0;
  }

  //LDRSH
  if((inst & 0xFE00) == 0x5E00)
  {
    rd = (inst >> 0) & 0x7;
    rn = (inst >> 3) & 0x7;
    rm = (inst >> 6) & 0x7;
    DO_DISS(statusMsg << "ldrsh r" << dec << rd << ",[r" << dec << rn << ",r" << dec << rm << "]" << endl);
    rb = read_register(rn) + read_register(rm);
    rc = read16(rb);
    rc &= 0xFFFF;
    if(rc & 0x8000)
      rc |= ((~0u) << 16);
    write_register(rd, rc);
    return 0;
  }

  //LSL(1)
  if((inst & 0xF800) == 0x0000)
  {
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
  if((inst & 0xFFC0) == 0x4080)
  {
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
  if((inst & 0xF800) == 0x0800)
  {
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
  if((inst & 0xFFC0) == 0x40C0)
  {
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
  if((inst & 0xF800) == 0x2000)
  {
    rb = (inst >> 0) & 0xFF;
    rd = (inst >> 8) & 0x07;
    DO_DISS(statusMsg << "movs r" << dec << rd << ",#0x" << Base::HEX2 << rb << endl);
    write_register(rd, rb);
    do_nflag(rb);
    do_zflag(rb);
    return 0;
  }

  //MOV(2) two low registers
  if((inst & 0xFFC0) == 0x1C00)
  {
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
  if((inst & 0xFF00) == 0x4600)
  {
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
      rc &= ~1; //write_register may do this as well
      rc += 2;  //The program counter is special
    }
    write_register(rd, rc);
    return 0;
  }

  //MUL
  if((inst & 0xFFC0) == 0x4340)
  {
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
  if((inst & 0xFFC0) == 0x43C0)
  {
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
  if((inst & 0xFFC0) == 0x4240)
  {
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
  if((inst & 0xFFC0) == 0x4300)
  {
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
  if((inst & 0xFE00) == 0xBC00)
  {
  #if defined(THUMB_DISS)
    statusMsg << "pop {";
    for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
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
    for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ra++)
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
  if((inst & 0xFE00) == 0xB400)
  {
  #if defined(THUMB_DISS)
    statusMsg << "push {";
    for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
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
    for(ra = 0, rb = 0x01, rc = 0; rb; rb = (rb << 1) & 0xFF, ra++)
    {
      if(inst & rb)
      {
        rc++;
      }
    }
    if(inst & 0x100) rc++;
    rc <<= 2;
    sp -= rc;
    rd = sp;
    for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ra++)
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
  if((inst & 0xFFC0) == 0xBA00)
  {
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
  if((inst & 0xFFC0) == 0xBA40)
  {
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
  if((inst & 0xFFC0) == 0xBAC0)
  {
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
  if((inst & 0xFFC0) == 0x41C0)
  {
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
  if((inst & 0xFFC0) == 0x4180)
  {
    rd = (inst >> 0) & 0x7;
    rm = (inst >> 3) & 0x7;
    DO_DISS(statusMsg << "sbc r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rd);
    rb = read_register(rm);
    rc = ra - rb;
    if(!(cpsr & CPSR_C)) rc--;
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

  //SETEND
  if((inst & 0xFFF7) == 0xB650)
  {
    statusMsg << "setend not implemented" << endl;
    return 1;
  }

  //STMIA
  if((inst & 0xF800) == 0xC000)
  {
    rn = (inst >> 8) & 0x7;
  #if defined(THUMB_DISS)
    statusMsg << "stmia r" << dec << rn << "!,{";
    for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
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
    for(ra = 0, rb = 0x01; rb; rb = (rb << 1) & 0xFF, ra++)
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
  if((inst & 0xF800) == 0x6000)
  {
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
  if((inst & 0xFE00) == 0x5000)
  {
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
  if((inst & 0xF800) == 0x9000)
  {
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
  if((inst & 0xF800) == 0x7000)
  {
    rd = (inst >> 0) & 0x07;
    rn = (inst >> 3) & 0x07;
    rb = (inst >> 6) & 0x1F;
    DO_DISS(statusMsg << "strb r" << dec << rd << ",[r" << dec << rn << ",#0x" << Base::HEX8 << rb << "]" << endl);
    rb = read_register(rn) + rb;
    rc = read_register(rd);
    ra = read16(rb & (~1u));
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
    write16(rb & (~1u), ra & 0xFFFF);
    return 0;
  }

  //STRB(2)
  if((inst & 0xFE00) == 0x5400)
  {
    rd = (inst >> 0) & 0x7;
    rn = (inst >> 3) & 0x7;
    rm = (inst >> 6) & 0x7;
    DO_DISS(statusMsg << "strb r" << dec << rd << ",[r" << dec << rn << ",r" << rm << "]" << endl);
    rb = read_register(rn) + read_register(rm);
    rc = read_register(rd);
    ra = read16(rb & (~1u));
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
    write16(rb & (~1u), ra & 0xFFFF);
    return 0;
  }

  //STRH(1)
  if((inst & 0xF800) == 0x8000)
  {
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
  if((inst & 0xFE00) == 0x5200)
  {
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
  if((inst & 0xFE00) == 0x1E00)
  {
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
  if((inst & 0xF800) == 0x3800)
  {
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
  if((inst & 0xFE00) == 0x1A00)
  {
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
  if((inst & 0xFF80) == 0xB080)
  {
    rb = inst & 0x7F;
    rb <<= 2;
    DO_DISS(statusMsg << "sub SP,#0x" << Base::HEX2 << rb << endl);
    ra = read_register(13);
    ra -= rb;
    write_register(13, ra);
    return 0;
  }

  //SWI
  if((inst & 0xFF00) == 0xDF00)
  {
    rb = inst & 0xFF;
    DO_DISS(statusMsg << "swi 0x" << Base::HEX2 << rb << endl);

    if((inst & 0xFF) == 0xCC)
    {
      write_register(0, cpsr);
      return 0;
    }
    else
    {
      statusMsg << endl << endl << "swi 0x" << Base::HEX2 << rb << endl;
      return 1;
    }
  }

  //SXTB
  if((inst & 0xFFC0) == 0xB240)
  {
    rd = (inst >> 0) & 0x7;
    rm = (inst >> 3) & 0x7;
    DO_DISS(statusMsg << "sxtb r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rm);
    rc = ra & 0xFF;
    if(rc & 0x80)
      rc |= (~0u) << 8;
    write_register(rd, rc);
    return 0;
  }

  //SXTH
  if((inst & 0xFFC0) == 0xB200)
  {
    rd = (inst >> 0) & 0x7;
    rm = (inst >> 3) & 0x7;
    DO_DISS(statusMsg << "sxth r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rm);
    rc = ra & 0xFFFF;
    if(rc & 0x8000)
      rc |= (~0u) << 16;
    write_register(rd, rc);
    return 0;
  }

  //TST
  if((inst & 0xFFC0) == 0x4200)
  {
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
  if((inst & 0xFFC0) == 0xB2C0)
  {
    rd = (inst >> 0) & 0x7;
    rm = (inst >> 3) & 0x7;
    DO_DISS(statusMsg << "uxtb r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rm);
    rc = ra & 0xFF;
    write_register(rd, rc);
    return 0;
  }

  //UXTH
  if((inst & 0xFFC0) == 0xB280)
  {
    rd = (inst >> 0) & 0x7;
    rm = (inst >> 3) & 0x7;
    DO_DISS(statusMsg << "uxth r" << dec << rd << ",r" << dec << rm << endl);
    ra = read_register(rm);
    rc = ra & 0xFFFF;
    write_register(rd, rc);
    return 0;
  }

  statusMsg << "invalid instruction " << Base::HEX8 << pc << " " << Base::HEX4 << inst << endl;
  return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::reset()
{
  std::fill(reg_norm, reg_norm+12, 0);
  reg_norm[13] = 0x40001FB4;
  reg_norm[14] = 0x00000C00;
  reg_norm[15] = 0x00000C0B;

  cpsr = mamcr = 0;
  handler_mode = false;

  systick_ctrl = 0x00000004;
  systick_reload = 0x00000000;
  systick_count = 0x00000000;
  systick_calibrate = 0x00ABCDEF;

  // fxq: don't care about below so much (maybe to guess timing???)
  instructions = fetches = reads = writes = systick_ints = 0;

  statusMsg.str("");

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Thumbulator::trapOnFatal = true;

#endif
