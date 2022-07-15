/***********************************************************
 *                                                         *
 * This source was taken from the Gens project             *
 * Written by Stéphane Dallongeville                       *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 * Modified/adapted for PicoDrive by notaz, 2007           *
 *                                                         *
 ***********************************************************/
#define LOGTAG "cdFile"
#include "scd.h"
#include "cd_file.h"
#include "cd_sys.h"
#include <imagine/logger/logger.h>
#include <imagine/util/ranges.hh>
#include <imagine/util/mayAliasInt.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <imagine/io/FileIO.hh>
#include <mednafen/mednafen.h>
#include <mednafen/cdrom/CDAccess.h>

#define cdprintf(x...)
//#define cdprintf(f,...) printf(f "\n",##__VA_ARGS__) // tmp
//#define DEBUG_CD

namespace Mednafen
{

bool MDFN_GetSettingB(const char *name) { return 0; }

}

static Mednafen::CDAccess *cdImage = nullptr;

int Load_ISO(Mednafen::CDAccess *cd)
{
	using namespace Mednafen;
	_scd_track *Tracks = sCD.TOC.Tracks;
	CDUtility::TOC toc;
	cd->Read_TOC(&toc);
	unsigned currLBA = 0;
	sCD.cddaLBA = 0;
	sCD.cddaDataLeftover = 0;
	for(auto i : IG::iotaCount(100))
	{
		if(i+1 > toc.last_track)
			break;
		auto lba = toc.tracks[i+1].lba;
		auto nextTrackLBA = toc.tracks[i+2].lba;
		if(!nextTrackLBA)
		{
			// leadout track
			nextTrackLBA = toc.tracks[100].lba;
		}
		auto length = nextTrackLBA - lba;
		CDUtility::LBA_to_AMSF(toc.tracks[i+1].lba, &sCD.TOC.Tracks[i].MSF.M, &sCD.TOC.Tracks[i].MSF.S, &sCD.TOC.Tracks[i].MSF.F);
		sCD.TOC.Tracks[i].ftype = toc.tracks[i+1].control ? TYPE_ISO : TYPE_MP3;
		sCD.TOC.Tracks[i].Length = length;
		logMsg("Track #%d: LBA %d MSF %d:%d:%d Length %d Type %d",
				i+1, lba, sCD.TOC.Tracks[i].MSF.M, sCD.TOC.Tracks[i].MSF.S, sCD.TOC.Tracks[i].MSF.F, length, sCD.TOC.Tracks[i].ftype);
		currLBA += length;
	}

	logMsg("last track %d", toc.last_track);
	sCD.TOC.Last_Track = toc.last_track;
	LBA_to_MSF(currLBA, &sCD.TOC.Tracks[toc.last_track].MSF);
	cdImage = cd;
	return 0;
}

void Unload_ISO(void)
{
	sCD.Status_CDD = 0;
	delete cdImage;
	cdImage = nullptr;
	for(auto &track: sCD.TOC.Tracks)
	{
		track = {};
	}
}

static void readLBA(void *dest, int lba)
{
	cdImage->Read_Sector((uint8*)dest, lba, 2048);
}

static void readCddaLBA(void *dest, int lba)
{
	cdImage->Read_Sector((uint8*)dest, lba, 2352);
}

int readCDDA(void *dest, unsigned size)
{
	if(!sCD.gate[0x36] && sCD.Status_CDD == 0x0100/*sCD.Cur_Track > 1 /*sCD.audioTrack && sCD.Status_CDD == 0x0100*/)
	{
		auto cddaBuffPos = (int32*)dest;
		auto sizeToWrite = size;
		//logMsg("%d frames in buffer", size);
		if(sCD.cddaDataLeftover)
		{
			//logMsg("reading %d frames of left-over CDDA", cddaDataLeftover);
			int32 cddaSector[588];
			cdImage->Read_Sector((uint8*)cddaSector, sCD.cddaLBA, 2352);
			unsigned copySize = std::min((unsigned)sCD.cddaDataLeftover, sizeToWrite);
			memcpy(cddaBuffPos, cddaSector + (588-sCD.cddaDataLeftover), copySize*4);
			sCD.cddaDataLeftover -= copySize;
			if(!sCD.cddaDataLeftover)
				sCD.cddaLBA++;
			cddaBuffPos += copySize;
			sizeToWrite -= copySize;
		}
		while(sizeToWrite >= 588)
		{
			//logMsg("reading 588 frames");
			cdImage->Read_Sector((uint8*)cddaBuffPos, sCD.cddaLBA, 2352);
			sCD.cddaLBA++;
			cddaBuffPos += 588;
			sizeToWrite -= 588;
		}
		if(sizeToWrite)
		{
			//logMsg("reading %d frames left", sizeToWrite);
			int32 cddaSector[588];
			cdImage->Read_Sector((uint8*)cddaSector, sCD.cddaLBA, 2352);
			memcpy(cddaBuffPos, cddaSector, sizeToWrite*4);
			sCD.cddaDataLeftover = 588 - sizeToWrite;
		}

		// apply fader
		{
			auto sample = (int16*)dest;
			for(auto i : IG::iotaCount(size*2))
			{
				sample[i] = (sample[i] * sCD.volume) / 1024;
			}
		}
		sCD.Cur_LBA = sCD.cddaLBA+12;
		return 1;
	}
	return 0;
}

int FILE_Read_One_LBA_CDC(void)
{
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
				//logMsg("read audio lba %d", sCD.Cur_LBA);
				readCddaLBA(sCD.cdc.Buffer + sCD.cdc.PT.N, sCD.Cur_LBA);
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
	sCD.cddaLBA = Track_to_LBA(sCD.Cur_Track);
	sCD.cddaDataLeftover = 0;

	logMsg("Play track #%i", sCD.Cur_Track);

	return 0;
}
