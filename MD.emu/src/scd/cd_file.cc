/***********************************************************
 *                                                         *
 * This source was taken from the Gens project             *
 * Written by Stéphane Dallongeville                       *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 * Modified/adapted for PicoDrive by notaz, 2007           *
 *                                                         *
 ***********************************************************/
#define thisModuleName "cdFile"
#include "scd.h"
#include "cd_file.h"
#include "cd_sys.h"
#include <logger/interface.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <io/sys.hh>

#define cdprintf(x...)
//#define cdprintf(f,...) printf(f "\n",##__VA_ARGS__) // tmp
//#define DEBUG_CD

int Load_ISO(const char *iso_name, int is_bin)
{
	int i, j, num_track, Cur_LBA, index, ret, iso_name_len;
	_scd_track *Tracks = sCD.TOC.Tracks;
	char tmp_name[1024], tmp_ext[10];
	Io *pmf;
	static const char *exts[] = {
		"%02d.mp3", " %02d.mp3", "-%02d.mp3", "_%02d.mp3", " - %02d.mp3",
		"%d.mp3", " %d.mp3", "-%d.mp3", "_%d.mp3", " - %d.mp3",
#if CASE_SENSITIVE_FS
		"%02d.MP3", " %02d.MP3", "-%02d.MP3", "_%02d.MP3", " - %02d.MP3",
#endif
	};

	//if (PicoCDLoadProgressCB != 0) PicoCDLoadProgressCB(1);

	Unload_ISO();

	Tracks[0].ftype = is_bin ? TYPE_BIN : TYPE_ISO;

	Tracks[0].F = pmf = IoSys::open(iso_name);
	if (Tracks[0].F == 0)
	{
		Tracks[0].ftype = 0;
		Tracks[0].Length = 0;
		return -1;
	}

	if (Tracks[0].ftype == TYPE_ISO)
		Tracks[0].Length = pmf->size() >> 11;	// size in sectors
	else	Tracks[0].Length = pmf->size() / 2352;

	Tracks[0].MSF.M = 0; // minutes
	Tracks[0].MSF.S = 2; // seconds
	Tracks[0].MSF.F = 0; // frames

	logMsg("Track 0 - %02d:%02d:%02d DATA, %d sectors", Tracks[0].MSF.M, Tracks[0].MSF.S, Tracks[0].MSF.F, Tracks[0].Length);

	Cur_LBA = Tracks[0].Length;				// Size in sectors

	iso_name_len = strlen(iso_name);
	if (iso_name_len >= sizeof(tmp_name))
		iso_name_len = sizeof(tmp_name) - 1;

	/*for (num_track = 2, i = 0; i < 100; i++)
	{
		//if (PicoCDLoadProgressCB != NULL && i > 1) PicoCDLoadProgressCB(i);

		for (j = 0; j < sizeof(exts)/sizeof(char *); j++)
		{
			int ext_len;
			Io *tmp_file;
			sprintf(tmp_ext, exts[j], i);
			ext_len = strlen(tmp_ext);

			memcpy(tmp_name, iso_name, iso_name_len + 1);
			tmp_name[iso_name_len - 4] = 0;
			strcat(tmp_name, tmp_ext);

			tmp_file = IoSys::open(tmp_name);//fopen(tmp_name, "rb");
			if (!tmp_file && i > 1 && iso_name_len > ext_len) {
				tmp_name[iso_name_len - ext_len] = 0;
				strcat(tmp_name, tmp_ext);
				tmp_file = IoSys::open(tmp_name);//fopen(tmp_name, "rb");
			}

			if (tmp_file)
			{
				int fs;
				index = num_track - 1;

				ret = fseek(tmp_file, 0, SEEK_END);
				fs = ftell(tmp_file);				// used to calculate lenght
				fseek(tmp_file, 0, SEEK_SET);

#if DONT_OPEN_MANY_FILES
				// some systems (like PSP) can't have many open files at a time,
				// so we work with their names instead.
				fclose(tmp_file);
				tmp_file = (void *) strdup(tmp_name);
#endif
				// TODO mp3
				//Tracks[index].KBtps = (short) mp3_get_bitrate(tmp_file, fs);
				Tracks[index].KBtps = 192;
				Tracks[index].KBtps >>= 3;
				if (ret != 0 || Tracks[index].KBtps <= 0)
				{
					logMsg("Error track %i: rate %i", index, Tracks[index].KBtps);
#if !DONT_OPEN_MANY_FILES
					fclose(tmp_file);
#else
					free(tmp_file);
#endif
					continue;
				}

				Tracks[index].F = tmp_file;

				LBA_to_MSF(Cur_LBA, &Tracks[index].MSF);

				// MP3 File
				Tracks[index].ftype = TYPE_MP3;
				fs *= 75;
				fs /= Tracks[index].KBtps * 1000;
				Tracks[index].Length = fs;
				Cur_LBA += Tracks[index].Length;

				logMsg("Track %i: %s - %02d:%02d:%02d len=%i AUDIO", index, tmp_name, Tracks[index].MSF.M,
					Tracks[index].MSF.S, Tracks[index].MSF.F, fs);

				num_track++;
				break;
			}
		}
	}*/

	// Fake some CD tracks
	num_track = 2;
	iterateTimes(51, i)
	{
		int index = num_track - 1;
		LBA_to_MSF(Cur_LBA, &Tracks[index].MSF);
		Tracks[index].Length = 4500*4;
		Cur_LBA += Tracks[index].Length;
		num_track++;
	}

	sCD.TOC.Last_Track = num_track - 1;

	index = num_track - 1;

	LBA_to_MSF(Cur_LBA, &Tracks[index].MSF);

	logMsg("End CD - %02d:%02d:%02d\n\n", Tracks[index].MSF.M,
		Tracks[index].MSF.S, Tracks[index].MSF.F);

	//if (PicoCDLoadProgressCB != NULL) PicoCDLoadProgressCB(100);
	return 0;
}


void Unload_ISO(void)
{
	int i;

	if (sCD.TOC.Tracks[0].F) fclose(sCD.TOC.Tracks[0].F);

	for(i = 1; i < 100; i++)
	{
		if (sCD.TOC.Tracks[i].F != NULL)
#if !DONT_OPEN_MANY_FILES
			fclose(sCD.TOC.Tracks[i].F);
#else
			free(sCD.TOC.Tracks[i].F);
#endif
	}
	memset(sCD.TOC.Tracks, 0, sizeof(sCD.TOC.Tracks));
}

static void readLBA(void *dest, int lba)
{
	bool isBin = sCD.TOC.Tracks[0].ftype == TYPE_BIN;

	int pos = isBin ? (lba * 2352 + 16) : (lba << 11);
	//logMsg("seeking to lba %d, %d", lba, pos);
	fseek(sCD.TOC.Tracks[0].F, pos, SEEK_SET);
	fread(dest, 2048, 1, sCD.TOC.Tracks[0].F);
}

int FILE_Read_One_LBA_CDC(void)
{
//	static char cp_buf[2560];

	if (sCD.gate[0x36] & 1)					// DATA
	{
		if (sCD.TOC.Tracks[0].F == NULL)
		{
			logMsg("no file for data track");
			return -1;
		}

		// moved below..
		//fseek(sCD.TOC.Tracks[0].F, where_read, SEEK_SET);
		//fread(cp_buf, 1, 2048, sCD.TOC.Tracks[0].F);

		//logMsg("Read file CDC 1 data sector :");
	}
	else									// AUDIO
	{
		// int rate, channel;

		// if (sCD.TOC.Tracks[sCD.Cur_Track - 1].ftype == TYPE_MP3)
		{
			// TODO
			// MP3_Update(cp_buf, &rate, &channel, 0);
			// Write_CD_Audio((short *) cp_buf, rate, channel, 588);
		}

		//logMsg("Read file CDC 1 audio sector :");
	}

	// Update CDC stuff

	CDC_Update_Header();

	if (sCD.gate[0x36] & 1)		// DATA track
	{
		if (sCD.cdc.CTRL.B.B0 & 0x80)		// DECEN = decoding enable
		{
			if (sCD.cdc.CTRL.B.B0 & 0x04)	// WRRQ : this bit enable write to buffer
			{
				int where_read = 0;

				// CAUTION : lookahead bit not implemented

				if (sCD.Cur_LBA < 0)
					where_read = 0;
				else if (sCD.Cur_LBA >= sCD.TOC.Tracks[0].Length)
					where_read = sCD.TOC.Tracks[0].Length - 1;
				else where_read = sCD.Cur_LBA;
				logMsg("read lba %d to offset 0x%X", sCD.Cur_LBA, (sCD.cdc.PT.N + 2352) & 0x7FFF);
				sCD.Cur_LBA++;

				sCD.cdc.WA.N = (sCD.cdc.WA.N + 2352) & 0x7FFF;		// add one sector to WA
				sCD.cdc.PT.N = (sCD.cdc.PT.N + 2352) & 0x7FFF;

				*(uint32a*)(sCD.cdc.Buffer + sCD.cdc.PT.N) = sCD.cdc.HEAD.N;
				readLBA(sCD.cdc.Buffer + sCD.cdc.PT.N + 4, where_read);

#ifdef DEBUG_CD
				logMsg("Read -> WA = %d  Buffer[%d] =", sCD.cdc.WA.N, sCD.cdc.PT.N & 0x3FFF);
				logMsg("Header 1 = %.2X %.2X %.2X %.2X", sCD.cdc.HEAD.B.B0,
					sCD.cdc.HEAD.B.B1, sCD.cdc.HEAD.B.B2, sCD.cdc.HEAD.B.B3);
				logMsg("Header 2 = %.2X %.2X %.2X %.2X --- %.2X %.2X",
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 0) & 0x3FFF],
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 1) & 0x3FFF],
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 2) & 0x3FFF],
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 3) & 0x3FFF],
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 4) & 0x3FFF],
					sCD.cdc.Buffer[(sCD.cdc.PT.N + 5) & 0x3FFF]);
#endif
			}

		}
	}
	else		// music track
	{
		sCD.Cur_LBA++;

		sCD.cdc.WA.N = (sCD.cdc.WA.N + 2352) & 0x7FFF;		// add one sector to WA
		sCD.cdc.PT.N = (sCD.cdc.PT.N + 2352) & 0x7FFF;

		if (sCD.cdc.CTRL.B.B0 & 0x80)		// DECEN = decoding enable
		{
			if (sCD.cdc.CTRL.B.B0 & 0x04)	// WRRQ : this bit enable write to buffer
			{
				// CAUTION : lookahead bit not implemented

				//memcpy(&sCD.cdc.Buffer[sCD.cdc.PT.N], cp_buf, 2352);
			}
		}
	}

	if (sCD.cdc.CTRL.B.B0 & 0x80)		// DECEN = decoding enable
	{
		sCD.cdc.STAT.B.B0 = 0x80;

		if (sCD.cdc.CTRL.B.B0 & 0x10)	// determine form bit form sub header ?
		{
			sCD.cdc.STAT.B.B2 = sCD.cdc.CTRL.B.B1 & 0x08;
		}
		else
		{
			sCD.cdc.STAT.B.B2 = sCD.cdc.CTRL.B.B1 & 0x0C;
		}

		if (sCD.cdc.CTRL.B.B0 & 0x02) sCD.cdc.STAT.B.B3 = 0x20;	// ECC done
		else sCD.cdc.STAT.B.B3 = 0x00;	// ECC not done

		if (sCD.cdc.IFCTRL & 0x20)
		{
			if (sCD.gate[0x33] & (1<<5))
			{
				//logMsg("sCD.cdc dec irq 5");
				scd_interruptSubCpu(5);
			}

			sCD.cdc.IFSTAT &= ~0x20;		// DEC interrupt happen
			sCD.cdc.Decode_Reg_Read = 0;	// Reset read after DEC int
		}
	}


	return 0;
}


int FILE_Play_CD_LBA(void)
{
	int index = sCD.Cur_Track - 1;
	sCD.audioTrack = index;

	logMsg("Play track #%i", sCD.Cur_Track);

	if (sCD.TOC.Tracks[index].F == NULL)
	{
		return 1;
	}

	if (sCD.TOC.Tracks[index].ftype == TYPE_MP3)
	{
		int pos1024 = 0;
		int Track_LBA_Pos = sCD.Cur_LBA - Track_to_LBA(sCD.Cur_Track);
		if (Track_LBA_Pos < 0) Track_LBA_Pos = 0;
		if (Track_LBA_Pos)
			pos1024 = Track_LBA_Pos * 1024 / sCD.TOC.Tracks[index].Length;

		//TODO mp3
		//mp3_start_play(sCD.TOC.Tracks[index].F, pos1024);
	}
	else
	{
		return 3;
	}

	return 0;
}

#undef thisModuleName
