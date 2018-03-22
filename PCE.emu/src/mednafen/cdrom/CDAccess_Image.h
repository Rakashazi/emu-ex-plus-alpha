#ifndef __MDFN_CDACCESS_IMAGE_H
#define __MDFN_CDACCESS_IMAGE_H

#include <map>

class FileStreamIOWrapper;
class CDAFReader;

struct CDRFILE_TRACK_INFO
{
  int32 LBA = 0;
	
	uint32 DIFormat = 0;
	uint8 subq_control = 0;

  int32 pregap = 0;
	int32 pregap_dv = 0;

	int32 postgap = 0;

	int32 index[100]{};

	int32 sectors = 0;	// Not including pregap sectors!
  std::shared_ptr<FileStreamIOWrapper> fp{};
	bool FirstFileInstance = 0;
	bool RawAudioMSBFirst = 0;
	long FileOffset = 0;
	unsigned int SubchannelMode = 0;

	uint32 LastSamplePos = 0;

	CDAFReader *AReader{};
};
#if 0
struct Medium_Chunk
{
	int64 Offset;		// Offset in [..TODO..]
	uint32 DIFormat;

        FILE *fp;
        bool FirstFileInstance;
        bool RawAudioMSBFirst;
        unsigned int SubchannelMode;

        uint32 LastSamplePos;
        AudioReader *AReader;
};

struct CD_Chunk
{
	int32 LBA;
	int32 Track;
	int32 Index;
	bool DataType;

	Medium_Chunk Medium;
};

static std::vector<CD_Chunk> Chunks;
#endif

class CDAccess_Image : public CDAccess
{
 public:

 CDAccess_Image(const std::string& path, bool image_memcache);
 ~CDAccess_Image() final;

 int Read_Raw_Sector(uint8 *buf, int32 lba) final;

 bool Fast_Read_Raw_PW_TSRE(uint8* pwbuf, int32 lba) const noexcept final;

 void Read_TOC(CDUtility::TOC *toc) final;

 void HintReadSector(uint32 lba, int32 count) final;

 int Read_Sector(uint8 *buf, int32 lba, uint32 size) final;

 private:

 int32 NumTracks = 0;
 int32 FirstTrack = 0;
 int32 LastTrack = 0;
 int32 total_sectors = 0;
 uint8 disc_type = 0;
 CDRFILE_TRACK_INFO Tracks[100]{}; // Track #0(HMM?) through 99
 CDUtility::TOC toc{};

 std::map<uint32, std::array<uint8, 12>> SubQReplaceMap;

 std::string base_dir;

 void ImageOpen(const std::string& path, bool image_memcache);
 void ImageOpenBinary(const std::string& path, bool isIso);
 void LoadSBI(const std::string& sbi_path);
 void GenerateTOC(void);
 void Cleanup(void);

 // MakeSubPQ will OR the simulated P and Q subchannel data into SubPWBuf.
 int32 MakeSubPQ(int32 lba, uint8 *SubPWBuf) const;

 void ParseTOCFileLineInfo(CDRFILE_TRACK_INFO *track, const int tracknum, const std::string &filename, const char *binoffset, const char *msfoffset, const char *length, bool image_memcache, std::map<std::string, std::shared_ptr<FileStreamIOWrapper>> &toc_streamcache);
 uint32 GetSectorCount(CDRFILE_TRACK_INFO *track);
};


#endif
