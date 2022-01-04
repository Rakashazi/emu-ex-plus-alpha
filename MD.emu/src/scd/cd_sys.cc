/***********************************************************
 *                                                         *
 * This source file was taken from the Gens project        *
 * Written by Stéphane Dallongeville                       *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 * Modified/adapted for PicoDrive by notaz, 2007           *
 *                                                         *
 ***********************************************************/

#include <stdio.h>

#include <imagine/logger/logger.h>
#include "scd.h"
#include "cd_sys.h"
#include "cd_file.h"
#include <mednafen/mednafen.h>
#include <mednafen/cdrom/CDAccess.h>

#define cdprintf(x...)
//#define DEBUG_CD

#define TRAY_OPEN	0x0500		// TRAY OPEN CDD status
#define NOCD		0x0000		// CD removed CDD status
#define STOPPED		0x0900		// STOPPED CDD status (happen after stop or close tray command)
#define READY		0x0400		// READY CDD status (also used for seeking)
#define FAST_FOW	0x0300		// FAST FORWARD track CDD status
#define FAST_REV	0x10300		// FAST REVERSE track CDD status
#define PLAYING		0x0100		// PLAYING audio track CDD status


static int CD_Present = 0;


#define CHECK_TRAY_OPEN				\
if (sCD.Status_CDD == TRAY_OPEN)	\
{									\
	sCD.cdd.status = sCD.Status_CDD;	\
									\
	sCD.cdd.Minute = 0;					\
	sCD.cdd.Seconde = 0;				\
	sCD.cdd.Frame = 0;					\
	sCD.cdd.Ext = 0;					\
									\
	sCD.CDD_Complete = 1;				\
									\
	return 2;						\
}


#define CHECK_CD_PRESENT			\
if (!CD_Present)					\
{									\
	sCD.Status_CDD = NOCD;			\
	sCD.cdd.status = sCD.Status_CDD;	\
									\
	sCD.cdd.Minute = 0;					\
	sCD.cdd.Seconde = 0;				\
	sCD.cdd.Frame = 0;					\
	sCD.cdd.Ext = 0;					\
									\
	sCD.CDD_Complete = 1;				\
									\
	return 3;						\
}


static int MSF_to_LBA(_msf *MSF)
{
	return (MSF->M * 60 * 75) + (MSF->S * 75) + MSF->F - 150;
}


void LBA_to_MSF(int lba, _msf *MSF)
{
	if (lba < -150) lba = 0;
	else lba += 150;
	MSF->M = lba / (60 * 75);
	MSF->S = (lba / 75) % 60;
	MSF->F = lba % 75;
}


static unsigned int MSF_to_Track(_msf *MSF)
{
	unsigned i, Start, Cur;

	Start = (MSF->M << 16) + (MSF->S << 8) + MSF->F;

	for(i = 1; i <= (sCD.TOC.Last_Track + 1); i++)
	{
		Cur = sCD.TOC.Tracks[i - 1].MSF.M << 16;
		Cur += sCD.TOC.Tracks[i - 1].MSF.S << 8;
		Cur += sCD.TOC.Tracks[i - 1].MSF.F;

		if (Cur > Start) break;
	}

	--i;

	if (i > sCD.TOC.Last_Track) return 100;
	else if (i < 1) i = 1;

	return (unsigned) i;
}


static unsigned int LBA_to_Track(int lba)
{
	_msf MSF;

	LBA_to_MSF(lba, &MSF);
	return MSF_to_Track(&MSF);
}


static void Track_to_MSF(int track, _msf *MSF)
{
	if (track < 1) track = 1;
	else if (track > (int)sCD.TOC.Last_Track) track = sCD.TOC.Last_Track;

	MSF->M = sCD.TOC.Tracks[track - 1].MSF.M;
	MSF->S = sCD.TOC.Tracks[track - 1].MSF.S;
	MSF->F = sCD.TOC.Tracks[track - 1].MSF.F;
}


int Track_to_LBA(int track)
{
	_msf MSF;

	Track_to_MSF(track, &MSF);
	return MSF_to_LBA(&MSF);
}


void Check_CD_Command(void)
{
	//logMsg("CHECK CD COMMAND");

	// Check CDC
	if (sCD.Status_CDC & 1)			// CDC is reading data ...
	{
		//logMsg("Got a read command");

		// DATA ?
		if (sCD.Cur_Track == 1)
		     sCD.gate[0x36] |=  0x01;
		else sCD.gate[0x36] &= ~0x01;			// AUDIO

		if (sCD.File_Add_Delay == 0)
		{
			FILE_Read_One_LBA_CDC();
		}
		else sCD.File_Add_Delay--;
	}

	// Check CDD
	if (sCD.CDD_Complete)
	{
		//logMsg("CDD cmd complete");
		sCD.CDD_Complete = 0;

		CDD_Export_Status();
	}

	if (sCD.Status_CDD == FAST_FOW)
	{
		logMsg("updating FF");
		sCD.Cur_LBA += 10;
		CDC_Update_Header();

	}
	else if (sCD.Status_CDD == FAST_REV)
	{
		logMsg("updating FR");
		sCD.Cur_LBA -= 10;
		if (sCD.Cur_LBA < -150) sCD.Cur_LBA = -150;
		CDC_Update_Header();
	}
}


int Init_CD_Driver(void)
{
	return 0;
}


void End_CD_Driver(void)
{
	Unload_ISO();
}


void Reset_CD(void)
{
	sCD.Cur_Track = 0;
	sCD.Cur_LBA = -150;
	sCD.Status_CDC &= ~1;
	sCD.Status_CDD = CD_Present ? READY : NOCD;
	sCD.CDD_Complete = 0;
	sCD.File_Add_Delay = 0;
}


int Insert_CD(Mednafen::CDAccess *cd)
{
	int ret = 0;

	CD_Present = 0;
	sCD.Status_CDD = NOCD;

	if(cd)
	{
		ret = Load_ISO(cd);
		if(ret == 0)
		{
			CD_Present = 1;
			sCD.Status_CDD = READY;
		}
	}

	return ret;
}


void Stop_CD(void)
{
	Unload_ISO();
	CD_Present = 0;
}


/*
void Change_CD(void)
{
	if (sCD.Status_CDD == TRAY_OPEN) Close_Tray_CDD_cC();
	else Open_Tray_CDD_cD();
}
*/

int Get_Status_CDD_c0(void)
{
	//logMsg("Status command : Cur LBA = %d, status %d", sCD.Cur_LBA, sCD.Status_CDD);

	// Clear immediat status
	if ((sCD.cdd.status & 0x0F00) == 0x0200)
		sCD.cdd.status = (sCD.Status_CDD & 0xFF00) | (sCD.cdd.status & 0x00FF);
	else if ((sCD.cdd.status & 0x0F00) == 0x0700)
		sCD.cdd.status = (sCD.Status_CDD & 0xFF00) | (sCD.cdd.status & 0x00FF);
	else if ((sCD.cdd.status & 0x0F00) == 0x0E00)
		sCD.cdd.status = (sCD.Status_CDD & 0xFF00) | (sCD.cdd.status & 0x00FF);

	//logMsg("issued CDD Status %d %d", sCD.Status_CDD, sCD.cdd.status);
	sCD.CDD_Complete = 1;

	return 0;
}


int Stop_CDD_c1(void)
{
	logMsg("issued CDD Stop");
	CHECK_TRAY_OPEN

	sCD.Status_CDC &= ~1;				// Stop CDC read

	if (CD_Present) sCD.Status_CDD = STOPPED;
	else sCD.Status_CDD = NOCD;
	sCD.cdd.status = 0x0000;

	sCD.gate[0x36] |= 0x01;			// Data bit set because stopped

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	sCD.CDD_Complete = 1;

	sCD.audioTrack = 0;

	return 0;
}


int Get_Pos_CDD_c20(void)
{
	//1 logMsg("issued CDD Get Pos");
	_msf MSF;

	cdprintf("command 200 : Cur LBA = %d", sCD.Cur_LBA);

	CHECK_TRAY_OPEN

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		sCD.Status_CDD = NOCD;
		sCD.cdd.status |= sCD.Status_CDD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	cdprintf("Status CDD = %.4X  Status = %.4X", sCD.Status_CDD, sCD.cdd.status);

	LBA_to_MSF(sCD.Cur_LBA, &MSF);

	sCD.cdd.Minute = INT_TO_BCDW(MSF.M);
	sCD.cdd.Seconde = INT_TO_BCDW(MSF.S);
	sCD.cdd.Frame = INT_TO_BCDW(MSF.F);
	sCD.cdd.Ext = 0;

	sCD.CDD_Complete = 1;

	return 0;
}


int Get_Track_Pos_CDD_c21(void)
{
	// 1 logMsg("issued CDD Track Pos");
	int elapsed_time;
	_msf MSF;

	cdprintf("command 201 : Cur LBA = %d", sCD.Cur_LBA);

	CHECK_TRAY_OPEN

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		sCD.Status_CDD = NOCD;
		sCD.cdd.status |= sCD.Status_CDD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	elapsed_time = sCD.Cur_LBA - Track_to_LBA(LBA_to_Track(sCD.Cur_LBA));
	LBA_to_MSF(elapsed_time - 150, &MSF);

	cdprintf("   elapsed = %d", elapsed_time);

	sCD.cdd.Minute = INT_TO_BCDW(MSF.M);
	sCD.cdd.Seconde = INT_TO_BCDW(MSF.S);
	sCD.cdd.Frame = INT_TO_BCDW(MSF.F);
	sCD.cdd.Ext = 0;

	sCD.CDD_Complete = 1;

	return 0;
}


int Get_Current_Track_CDD_c22(void)
{
	// 1 logMsg("issued CDD Get Curr Track");
	cdprintf("Status CDD = %.4X  Status = %.4X", sCD.Status_CDD, sCD.cdd.status);

	CHECK_TRAY_OPEN

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		sCD.Status_CDD = NOCD;
		sCD.cdd.status |= sCD.Status_CDD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	sCD.Cur_Track = LBA_to_Track(sCD.Cur_LBA);

	if (sCD.Cur_Track == 100) sCD.cdd.Minute = 0x0A02;
	else sCD.cdd.Minute = INT_TO_BCDW(sCD.Cur_Track);
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	sCD.CDD_Complete = 1;

	return 0;
}


int Get_Total_Lenght_CDD_c23(void)
{
	logMsg("issued CDD Total Len");
	CHECK_TRAY_OPEN

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		sCD.Status_CDD = NOCD;
		sCD.cdd.status |= sCD.Status_CDD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	sCD.cdd.Minute = INT_TO_BCDW(sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.M);
	sCD.cdd.Seconde = INT_TO_BCDW(sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.S);
	sCD.cdd.Frame = INT_TO_BCDW(sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.F);
	sCD.cdd.Ext = 0;

	logMsg("track is %d.%d.%d long", sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.M, sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.S, sCD.TOC.Tracks[sCD.TOC.Last_Track].MSF.F);

	sCD.CDD_Complete = 1;

	return 0;
}


int Get_First_Last_Track_CDD_c24(void)
{
	CHECK_TRAY_OPEN

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		logMsg("no CD present in Get_First_Last_Track_CDD_c24");
		sCD.Status_CDD = NOCD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	sCD.cdd.Minute = INT_TO_BCDW(1);
	sCD.cdd.Seconde = INT_TO_BCDW(sCD.TOC.Last_Track);
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued First/Last track %d", sCD.TOC.Last_Track);

	sCD.CDD_Complete = 1;

	return 0;
}


int Get_Track_Adr_CDD_c25(void)
{
	logMsg("issued Track Addr");
	int track_number;

	CHECK_TRAY_OPEN

	// track number in TC4 & TC5

	track_number = (sCD.gate[0x38+10+4] & 0xF) * 10 + (sCD.gate[0x38+10+5] & 0xF);

	sCD.cdd.status &= 0xFF;
	if (!CD_Present)
	{
		logMsg("no CD present in Get_Track_Adr_CDD_c25");
		sCD.Status_CDD = NOCD;
		sCD.cdd.status |= sCD.Status_CDD;
	}
//	else if (!(CDC.CTRL.B.B0 & 0x80)) sCD.cdd.status |= sCD.Status_CDD;
	sCD.cdd.status |= sCD.Status_CDD;

	if (track_number > (int)sCD.TOC.Last_Track) track_number = sCD.TOC.Last_Track;
	else if (track_number < 1) track_number = 1;

	sCD.cdd.Minute = INT_TO_BCDW(sCD.TOC.Tracks[track_number - 1].MSF.M);
	sCD.cdd.Seconde = INT_TO_BCDW(sCD.TOC.Tracks[track_number - 1].MSF.S);
	sCD.cdd.Frame = INT_TO_BCDW(sCD.TOC.Tracks[track_number - 1].MSF.F);
	sCD.cdd.Ext = track_number % 10;

	if (track_number == 1) sCD.cdd.Frame |= 0x0800; // data track

	sCD.CDD_Complete = 1;
	return 0;
}


int Play_CDD_c3(void)
{
	_msf MSF;
	int delay, new_lba;

	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	// MSF of the track to play in TC buffer

	MSF.M = (sCD.gate[0x38+10+2] & 0xF) * 10 + (sCD.gate[0x38+10+3] & 0xF);
	MSF.S = (sCD.gate[0x38+10+4] & 0xF) * 10 + (sCD.gate[0x38+10+5] & 0xF);
	MSF.F = (sCD.gate[0x38+10+6] & 0xF) * 10 + (sCD.gate[0x38+10+7] & 0xF);

	sCD.Cur_Track = MSF_to_Track(&MSF);

	new_lba = MSF_to_LBA(&MSF);
	delay = new_lba - sCD.Cur_LBA;
	if (delay < 0) delay = -delay;
	delay >>= 12;

	sCD.Cur_LBA = new_lba;
	CDC_Update_Header();

	//logMsg("Read : Cur LBA = %d, M=%d, S=%d, F=%d", sCD.Cur_LBA, MSF.M, MSF.S, MSF.F);

	if (sCD.Status_CDD != PLAYING) delay += 20;

	sCD.Status_CDD = PLAYING;
	sCD.cdd.status = 0x0102;
//	sCD.cdd.status = COMM_OK;

	if (sCD.File_Add_Delay == 0) sCD.File_Add_Delay = delay;

	if (sCD.Cur_Track == 1)
	{
		sCD.gate[0x36] |=  0x01;				// DATA
		sCD.audioTrack = 0;
	}
	else
	{
		sCD.gate[0x36] &= ~0x01;				// AUDIO
		//CD_Audio_Starting = 1;
		FILE_Play_CD_LBA();
	}

	if (sCD.Cur_Track == 100) sCD.cdd.Minute = 0x0A02;
	else sCD.cdd.Minute = INT_TO_BCDW(sCD.Cur_Track);
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	sCD.Status_CDC |= 1;			// Read data with CDC

	logMsg("issued CDD Play");
	sCD.CDD_Complete = 1;
	return 0;
}


int Seek_CDD_c4(void)
{
	_msf MSF;

	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	// MSF to seek in TC buffer

	MSF.M = (sCD.gate[0x38+10+2] & 0xF) * 10 + (sCD.gate[0x38+10+3] & 0xF);
	MSF.S = (sCD.gate[0x38+10+4] & 0xF) * 10 + (sCD.gate[0x38+10+5] & 0xF);
	MSF.F = (sCD.gate[0x38+10+6] & 0xF) * 10 + (sCD.gate[0x38+10+7] & 0xF);

	sCD.Cur_Track = MSF_to_Track(&MSF);
	sCD.Cur_LBA = MSF_to_LBA(&MSF);
	CDC_Update_Header();

	sCD.Status_CDC &= ~1;				// Stop CDC read

	sCD.Status_CDD = READY;
	sCD.cdd.status = 0x0200;

	// DATA ?
	if (sCD.Cur_Track == 1)
	     sCD.gate[0x36] |=  0x01;
	else sCD.gate[0x36] &= ~0x01;		// AUDIO

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD Seek");
	sCD.CDD_Complete = 1;

	return 0;
}


int Pause_CDD_c6(void)
{
	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	sCD.Status_CDC &= ~1;			// Stop CDC read to start a new one if raw data

	sCD.Status_CDD = READY;
	sCD.cdd.status = sCD.Status_CDD;

	sCD.gate[0x36] |= 0x01;		// Data bit set because stopped

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD Pause");
	sCD.CDD_Complete = 1;

	return 0;
}


int Resume_CDD_c7(void)
{
	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	sCD.Cur_Track = LBA_to_Track(sCD.Cur_LBA);

#ifdef DEBUG_CD
	{
		_msf MSF;
		LBA_to_MSF(sCD.Cur_LBA, &MSF);
		cdprintf("Resume read : Cur LBA = %d, M=%d, S=%d, F=%d", sCD.Cur_LBA, MSF.M, MSF.S, MSF.F);
	}
#endif

	sCD.Status_CDD = PLAYING;
	sCD.cdd.status = 0x0102;

	if (sCD.Cur_Track == 1)
	{
		sCD.gate[0x36] |=  0x01;				// DATA
	}
	else
	{
		sCD.gate[0x36] &= ~0x01;				// AUDIO
		//CD_Audio_Starting = 1;
		FILE_Play_CD_LBA();
	}

	if (sCD.Cur_Track == 100) sCD.cdd.Minute = 0x0A02;
	else sCD.cdd.Minute = INT_TO_BCDW(sCD.Cur_Track);
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	sCD.Status_CDC |= 1;			// Read data with CDC

	logMsg("issued CDD Resume");
	sCD.CDD_Complete = 1;
	return 0;
}


int Fast_Foward_CDD_c8(void)
{
	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	sCD.Status_CDC &= ~1;				// Stop CDC read

	sCD.Status_CDD = FAST_FOW;
	sCD.cdd.status = sCD.Status_CDD | 2;

	sCD.cdd.Minute = INT_TO_BCDW(sCD.Cur_Track);
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD FF");
	sCD.CDD_Complete = 1;

	return 0;
}


int Fast_Rewind_CDD_c9(void)
{
	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	sCD.Status_CDC &= ~1;				// Stop CDC read

	sCD.Status_CDD = FAST_REV;
	sCD.cdd.status = sCD.Status_CDD | 2;

	sCD.cdd.Minute = INT_TO_BCDW(sCD.Cur_Track);
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD Rewind");
	sCD.CDD_Complete = 1;

	return 0;
}


int Close_Tray_CDD_cC(void)
{
	CD_Present = 0;
	//Clear_Sound_Buffer();

	sCD.Status_CDC &= ~1;			// Stop CDC read

	//if (PicoMCDcloseTray != NULL)
	//	CD_Present = PicoMCDcloseTray();

	sCD.Status_CDD = CD_Present ? STOPPED : NOCD;
	sCD.cdd.status = 0x0000;

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD Close Tray");
	sCD.CDD_Complete = 1;

	return 0;
}


int Open_Tray_CDD_cD(void)
{
	CHECK_TRAY_OPEN

	sCD.Status_CDC &= ~1;			// Stop CDC read

	Unload_ISO();
	CD_Present = 0;

	//if (PicoMCDopenTray != NULL)
	//	PicoMCDopenTray();

	sCD.Status_CDD = TRAY_OPEN;
	sCD.cdd.status = 0x0E00;

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	logMsg("issued CDD Open Tray");
	sCD.CDD_Complete = 1;

	return 0;
}


int CDD_cA(void)
{
	CHECK_TRAY_OPEN
	CHECK_CD_PRESENT

	sCD.Status_CDC &= ~1;

	sCD.Status_CDD = READY;
	sCD.cdd.status = sCD.Status_CDD;

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = INT_TO_BCDW(1);
	sCD.cdd.Frame = INT_TO_BCDW(1);
	sCD.cdd.Ext = 0;

	logMsg("issued CDD cA");
	sCD.CDD_Complete = 1;

	return 0;
}


int CDD_Def(void)
{
	sCD.cdd.status = sCD.Status_CDD;

	sCD.cdd.Minute = 0;
	sCD.cdd.Seconde = 0;
	sCD.cdd.Frame = 0;
	sCD.cdd.Ext = 0;

	return 0;
}


