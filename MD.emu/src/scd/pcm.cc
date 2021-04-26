// Emulation routines for the RF5C164 PCM chip.
// Based on Gens code by Stéphane Dallongeville
// (c) Copyright 2007, Grazvydas "notaz" Ignotas

#include "scd.h"
#include "pcm.h"
#include <imagine/logger/logger.h>
#include <imagine/util/mayAliasInt.h>

static unsigned int g_rate = 0; // 18.14 fixed point

void pcm_write(unsigned int a, unsigned int d)
{
//logMsg("pcm_write(%i, %02x)", a, d);

	if (a < 7)
	{
		sCD.pcm.ch[sCD.pcm.cur_ch].regs[a] = d;
	}
	else if (a == 7) // control register
	{
		if (d & 0x40)	sCD.pcm.cur_ch = d & 7;
		else
		{
			sCD.pcm.bank = d & 0xf;
			//logMsg("set pcm bank %d", sCD.pcm.bank);
		}
		sCD.pcm.control = d;
		// logMsg("pcm control=%02x", sCD.pcm.control);
	}
	else if (a == 8) // sound on/off
	{
		if (!(sCD.pcm.enabled & 0x01)) sCD.pcm.ch[0].addr =
			sCD.pcm.ch[0].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x02)) sCD.pcm.ch[1].addr =
			sCD.pcm.ch[1].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x04)) sCD.pcm.ch[2].addr =
			sCD.pcm.ch[2].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x08)) sCD.pcm.ch[3].addr =
			sCD.pcm.ch[3].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x10)) sCD.pcm.ch[4].addr =
			sCD.pcm.ch[4].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x20)) sCD.pcm.ch[5].addr =
			sCD.pcm.ch[5].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x40)) sCD.pcm.ch[6].addr =
			sCD.pcm.ch[6].regs[6] << (PCM_STEP_SHIFT + 8);
		if (!(sCD.pcm.enabled & 0x80)) sCD.pcm.ch[7].addr =
			sCD.pcm.ch[7].regs[6] << (PCM_STEP_SHIFT + 8);
//		logMsg("addr %x %x %x %x %x %x %x %x", sCD.pcm.ch[0].addr, sCD.pcm.ch[1].addr
//		, sCD.pcm.ch[2].addr, sCD.pcm.ch[3].addr, sCD.pcm.ch[4].addr, sCD.pcm.ch[5].addr
//		, sCD.pcm.ch[6].addr, sCD.pcm.ch[7].addr);

		sCD.pcm.enabled = ~d;
//logMsg("enabled=%02x", sCD.pcm.enabled);
	}
}


void scd_pcm_setRate(int rate)
{
	float step = 31.8 * 1024.0 / (float) rate; // max <4 @ 8000Hz
	step *= 256*256/4;
	g_rate = (unsigned int) step;
	if (step - (float) g_rate >= 0.5) g_rate++;
	//logMsg("g_rate: %f %08x", (double)step, g_rate);
}


void scd_pcm_update(PCMSampleType *buffer, int length, int stereo)
{
	SegaCD::PCM::Channel *ch;
	unsigned int step, addr;
	int mul_l, mul_r, smp;
	int i, j, k;
	PCMSampleType *out;


	// PCM disabled or all channels off (to be checked by caller)
	//if (!(sCD.pcm.control & 0x80) || !sCD.pcm.enabled) return;

	unsigned activeCH = 0;

	for (i = 0; i < 8; i++)
	{
		if (!(sCD.pcm.enabled & (1 << i))) continue; // channel disabled
		activeCH++;
		//logMsg("pcm ch %d", i);

		out = buffer;
		ch = &sCD.pcm.ch[i];

		addr = ch->addr; // >> PCM_STEP_SHIFT;
		mul_l = ((int)ch->regs[0] * (ch->regs[1] & 0xf)) >> (5+1); // (env * pan) >> 5
		mul_r = ((int)ch->regs[0] * (ch->regs[1] >>  4)) >> (5+1);
		step  = ((unsigned int)(*(uint16a*)&ch->regs[2]) * g_rate) >> 14; // freq step
//		logMsg("step=%i, cstep=%i, mul_l=%i, mul_r=%i, ch=%i, addr=%x, en=%02x",
//			*(unsigned short *)&ch->regs[2], step, mul_l, mul_r, i, addr, sCD.pcm.enabled);

		if (!stereo && mul_l < mul_r) mul_l = mul_r;

		for (j = 0; j < length; j++)
		{
//			logMsg("addr=%08x", addr);
			smp = sCD.pcmMem.b[addr >> PCM_STEP_SHIFT];

			// test for loop signal
			if (smp == 0xff)
			{
				addr = *(uint16a*)&ch->regs[4]; // loop_addr
				smp = sCD.pcmMem.b[addr];
				addr <<= PCM_STEP_SHIFT;
				if (smp == 0xff) break;
			}

			if (smp & 0x80) smp = -(smp & 0x7f);

			//if(j == 0)
				//logMsg("pcm out %d", smp * mul_l);
			if(activeCH == 1)
				*out++ = smp * mul_l; // max 128 * 119 = 15232
			else
				*out++ += smp * mul_l; // max 128 * 119 = 15232

			if(stereo)
			{
				if(activeCH == 1)
					*out++ = smp * mul_r;
				else
					*out++ += smp * mul_r;
			}

			// update address register
			k = (addr >> PCM_STEP_SHIFT) + 1;
			addr = (addr + step) & 0x7FFFFFF;

			for(; k < int(addr >> PCM_STEP_SHIFT); k++)
			{
				if (sCD.pcmMem.b[k] == 0xff)
				{
					addr = (unsigned int)(*(uint16a*)&ch->regs[4]) << PCM_STEP_SHIFT; // loop_addr
					break;
				}
			}
		}

		if (sCD.pcmMem.b[addr >> PCM_STEP_SHIFT] == 0xff)
			addr = (unsigned int)(*(uint16a*)&ch->regs[4]) << PCM_STEP_SHIFT; // loop_addr

		ch->addr = addr;
	}
	//logMsg("%d active channels", activeCH);
}

