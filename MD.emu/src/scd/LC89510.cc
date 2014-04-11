/***********************************************************
 *                                                         *
 * This source file was taken from the Gens project        *
 * Written by Stéphane Dallongeville                       *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 * Modified/adapted for PicoDrive by notaz, 2007           *
 *                                                         *
 ***********************************************************/

#include <string.h>
#include "scd.h"
#include "cd_sys.h"
#include "misc.h"
#include <imagine/logger/logger.h>

#define cdprintf(x...)

#define CDC_DMA_SPEED 256

static void CDD_Reset(void)
{
	// Reseting CDD
	//logMsg("reset CDD");

	memset(sCD.gate+0x34, 0, 2*2); // CDD.Fader, CDD.Control
	mem_zero(sCD.cdd);

	// clear receive status and transfer command
	memset(sCD.gate+0x38, 0, 20);
	sCD.gate[0x38+9] = 0xF;		// Default checksum
}


static void CDC_Reset(void)
{
	// Reseting CDC

	memset(sCD.cdc.Buffer, 0, sizeof(sCD.cdc.Buffer));

	sCD.cdc.COMIN = 0;
	sCD.cdc.IFSTAT = 0xFF;
	sCD.cdc.DAC.N = 0;
	sCD.cdc.DBC.N = 0;
	sCD.cdc.HEAD.N = 0x01000000;
	sCD.cdc.PT.N = 0;
	sCD.cdc.WA.N = 2352 * 2;
	sCD.cdc.STAT.N = 0x00000080;
	sCD.cdc.SBOUT = 0;
	sCD.cdc.IFCTRL = 0;
	sCD.cdc.CTRL.N = 0;

	sCD.cdc.Decode_Reg_Read = 0;
	sCD.Status_CDC &= ~0x08;
}


void LC89510_Reset(void)
{
	CDD_Reset();
	CDC_Reset();

	// clear DMA_Adr & Stop_Watch
	memset(sCD.gate + 0xA, 0, 4);
}


void Update_CDC_TRansfer(int which)
{
	unsigned int DMA_Adr, dep, length;
	unsigned short *dest;
	unsigned char  *src;

	if (sCD.cdc.DBC.N <= (CDC_DMA_SPEED * 2))
	{
		logMsg("CDC DMA transfer complete");
		length = (sCD.cdc.DBC.N + 1) >> 1;
		sCD.Status_CDC &= ~0x08;	// Last transfer
		sCD.gate[4] |=  0x80;	// End data transfer
		sCD.gate[4] &= ~0x40;	// no more data ready
		sCD.cdc.IFSTAT |= 0x08;		// No more data transfer in progress

		if (sCD.cdc.IFCTRL & 0x40)	// DTEIEN = Data Trasnfer End Interrupt Enable ?
		{
			sCD.cdc.IFSTAT &= ~0x40;

			if (sCD.gate[0x33] & (1<<5))
			{
				logMsg("sCD.cdc DTE irq 5");
				scd_interruptSubCpu(5);
			}
		}
	}
	else length = CDC_DMA_SPEED;


	// TODO: dst bounds checking?
	src = sCD.cdc.Buffer + sCD.cdc.DAC.N;
	DMA_Adr = (sCD.gate[0xA]<<8) | sCD.gate[0xB];

	if (which == 7) // WORD RAM
	{
		logMsg("CDC transfer to WORD");
		if (sCD.gate[3] & 4)
		{
			// test: Final Fight
			int bank = !(sCD.gate[3]&1);
			dep = ((DMA_Adr & 0x3FFF) << 3);
			logMsg("CD DMA # %04x -> word_ram1M # %06x, len=%i",
					sCD.cdc.DAC.N, dep, length);

			dest = (unsigned short *) (sCD.word.ram1M[bank] + dep);

			memcpy16bswap(dest, src, length);

			/*{ // debug
				unsigned char *b1 = Pico_mcd->word_ram1M[bank] + dep;
				unsigned char *b2 = (unsigned char *)(dest+length) - 8;
				dprintf("%02x %02x %02x %02x .. %02x %02x %02x %02x",
					b1[0], b1[1], b1[4], b1[5], b2[0], b2[1], b2[4], b2[5]);
			}*/
		}
		else
		{
			dep = ((DMA_Adr & 0x7FFF) << 3);
			logMsg("CD DMA # %04x -> word_ram2M # %06x, len=%i",
					sCD.cdc.DAC.N, dep, length);
			dest = (unsigned short *) (sCD.word.ram2M + dep);

			memcpy16bswap(dest, src, length);

			/*{ // debug
				unsigned char *b1 = Pico_mcd->word_ram2M + dep;
				unsigned char *b2 = (unsigned char *)(dest+length) - 4;
				dprintf("%02x %02x %02x %02x .. %02x %02x %02x %02x",
					b1[0], b1[1], b1[2], b1[3], b2[0], b2[1], b2[2], b2[3]);
			}*/
		}
	}
	else if (which == 4) // PCM RAM (check: popful Mail)
	{
		logMsg("CDC transfer to PCM");
		dep = (DMA_Adr & 0x03FF) << 2;
		logMsg("CD DMA # %04x -> PCM[%i] # %04x, len=%i",
			sCD.cdc.DAC.N, sCD.pcm.bank, dep, length);
		dest = (unsigned short *) (sCD.pcmMem.bank[sCD.pcm.bank] + dep);

		if (sCD.cdc.DAC.N & 1) /* unaligned src? */
			memcpy(dest, src, length*2);
		else	memcpy16(dest, (unsigned short *) src, length);
	}
	else if (which == 5) // PRG RAM
	{
		logMsg("CDC transfer to PRG");
		dep = DMA_Adr << 3;
		dest = (unsigned short *) (sCD.prg.b + dep);
		logMsg("CD DMA # %04x -> prg_ram # %06x, len=%i",
				sCD.cdc.DAC.N, dep, length);

		memcpy16bswap(dest, src, length);

		/*{ // debug
			unsigned char *b1 = Pico_mcd->prg_ram + dep;
			unsigned char *b2 = (unsigned char *)(dest+length) - 4;
			dprintf("%02x %02x %02x %02x .. %02x %02x %02x %02x",
				b1[0], b1[1], b1[2], b1[3], b2[0], b2[1], b2[2], b2[3]);
		}*/
	}

	length <<= 1;
	sCD.cdc.DAC.N = (sCD.cdc.DAC.N + length) & 0xFFFF;
	if (sCD.Status_CDC & 0x08) sCD.cdc.DBC.N -= length;
	else sCD.cdc.DBC.N = 0;

	// update DMA_Adr
	length >>= 2;
	if (which != 4) length >>= 1;
	DMA_Adr += length;
	sCD.gate[0xA] = DMA_Adr >> 8;
	sCD.gate[0xB] = DMA_Adr;
}


unsigned short Read_CDC_Host(int is_sub)
{
	int addr;

	if (!(sCD.Status_CDC & 0x08))
	{
		// Transfer data disabled
		logMsg("Read_CDC_Host FIXME: Transfer data disabled");
		return 0;
	}

	if ((is_sub && (sCD.gate[4] & 7) != 3) ||
		(!is_sub && (sCD.gate[4] & 7) != 2))
	{
		// Wrong setting
		logMsg("Read_CDC_Host FIXME: Wrong setting");
		return 0;
	}

	sCD.cdc.DBC.N -= 2;

	if (sCD.cdc.DBC.N <= 0)
	{
		//logMsg("CDC data transfer complete");
		sCD.cdc.DBC.N = 0;
		sCD.Status_CDC &= ~0x08;		// Last transfer
		sCD.gate[4] |=  0x80;		// End data transfer
		sCD.gate[4] &= ~0x40;		// no more data ready
		sCD.cdc.IFSTAT |= 0x08;			// No more data transfer in progress

		if (sCD.cdc.IFCTRL & 0x40)		// DTEIEN = Data Transfer End Interrupt Enable ?
		{
			sCD.cdc.IFSTAT &= ~0x40;

			if (sCD.gate[0x33]&(1<<5)) {
				logMsg("m68k: s68k irq 5");
				scd_interruptSubCpu(5);
			}

			cdprintf("CDC - DTE interrupt");
		}
	}

	addr = sCD.cdc.DAC.N;
	sCD.cdc.DAC.N += 2;

	//if(((sCD.cdc.Buffer[addr]<<8) | sCD.cdc.Buffer[addr+1]) != 0)
	/*logMsg("Read_CDC_Host sub=%i d=%04x dac=%04x dbc=%04x", is_sub,
		(sCD.cdc.Buffer[addr]<<8) | sCD.cdc.Buffer[addr+1], sCD.cdc.DAC.N, sCD.cdc.DBC.N);*/

	assert(addr+1 < (int)sizeof(sCD.cdc.Buffer));
	return (sCD.cdc.Buffer[addr]<<8) | sCD.cdc.Buffer[addr+1];
}


void CDC_Update_Header(void)
{
	if (sCD.cdc.CTRL.B.B1 & 0x01)		// Sub-Header wanted ?
	{
		sCD.cdc.HEAD.B.B0 = 0;
		sCD.cdc.HEAD.B.B1 = 0;
		sCD.cdc.HEAD.B.B2 = 0;
		sCD.cdc.HEAD.B.B3 = 0;
	}
	else
	{
		_msf MSF;

		LBA_to_MSF(sCD.Cur_LBA, &MSF);

		sCD.cdc.HEAD.B.B0 = INT_TO_BCDB(MSF.M);
		sCD.cdc.HEAD.B.B1 = INT_TO_BCDB(MSF.S);
		sCD.cdc.HEAD.B.B2 = INT_TO_BCDB(MSF.F);
		sCD.cdc.HEAD.B.B3 = 0x01;
	}
}


unsigned char CDC_Read_Reg(void)
{
	unsigned char ret;

	switch(sCD.gate[5] & 0xF)
	{
		case 0x0: // COMIN
			cdprintf("CDC read reg 00 = %.2X", sCD.cdc.COMIN);

			sCD.gate[5] = 0x1;
			return sCD.cdc.COMIN;

		case 0x1: // IFSTAT
			cdprintf("CDC read reg 01 = %.2X", sCD.cdc.IFSTAT);

			sCD.cdc.Decode_Reg_Read |= (1 << 1);		// Reg 1 (decoding)
			sCD.gate[5] = 0x2;
			return sCD.cdc.IFSTAT;

		case 0x2: // DBCL
			cdprintf("CDC read reg 02 = %.2X", sCD.cdc.DBC.B.L);

			sCD.gate[5] = 0x3;
			return sCD.cdc.DBC.B.L;

		case 0x3: // DBCH
			cdprintf("CDC read reg 03 = %.2X", sCD.cdc.DBC.B.H);

			sCD.gate[5] = 0x4;
			return sCD.cdc.DBC.B.H;

		case 0x4: // HEAD0
			cdprintf("CDC read reg 04 = %.2X", sCD.cdc.HEAD.B.B0);

			sCD.cdc.Decode_Reg_Read |= (1 << 4);		// Reg 4 (decoding)
			sCD.gate[5] = 0x5;
			return sCD.cdc.HEAD.B.B0;

		case 0x5: // HEAD1
			cdprintf("CDC read reg 05 = %.2X", sCD.cdc.HEAD.B.B1);

			sCD.cdc.Decode_Reg_Read |= (1 << 5);		// Reg 5 (decoding)
			sCD.gate[5] = 0x6;
			return sCD.cdc.HEAD.B.B1;

		case 0x6: // HEAD2
			cdprintf("CDC read reg 06 = %.2X", sCD.cdc.HEAD.B.B2);

			sCD.cdc.Decode_Reg_Read |= (1 << 6);		// Reg 6 (decoding)
			sCD.gate[5] = 0x7;
			return sCD.cdc.HEAD.B.B2;

		case 0x7: // HEAD3
			cdprintf("CDC read reg 07 = %.2X", sCD.cdc.HEAD.B.B3);

			sCD.cdc.Decode_Reg_Read |= (1 << 7);		// Reg 7 (decoding)
			sCD.gate[5] = 0x8;
			return sCD.cdc.HEAD.B.B3;

		case 0x8: // PTL
			cdprintf("CDC read reg 08 = %.2X", sCD.cdc.PT.B.L);

			sCD.cdc.Decode_Reg_Read |= (1 << 8);		// Reg 8 (decoding)
			sCD.gate[5] = 0x9;
			return sCD.cdc.PT.B.L;

		case 0x9: // PTH
			cdprintf("CDC read reg 09 = %.2X", sCD.cdc.PT.B.H);

			sCD.cdc.Decode_Reg_Read |= (1 << 9);		// Reg 9 (decoding)
			sCD.gate[5] = 0xA;
			return sCD.cdc.PT.B.H;

		case 0xA: // WAL
			cdprintf("CDC read reg 10 = %.2X", sCD.cdc.WA.B.L);

			sCD.gate[5] = 0xB;
			return sCD.cdc.WA.B.L;

		case 0xB: // WAH
			cdprintf("CDC read reg 11 = %.2X", sCD.cdc.WA.B.H);

			sCD.gate[5] = 0xC;
			return sCD.cdc.WA.B.H;

		case 0xC: // STAT0
			cdprintf("CDC read reg 12 = %.2X", sCD.cdc.STAT.B.B0);

			sCD.cdc.Decode_Reg_Read |= (1 << 12);		// Reg 12 (decoding)
			sCD.gate[5] = 0xD;
			return sCD.cdc.STAT.B.B0;

		case 0xD: // STAT1
			cdprintf("CDC read reg 13 = %.2X", sCD.cdc.STAT.B.B1);

			sCD.cdc.Decode_Reg_Read |= (1 << 13);		// Reg 13 (decoding)
			sCD.gate[5] = 0xE;
			return sCD.cdc.STAT.B.B1;

		case 0xE: // STAT2
			cdprintf("CDC read reg 14 = %.2X", sCD.cdc.STAT.B.B2);

			sCD.cdc.Decode_Reg_Read |= (1 << 14);		// Reg 14 (decoding)
			sCD.gate[5] = 0xF;
			return sCD.cdc.STAT.B.B2;

		case 0xF: // STAT3
			cdprintf("CDC read reg 15 = %.2X", sCD.cdc.STAT.B.B3);

			ret = sCD.cdc.STAT.B.B3;
			sCD.cdc.IFSTAT |= 0x20;			// decoding interrupt flag cleared
			if ((sCD.cdc.CTRL.B.B0 & 0x80) && (sCD.cdc.IFCTRL & 0x20))
			{
				if ((sCD.cdc.Decode_Reg_Read & 0x73F2) == 0x73F2)
					sCD.cdc.STAT.B.B3 = 0x80;
			}
			return ret;
	}

	return 0;
}

bool cdcTransferActive = 0;


void CDC_Write_Reg(unsigned char Data)
{
	//logMsg("CDC write reg%02d = %.2X", sCD.gate[5] & 0xF, Data);

	switch (sCD.gate[5] & 0xF)
	{
		case 0x0: // SBOUT
			sCD.gate[5] = 0x1;
			sCD.cdc.SBOUT = Data;

			break;

		case 0x1: // IFCTRL
			sCD.gate[5] = 0x2;
			sCD.cdc.IFCTRL = Data;

			if ((sCD.cdc.IFCTRL & 0x02) == 0)		// Stop data transfer
			{
				sCD.cdc.DBC.N = 0;
				sCD.Status_CDC &= ~0x08;
				sCD.cdc.IFSTAT |= 0x08;		// No more data transfer in progress
			}
			break;

		case 0x2: // DBCL
			sCD.gate[5] = 0x3;
			sCD.cdc.DBC.B.L = Data;

			break;

		case 0x3: // DBCH
			sCD.gate[5] = 0x4;
			sCD.cdc.DBC.B.H = Data;

			break;

		case 0x4: // DACL
			sCD.gate[5] = 0x5;
			sCD.cdc.DAC.B.L = Data;

			break;

		case 0x5: // DACH
			sCD.gate[5] = 0x6;
			sCD.cdc.DAC.B.H = Data;

			break;

		case 0x6: // DTTRG
			if (sCD.cdc.IFCTRL & 0x02)		// Data transfer enable ?
			{
				sCD.cdc.IFSTAT &= ~0x08;		// Data transfer in progress
				sCD.Status_CDC |= 0x08;	// Data transfer in progress
				sCD.gate[4] &= 0x7F;		// A data transfer start

				/*logMsg("Data Transfer, RS0 = %.4X  DAC = %.4X  DBC = %.4X  DMA adr = %.4X", sCD.gate[4]<<8,
					sCD.cdc.DAC.N, sCD.cdc.DBC.N, (sCD.gate[0xA]<<8) | sCD.gate[0xB]);*/
				cdcTransferActive = 1;
			}
			break;

		case 0x7: // DTACK
			sCD.cdc.IFSTAT |= 0x40;			// end data transfer interrupt flag cleared
			break;

		case 0x8: // WAL
			sCD.gate[5] = 0x9;
			sCD.cdc.WA.B.L = Data;

			break;

		case 0x9: // WAH
			sCD.gate[5] = 0xA;
			sCD.cdc.WA.B.H = Data;

			break;

		case 0xA: // CTRL0
			sCD.gate[5] = 0xB;
			sCD.cdc.CTRL.B.B0 = Data;

			break;

		case 0xB: // CTRL1
			sCD.gate[5] = 0xC;
			sCD.cdc.CTRL.B.B1 = Data;

			break;

		case 0xC: // PTL
			sCD.gate[5] = 0xD;
			sCD.cdc.PT.B.L = Data;

			break;

		case 0xD: // PTH
			sCD.gate[5] = 0xE;
			sCD.cdc.PT.B.H = Data;

			break;

		case 0xE: // CTRL2
			sCD.cdc.CTRL.B.B2 = Data;
			break;

		case 0xF: // RESET
			CDC_Reset();
			break;
	}
}


static int bswapwrite(int a, unsigned short d)
{
	//*(unsigned short *)(sCD.gate + a) = (d>>8)|(d<<8);
	uint16a *addr = (uint16a*)(sCD.gate + a);
	*addr = (d>>8)|(d<<8);
	return d + (d >> 8);
}

void CDD_Export_Status(void)
{
	unsigned int csum;

	csum  = bswapwrite( 0x38+0, sCD.cdd.status);
	csum += bswapwrite( 0x38+2, sCD.cdd.Minute);
	csum += bswapwrite( 0x38+4, sCD.cdd.Seconde);
	csum += bswapwrite( 0x38+6, sCD.cdd.Frame);
	sCD.gate[0x38+8] = sCD.cdd.Ext;
	csum += sCD.cdd.Ext;
	sCD.gate[0x38+9] = ~csum & 0xf;

	sCD.gate[0x37] &= 3; // CDD.Control

	if (sCD.gate[0x33] & (1<<4))
	{
		//logMsg("CDD export irq 4");
		scd_interruptSubCpu(4);
	}

//	cdprintf("CDD exported status\n");
	cdprintf("out:  Status=%.4X, Minute=%.4X, Second=%.4X, Frame=%.4X  Checksum=%.4X",
		(sCD.gate[0x38+0] << 8) | sCD.gate[0x38+1],
		(sCD.gate[0x38+2] << 8) | sCD.gate[0x38+3],
		(sCD.gate[0x38+4] << 8) | sCD.gate[0x38+5],
		(sCD.gate[0x38+6] << 8) | sCD.gate[0x38+7],
		(sCD.gate[0x38+8] << 8) | sCD.gate[0x38+9]);
}


void CDD_Import_Command(void)
{
//	cdprintf("CDD importing command\n");
	cdprintf("in:  Command=%.4X, Minute=%.4X, Second=%.4X, Frame=%.4X  Checksum=%.4X",
		(sCD.gate[0x38+10+0] << 8) | sCD.gate[0x38+10+1],
		(sCD.gate[0x38+10+2] << 8) | sCD.gate[0x38+10+3],
		(sCD.gate[0x38+10+4] << 8) | sCD.gate[0x38+10+5],
		(sCD.gate[0x38+10+6] << 8) | sCD.gate[0x38+10+7],
		(sCD.gate[0x38+10+8] << 8) | sCD.gate[0x38+10+9]);

	switch (sCD.gate[0x38+10+0])
	{
		case 0x0:	// STATUS (?)
			//logMsg("CDD: status");
			Get_Status_CDD_c0();
			break;

		case 0x1:	// STOP ALL (?)
			//logMsg("CDD: stop");
			Stop_CDD_c1();
			break;

		case 0x2:	// GET TOC INFORMATIONS
			switch(sCD.gate[0x38+10+3])
			{
				case 0x0:	// get current position (MSF format)
					//logMsg("CDD: toc: pos");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00);
					Get_Pos_CDD_c20();
					break;

				case 0x1:	// get elapsed time of current track played/scanned (relative MSF format)
					//logMsg("CDD: toc: elapsed time");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00) | 1;
					Get_Track_Pos_CDD_c21();
					break;

				case 0x2:	// get current track in RS2-RS3
					//logMsg("CDD: toc: rs2-rs3");
					sCD.cdd.status =  (sCD.cdd.status & 0xFF00) | 2;
					Get_Current_Track_CDD_c22();
					break;

				case 0x3:	// get total length (MSF format)
					//logMsg("CDD: toc: length");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00) | 3;
					Get_Total_Lenght_CDD_c23();
					break;

				case 0x4:	// first & last track number
					//logMsg("CDD: toc: first last track num");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00) | 4;
					Get_First_Last_Track_CDD_c24();
					break;

				case 0x5:	// get track addresse (MSF format)
					//logMsg("CDD: toc: track address");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00) | 5;
					Get_Track_Adr_CDD_c25();
					break;

				default :	// invalid, then we return status
					//logMsg("CDD: toc: invalid");
					sCD.cdd.status = (sCD.cdd.status & 0xFF00) | 0xF;
					Get_Status_CDD_c0();
					break;
			}
			break;

		case 0x3:	// READ
			//logMsg("CDD: read");
			Play_CDD_c3();
			break;

		case 0x4:	// SEEK
			//logMsg("CDD: seek");
			Seek_CDD_c4();
			break;

		case 0x6:	// PAUSE/STOP
			//logMsg("CDD: pause");
			Pause_CDD_c6();
			break;

		case 0x7:	// RESUME
			//logMsg("CDD: resume");
			Resume_CDD_c7();
			break;

		case 0x8:	// FAST FOWARD
			//logMsg("CDD: ff");
			Fast_Foward_CDD_c8();
			break;

		case 0x9:	// FAST REWIND
			//logMsg("CDD: fr");
			Fast_Rewind_CDD_c9();
			break;

		case 0xA:	// RECOVER INITIAL STATE (?)
			//logMsg("CDD: recover");
			CDD_cA();
			break;

		case 0xC:	// CLOSE TRAY
			//logMsg("CDD: close tray");
			Close_Tray_CDD_cC();
			break;

		case 0xD:	// OPEN TRAY
			//logMsg("CDD: open tray");
			Open_Tray_CDD_cD();
			break;

		default:	// UNKNOWN
			logMsg("CDD: unknown");
			CDD_Def();
			break;
	}
}

