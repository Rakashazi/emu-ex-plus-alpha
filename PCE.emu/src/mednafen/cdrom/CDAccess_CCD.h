/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* CDAccess_CCD.h:
**  Copyright (C) 2013-2016 Mednafen Team
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

#include <imagine/io/FileIO.hh>
#include <mednafen/FileStream.h>
#include <mednafen/MemoryStream.h>

#include "CDAccess.h"

#include <vector>

class CDAccess_CCD : public CDAccess
{
 public:

 CDAccess_CCD(const std::string& path, bool image_memcache);
 ~CDAccess_CCD();

 bool Read_Raw_Sector(uint8 *buf, int32 lba) override;
 bool Read_Sector(uint8 *buf, int32 lba, uint32 size) override;

 virtual bool Fast_Read_Raw_PW_TSRE(uint8* pwbuf, int32 lba) const noexcept override;

 void Read_TOC(CDUtility::TOC *toc) override;

 void HintReadSector(uint32 lba, int32 count) override;

 private:

 void Load(const std::string& path, bool image_memcache);
 void Cleanup(void);

 void CheckSubQSanity(void);

 FileIO img_stream;
 std::unique_ptr<uint8[]> sub_data;

 size_t img_numsectors;
 CDUtility::TOC tocd;
};
