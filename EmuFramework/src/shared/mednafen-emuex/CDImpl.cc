/*  This file is part of EmuFramework.

	EmuFramework is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	EmuFramework is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <mednafen/mednafen.h>
#include <mednafen/cdrom/CDAccess.h>
#include <mednafen/cdrom/CDAccess_Image.h>
#include <mednafen/cdrom/CDAccess_CCD.h>
#include <mednafen/cdrom/CDAccess_CHD.h>

namespace Mednafen
{

using namespace CDUtility;

// Disk-image(rip) track/sector formats
enum
{
 DI_FORMAT_AUDIO       = 0x00,
 DI_FORMAT_MODE1       = 0x01,
 DI_FORMAT_MODE1_RAW   = 0x02,
 DI_FORMAT_MODE2       = 0x03,
 DI_FORMAT_MODE2_FORM1 = 0x04,
 DI_FORMAT_MODE2_FORM2 = 0x05,
 DI_FORMAT_MODE2_RAW   = 0x06,
 DI_FORMAT_CDI_RAW     = 0x07,
 _DI_FORMAT_COUNT
};

static const int32 DI_Size_Table[8] =
{
 2352, // Audio
 2048, // MODE1
 2352, // MODE1 RAW
 2336, // MODE2
 2048, // MODE2 Form 1
 2324, // Mode 2 Form 2
 2352, // MODE2 RAW
 2352, // CD-I RAW
};

void CDAccess_Image::ImageOpenBinary(VirtualFS* vfs, const std::string& path, bool isIso)
{
	NumTracks = FirstTrack = LastTrack = 1;
	total_sectors = 0;
	disc_type = DISC_TYPE_CDDA_OR_M1;
	auto &track = Tracks[1];
	track = {};
	track.fp = vfs->open(path, VirtualFS::MODE_READ);;
	track.FirstFileInstance = 1;
	track.DIFormat = DI_FORMAT_MODE1_RAW;
	if(isIso)
	{
		track.DIFormat = DI_FORMAT_MODE1;
		track.postgap = 150;
		total_sectors += track.postgap;
	}
	track.index[0] = -1;
	for(int32 i = 2; i < 100; i++)
		track.index[i] = -1;
	track.sectors = GetSectorCount(&track);
	total_sectors += track.sectors;
	track.subq_control |= SUBQ_CTRLF_DATA;
	GenerateTOC();
}

static int readSector(auto &cdAccess, uint8 *buf, int32 lba, uint32 size)
{
	uint8 data[2352 + 96]{};
	int format = cdAccess.Read_Raw_Sector(data, lba);
	switch(format)
	{
		case DI_FORMAT_AUDIO:
		assert(size == 2352);
		memcpy(buf, data, size);
		break;

		case DI_FORMAT_MODE1:
		case DI_FORMAT_MODE1_RAW:
		assert(size == 2048);
		memcpy(buf, data + 12 + 3 + 1, size);
		break;

		case DI_FORMAT_CDI_RAW:
		memcpy(buf, data, size);
		break;

		case DI_FORMAT_MODE2:
		case DI_FORMAT_MODE2_RAW:
		memcpy(buf, data + 16, size);
		break;

		case DI_FORMAT_MODE2_FORM1:
		case DI_FORMAT_MODE2_FORM2:
		memcpy(buf, data + 24, size);
		break;
	}
	return format;
}

int CDAccess_Image::Read_Sector(uint8 *buf, int32 lba, uint32 size)
{
	return readSector(*this, buf, lba, size);
}

void CDAccess_Image::HintReadSector(int32 lba, int32 count)
{
	for(int32 track = FirstTrack; track < (FirstTrack + NumTracks); track++)
	{
	 CDRFILE_TRACK_INFO *ct = &Tracks[track];

	 if(lba >= (ct->LBA - ct->pregap_dv - ct->pregap) && lba < (ct->LBA + ct->sectors + ct->postgap))
	 {
		// Handle pregap and postgap reading
		if(lba < (ct->LBA - ct->pregap_dv) || lba >= (ct->LBA + ct->sectors))
		{
		 // Null sector data, per spec
		}
		else
		{
			if(ct->AReader)
			{
				// ignore audio readers, they are read sequentially anyway
			}
			else
			{
				long SeekPos = ct->FileOffset;
				long LBARelPos = lba - ct->LBA;

				SeekPos += LBARelPos * DI_Size_Table[ct->DIFormat];

				if(ct->SubchannelMode)
				 SeekPos += 96 * (lba - ct->LBA);

				ct->fp->advise(SeekPos, 2352 * count, IG::IOAdvice::WillNeed);
			}
		}
	 }
	}
}

int CDAccess_CCD::Read_Sector(uint8 *buf, int32 lba, uint32 size)
{
	if(lba < 0 || (size_t)lba >= img_numsectors)
	 return -1;

	img_stream->readAtPos(buf, size, lba * 2352);
	return 0;
}

void CDAccess_CCD::HintReadSector(int32 lba, int32 count)
{
 img_stream->advise(lba * 2352, 2352 * count, IG::IOAdvice::WillNeed);
}

int CDAccess_CHD::Read_Sector(uint8 *buf, int32 lba, uint32 size)
{
	return readSector(*this, buf, lba, size);
}

}
