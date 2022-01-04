#pragma once

#define TYPE_ISO 1
#define TYPE_BIN 2
#define TYPE_MP3 3
//#define TYPE_WAV 4

namespace Mednafen
{
class CDAccess;
}

int Load_ISO(Mednafen::CDAccess *cd);
//int  Load_ISO(const char *iso_name, int is_bin);
void Unload_ISO(void);
int  FILE_Read_One_LBA_CDC(void);
int  FILE_Play_CD_LBA(void);
