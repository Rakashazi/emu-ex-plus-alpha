/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* CDAccess_CHD.h:
**  Copyright (C) 2017 Romain Tisserand
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <mednafen/FileStream.h>
#include <mednafen/MemoryStream.h>

#include "CDAccess.h"
#include <libchdr/chd.h>

namespace Mednafen
{

struct CHDFILE_TRACK_INFO
{
   int32_t LBA;

   uint32_t DIFormat;
   uint8_t subq_control;

   int32_t pregap;
   int32_t pregap_dv;

   int32_t postgap;

   int32_t index[100];

   int32_t sectors; // Not including pregap sectors!
   bool FirstFileInstance;
   bool RawAudioMSBFirst;
   long FileOffset;
   unsigned int SubchannelMode;

   uint32_t LastSamplePos;

   uint32_t fileOffset;
};

class CDAccess_CHD final : public CDAccess
{
 public:

 CDAccess_CHD(VirtualFS* vfs, const std::string& path, bool image_memcache);
 ~CDAccess_CHD() final;

 int Read_Raw_Sector(uint8 *buf, int32 lba) final;

 bool Fast_Read_Raw_PW_TSRE(uint8* pwbuf, int32 lba) const noexcept final;

 void Read_TOC(CDUtility::TOC *toc) final;

 void HintReadSector(int32 lba, int32 count) final {};

 int Read_Sector(uint8 *buf, int32 lba, uint32 size) final;

 private:

 void Load(VirtualFS* vfs, const std::string& path, bool image_memcache);
 void Cleanup(void);

  // MakeSubPQ will OR the simulated P and Q subchannel data into SubPWBuf.
  int32_t MakeSubPQ(int32_t lba, uint8_t *SubPWBuf) const;

  bool Read_CHD_Hunk_RAW(uint8_t *buf, int32_t lba, CHDFILE_TRACK_INFO* track);
  bool Read_CHD_Hunk_M1(uint8_t *buf, int32_t lba, CHDFILE_TRACK_INFO* track);
  bool Read_CHD_Hunk_M2(uint8_t *buf, int32_t lba, CHDFILE_TRACK_INFO* track);

  int32_t NumTracks;
  int32_t FirstTrack;
  int32_t LastTrack;
  int32_t total_sectors;
  uint8_t disc_type;
  CDUtility::TOC toc;
  CHDFILE_TRACK_INFO Tracks[100]; // Track #0(HMM?) through 99

  //struct disc;
  //struct session sessions[DISC_MAX_SESSIONS];
  int num_sessions;
  //struct track tracks[DISC_MAX_TRACKS];
  int num_tracks;

  chd_file *chd;
  /* hunk data cache */
  uint8_t *hunkmem;
  /* last hunknum read */
  int oldhunk;
};

}
