#ifndef __MDFN_CDROMFILE_H
#define __MDFN_CDROMFILE_H

#include <stdio.h>

#include "CDUtility.h"

class CDAccess
{
 public:

 CDAccess();
 virtual ~CDAccess();

 virtual bool Read_Sector(uint8 *buf, int32 lba, uint32 size) = 0;

 virtual bool Read_Raw_Sector(uint8 *buf, int32 lba) = 0;

 virtual void Read_TOC(CDUtility::TOC *toc) = 0;

 virtual bool Is_Physical(void) = 0;

 virtual void Eject(bool eject_status) = 0;		// Eject a disc if it's physical, otherwise NOP.  Returns true on success(or NOP), false on error
};

CDAccess *cdaccess_open(const char *path);

#endif
