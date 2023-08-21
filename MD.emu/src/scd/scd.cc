#include <scd/scd.h>
#include "shared.h"
#include "pcm.h"
#include "LC89510.h"
#include "misc.h"
#include "mem.hh"

#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/ranges.hh>
#include <imagine/io/FileIO.hh>

SegaCD sCD;

uint8_t bram[0x2000];

static int intAckS68k(M68KCPU &m68ki_cpu, int level)
{
	m68ki_cpu.int_level = 0;
  return M68K_INT_ACK_AUTOVECTOR;
}

void scd_interruptSubCpu(unsigned irq)
{
	unsigned enabled = sCD.gate[0x33] & (1 << irq);
	if(!enabled) { bug_unreachable("irq"); }
	//logMsg("int %d, mask %X, enabled %d", irq, sCD.gate[0x33], enabled);
	sCD.cpu.setIRQ(irq);
}

void dumpPRG(const char *n)
{
	auto f = IG::FileIO{n, IG::OpenFlags::testNewFile()};
	f.write(sCD.prg.b, sizeof(sCD.prg.b));
}

void handleBad68KIns()
{
	dumpPRG("s68kcode2.bin");
	//bug_unreachable("");
}

void m68kPCChange(M68KCPU &cpu, unsigned oldPC, unsigned PC)
{
	if(oldPC < 0x20000 && PC >= 0xFF0000)
		logMsg("main cpu bios %08X -> ram %08X code", oldPC, PC);
	else if(oldPC >= 0xFF0000 && PC < 0x20000)
		logMsg("main cpu ram %08X -> bios %08X code", oldPC, PC);
}

void s68kPCChange(M68KCPU &cpu, unsigned oldPC, unsigned PC)
{
	if(oldPC < 0x6000 && PC >= 0x6000)
		logMsg("sub cpu bios %08X -> user %08X code", oldPC, PC);
	else if(oldPC >= 0x6000 && PC < 0x6000)
		logMsg("sub cpu user %08X -> bios %08X code", oldPC, PC);
	else
		logMsg("sub cpu pc %08X -> %08X", oldPC, PC);
}

void scd_resetSubCpu()
{
	sCD.cpu.sp[0]=0;
	m68k_pulse_reset(sCD.cpu);
	//dumpPRG("s68kcode.bin");
}

void scd_runSubCpu(unsigned cycles)
{
	if((sCD.busreq&3) == 1)
	{
		//logMsg("running sub-cpu from cycle %d to %d", sCD.cpu.cycleCount, cycles);
		m68k_run(sCD.cpu, cycles);
	}
	else
		sCD.cpu.cycleCount = cycles;
}

unsigned nullRead8(unsigned address)
{
  logMsg("Null read8 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
  return 0;
}

unsigned nullRead16(unsigned address)
{
	logMsg("Null read16 %08X (%08X)", address, m68k_get_reg (mm68k, M68K_REG_PC));
  return 0;
}

static void updateSegaCdMemMap(SegaCD &sCD)
{
	updateMainCpuPrgMap(sCD);
	updateCpuWordMap(sCD);
	updateMainCpuSramMap(sCD);
}

void scd_init()
{
	logMsg("doing scd init");
	M68KCPU &sm68k = sCD.cpu;
	IG::fill(sm68k.memory_map);

	for (int i=0; i<=0xFF; i++)
	{
		// init
		sm68k.memory_map[i].read8    = subUndefRead8;
		sm68k.memory_map[i].read16   = subUndefRead16;
		sm68k.memory_map[i].write8   = subUndefWrite8;
		sm68k.memory_map[i].write16  = subUndefWrite16;
	}

	for (int i=0; i<0x8; i++)
	{
		// SUB CPU Program RAM
		sm68k.memory_map[i].base     = sCD.prg.b + (i<<16);
		sm68k.memory_map[i].read8    = 0;
		sm68k.memory_map[i].read16   = 0;
		// only first 64K can be write-protected
		sm68k.memory_map[i].write8   = !i ? subPrgWriteProtectCheck8 : 0;
		sm68k.memory_map[i].write16  = !i ? subPrgWriteProtectCheck16 : 0;
	}

	for (int i=0x8; i<0xc; i++)
	{
		// SUB CPU WORD RAM 2Mbit
		sm68k.memory_map[i].base     = sCD.word.ram2M + ((i-8)<<16);
		// read/write handler set on reset
	}

	sm68k.memory_map[0xff].read8    = subGateRead8;
	sm68k.memory_map[0xff].read16   = subGateRead16;
	sm68k.memory_map[0xff].write8   = subGateWrite8;
	sm68k.memory_map[0xff].write16  = subGateWrite16;

	sm68k.memory_map[0xfe].read8    = bramRead8;
	sm68k.memory_map[0xfe].read16   = bramRead16;
	sm68k.memory_map[0xfe].write8   = bramWrite8;
	sm68k.memory_map[0xfe].write16  = bramWrite16;

	m68k_init(sm68k);
	sm68k.setID(1);
	//m68k_set_int_ack_callback(sm68k, intAckS68k);
	//m68k_set_pc_changed_callback(sm68k, s68kPCChange);
	//m68k_set_pc_changed_callback(mm68k, m68kPCChange);

	sCD.TOC = {};
}

void scd_deinit()
{
	logMsg("deinit SCD");
	Stop_CD();
	sCD.isActive = 0;
}

void scd_updateCddaVol()
{
	auto fader = (sCD.gate[0x34] << 4) | (sCD.gate[0x35] >> 4);
	sCD.volume = (fader & 0x7fc) ? (fader & 0x7fc) : (fader & 0x03);
	logMsg("set volume multipler %d", sCD.volume);
}

void scd_reset()
{
	logMsg("doing SCD reset");

	sCD.audioTrack = 0;
	sCD.counter75hz = 0;
	sCD.timer_int3 = 0;
	sCD.stopwatchTimer = 0;
	sCD.delayedDMNA = 0;
	sCD.pcmMem = {};
	sCD.word = {};
	sCD.prg = {};
	IG::fill(sCD.gate);
	sCD.pcm = {};
	sCD.gate[0x3] = 1; // 2M word RAM mode with m68k access after reset
	sCD.volume = 1024;

	scd_resetSubCpu();
	sCD.subResetPending = 1; // s68k reset pending
	sCD.busreq = 0;

	*(uint32a *)(cart.rom + 0x70) = 0xffffffff; // reset hint vector (simplest way to implement reg6)

	LC89510_Reset();
	Reset_CD();
	gfx_cd_reset(sCD.rot_comp);
	updateSegaCdMemMap(sCD);
}

void scd_memmap()
{
  for (int i=0; i<0x40; i++)
  {
    /* clear cartridge ROM */
    mm68k.memory_map[i].base     = 0;
    mm68k.memory_map[i].read8    = m68k_lockup_r_8;
    mm68k.memory_map[i].read16   = m68k_lockup_r_16;
    mm68k.memory_map[i].write8   = m68k_lockup_w_8;
    mm68k.memory_map[i].write16  = m68k_lockup_w_16;
  }

	for (int i=0; i<0x2; i++)
	{
		// BIOS ROM
		mm68k.memory_map[i].base     = cart.rom + (i<<16);
		mm68k.memory_map[i].read8    = 0;
		mm68k.memory_map[i].read16   = 0;
		mm68k.memory_map[i].write8   = m68k_unused_8_w;
		mm68k.memory_map[i].write16  = m68k_unused_16_w;
	}

	for (int i=0x2; i<0x4; i++)
	{
		// SUB CPU Program RAM
		mm68k.memory_map[i].base     = 0;
		mm68k.memory_map[i].read8    = nullRead8;
		mm68k.memory_map[i].read16   = nullRead16;
		mm68k.memory_map[i].write8   = m68k_unused_8_w;
		mm68k.memory_map[i].write16  = m68k_unused_16_w;
	}

	for (int i=0x20; i<0x24; i++)
	{
		// SUB CPU WORD RAM 2Mbit
		mm68k.memory_map[i].base     = sCD.word.ram2M + ((i-0x20)<<16);
		mm68k.memory_map[i].read8    = 0;
		mm68k.memory_map[i].read16   = 0;
		mm68k.memory_map[i].write8   = 0;
		mm68k.memory_map[i].write16  = 0;
	}

	mm68k.memory_map[0x40].read8    = sramCartRegRead8;
	mm68k.memory_map[0x40].read16   = sramCartRegRead16;
	mm68k.memory_map[0x40].write8   = m68k_unused_8_w;
	mm68k.memory_map[0x40].write16  = m68k_unused_16_w;

	mm68k.memory_map[0xa1].read8    = mainGateRead8;
	mm68k.memory_map[0xa1].read16   = mainGateRead16;
	mm68k.memory_map[0xa1].write8   = mainGateWrite8;
	mm68k.memory_map[0xa1].write16  = mainGateWrite16;

	// SRAM cart setup
	for(auto i : IG::iotaCount(2))
	{
		mm68k.memory_map[0x60 + i].base    = 0;
		mm68k.memory_map[0x60 + i].read8   = sramCartRead8;
		mm68k.memory_map[0x60 + i].read16  = sramCartRead16;
	}
	for(auto i : IG::iotaCount(2))
	{
		mm68k.memory_map[0x60 + i].write8   = m68k_unused_8_w;
		mm68k.memory_map[0x60 + i].write16  = m68k_unused_16_w;
	}

	mm68k.memory_map[0x7f].read8    = bcramRegRead8;
	mm68k.memory_map[0x7f].read16   = bcramRegRead8;
	mm68k.memory_map[0x7f].write8   = bcramRegWrite8;
	mm68k.memory_map[0x7f].write16  = bcramRegWrite8;
}

extern bool cdcTransferActive;

void scd_checkDma(void)
{
	if (!(sCD.Status_CDC & 0x08))
	{
		return;
	}

	//logMsg("CDC data transfer in progress");

	int ddx = sCD.gate[4] & 7;
	if (ddx <  2)
	{
		logMsg("invalid CDC DD");
		return; // invalid
	}
	if (ddx <  4)
	{
		//logMsg("Data set ready in host port");
		sCD.gate[4] |= 0x40; // Data set ready in host port
		return;
	}
	if (ddx == 6)
	{
		logMsg("invalid CDC DD");
		return; // invalid
	}

	Update_CDC_TRansfer(ddx); // now go and do the actual transfer
}

void scd_shutdown(void)
{
	sCD.isActive = 0;
}

void scd_update()
{
	//logMsg("scd scanline update");
	const unsigned counter75hz_lim = vdp_pal ? 2080 : 2096;

	// 75Hz CDC update
	if ((sCD.counter75hz+=10) >= counter75hz_lim)
	{
		//logMsg("CDC 75hz update");
		sCD.counter75hz -= counter75hz_lim;
		Check_CD_Command();
	}

	// update timers
	const unsigned counter_timer = vdp_pal ? 0x21630 : 0x2121c; // 136752 : 135708;
	sCD.stopwatchTimer += counter_timer;
	int int3_set;
	if ((int3_set = sCD.gate[0x31]))
	{
		sCD.timer_int3 -= counter_timer;
		if (sCD.timer_int3 < 0)
		{
			if (sCD.gate[0x33] & (1<<3))
			{
				//logMsg("s68k: timer irq 3");
				scd_interruptSubCpu(3);
				sCD.timer_int3 += int3_set << 16;
			}
			// is this really what happens if irq3 is masked out?
			//logMsg("s68k: timer irq 3 masked");
			sCD.timer_int3 &= 0xffffff;
		}
	}

	// update gfx chip
	if (sCD.rot_comp.stampDataSize & 0x8000)
		gfx_cd_update(sCD.rot_comp);

	// delayed setting of DMNA bit (needed for Silpheed)
	if(sCD.delayedDMNA)
	{
		//logMsg("set delayed DMNA");
		sCD.delayedDMNA = 0;
		if (!(sCD.gate[3] & 4))
		{
			sCD.gate[3] |=  2;
			sCD.gate[3] &= ~1;
			updateCpuWordMap(sCD);
		}
		//logMsg("DMNA (delayed): %d", sCD.gate[3] & 0x2 >> 1);
	}
	//logMsg("done");
}

int scd_loadState(uint8 *state, unsigned exVersion)
{
	logMsg("loading CD state");
	int bufferptr = 0;

	load_param(&sCD.cpu.cycleCount, sizeof(sCD.cpu.cycleCount));
  {
    uint16 tmp16;
    uint32 tmp32;
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D0, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D1, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D2, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D3, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D4, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D5, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D6, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_D7, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A0, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A1, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A2, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A3, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A4, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A5, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A6, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_A7, tmp32);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_PC, tmp32);
    load_param(&tmp16, 2); m68k_set_reg(sCD.cpu, M68K_REG_SR, tmp16);
    load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_USP,tmp32);
    if(exVersion >= 1)
		{
			load_param(&tmp32, 4); m68k_set_reg(sCD.cpu, M68K_REG_ISP,tmp32);
		}
  }

  load_param(&sCD.busreq, sizeof(sCD.busreq));
  load_param(&sCD.stopwatchTimer, 4);
  load_param(&sCD.counter75hz, 4);
  load_param(&sCD.timer_int3, 4);
  load_param(&sCD.gate, sizeof(sCD.gate));
  scd_updateCddaVol();
  load_param(&sCD.CDD_Complete, 1);
  load_param(&sCD.Status_CDD, 4);
  load_param(&sCD.Status_CDC, 4);
  load_param(&sCD.Cur_LBA, 4);
  load_param(&sCD.Cur_Track, 4);
  load_param(&sCD.File_Add_Delay, 4);
  load_param(&sCD.cdc, sizeof(sCD.cdc));
  load_param(&sCD.cdd, sizeof(sCD.cdd));

  load_param(&sCD.prg, sizeof(sCD.prg));
  load_param(&sCD.word, sizeof(sCD.word));

  load_param(&sCD.rot_comp, sizeof(sCD.rot_comp));

  load_param(&sCD.pcmMem, sizeof(sCD.pcmMem));
  load_param(&sCD.pcm, sizeof(sCD.pcm));

  load_param(&sCD.bcramReg, 1);
  load_param(&sCD.audioTrack, 1);

  load_param(cart.rom + 0x70, 4);

  load_param(&sCD.subResetPending, 1);
  load_param(&sCD.delayedDMNA, 1);

  load_param(&sCD.cddaLBA, 4);
  load_param(&sCD.cddaDataLeftover, 2);

  uint8_t reserved[24];
  load_param(reserved, 24);

  updateSegaCdMemMap(sCD);

  return bufferptr;
}

int scd_saveState(uint8 *state)
{
  int bufferptr = 0;

  save_param(&sCD.cpu.cycleCount, sizeof(sCD.cpu.cycleCount));
  {
    uint16 tmp16;
    uint32 tmp32;
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D0);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D1);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D2);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D3);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D4);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D5);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D6);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_D7);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A0);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A1);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A2);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A3);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A4);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A5);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A6);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_A7);  save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_PC);  save_param(&tmp32, 4);
    tmp16 = m68k_get_reg(sCD.cpu, M68K_REG_SR);  save_param(&tmp16, 2);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_USP); save_param(&tmp32, 4);
    tmp32 = m68k_get_reg(sCD.cpu, M68K_REG_ISP); save_param(&tmp32, 4);
  }

  save_param(&sCD.busreq, sizeof(sCD.busreq));
  save_param(&sCD.stopwatchTimer, 4);
  save_param(&sCD.counter75hz, 4);
  save_param(&sCD.timer_int3, 4);
  save_param(&sCD.gate, sizeof(sCD.gate));
  save_param(&sCD.CDD_Complete, 1);
  save_param(&sCD.Status_CDD, 4);
  save_param(&sCD.Status_CDC, 4);
  save_param(&sCD.Cur_LBA, 4);
  save_param(&sCD.Cur_Track, 4);
  save_param(&sCD.File_Add_Delay, 4);
  save_param(&sCD.cdc, sizeof(sCD.cdc));
  save_param(&sCD.cdd, sizeof(sCD.cdd));

  save_param(&sCD.prg, sizeof(sCD.prg));
  save_param(&sCD.word, sizeof(sCD.word));

  save_param(&sCD.rot_comp, sizeof(sCD.rot_comp));

  save_param(&sCD.pcmMem, sizeof(sCD.pcmMem));
  save_param(&sCD.pcm, sizeof(sCD.pcm));

  save_param(&sCD.bcramReg, 1);
  save_param(&sCD.audioTrack, 1);

  save_param(cart.rom + 0x70, 4);

  save_param(&sCD.subResetPending, 1);
  save_param(&sCD.delayedDMNA, 1);

  save_param(&sCD.cddaLBA, 4);
  save_param(&sCD.cddaDataLeftover, 2);

  uint8_t reserved[24]{};
  save_param(reserved, 24);

  return bufferptr;
}
