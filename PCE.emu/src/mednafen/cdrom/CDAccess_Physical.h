#ifndef __MDFN_CDACCESS_PHYSICAL_H
#define __MDFN_CDACCESS_PHYSICAL_H

// Don't include <cdio.h> here, else it will pollute with its #define's.

class CDAccess_Physical : public CDAccess
{
 public:

 CDAccess_Physical(const char *path);
 ~CDAccess_Physical() override;

 bool Read_Sector(uint8 *buf, int32 lba, uint32 size) override;

 bool Read_Raw_Sector(uint8 *buf, int32 lba) override;

 void Read_TOC(CDUtility::TOC *toc) override;

 bool Is_Physical(void) override;

 void Eject(bool eject_status) override;
 private:

 void *p_cdio;

 void DetermineFeatures(void);
 void PhysOpen(const char *path);
 void ReadPhysDiscInfo(unsigned retry);

 void PreventAllowMediumRemoval(bool prevent);

 CDUtility::TOC PhysTOC;

 // TODO: 1-bit per sector on the physical CD.  If set, don't read that sector.
 uint8 SkipSectorRead[65536];
};

#endif
