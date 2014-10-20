/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// AR_Open(), and AudioReader, will NOT take "ownership" of the Stream object(IE it won't ever delete it).  Though it does assume it has exclusive access
// to it for as long as the AudioReader object exists.

// Don't allow exceptions to propagate into the vorbis/musepack/etc. libraries, as it could easily leave the state of the library's decoder "object" in an
// inconsistent state, which would cause all sorts of unfun when we try to destroy it while handling the exception farther up.

#include "../mednafen.h"
#include "audioreader.h"

#include <sys/types.h>
#include <sys/stat.h>

#define OV_EXCLUDE_STATIC_CALLBACKS

#if defined ARCH_X86 || defined __aarch64__
#define CONFIG_PACKAGE_LIBVORBIS
#endif

#ifdef CONFIG_PACKAGE_LIBVORBIS
#include <vorbis/vorbisfile.h>
#else
#include <tremor/ivorbisfile.h>
#endif

#include <imagine/io/api/vorbis.hh>
#ifdef HAVE_LIBSNDFILE
#include <sndfile.h>
#include <imagine/io/api/sndfile.hh>
#endif

#include <string.h>
#include <errno.h>
#include <time.h>

#include "../general.h"
#include "../endian.h"

AudioReader::AudioReader() : LastReadPos(0)
{

}

AudioReader::~AudioReader()
{

}

int64 AudioReader::Read_(int16 *buffer, int64 frames)
{
 abort();
 return(false);
}

bool AudioReader::Seek_(int64 frame_offset)
{
 abort();
 return(false);
}

int64 AudioReader::FrameCount(void)
{
 abort();
 return(0);
}

/*
**
**
**
**
**
**
**
**
**
*/

class OggVorbisReader : public AudioReader
{
 public:
 OggVorbisReader(IO &fp);
 ~OggVorbisReader();

 int64 Read_(int16 *buffer, int64 frames);
 bool Seek_(int64 frame_offset);
 int64 FrameCount(void);

 private:
 OggVorbis_File ovfile;
};

OggVorbisReader::OggVorbisReader(IO &fp)
{
 fp.seekS(0);

 if(ov_open_callbacks(&fp, &ovfile, NULL, 0, IOAPI::vorbisNoClose))
	 throw(0);
}

OggVorbisReader::~OggVorbisReader()
{
	ov_clear(&ovfile);
}

int64 OggVorbisReader::Read_(int16 *buffer, int64 frames)
{
 uint8 *tw_buf = (uint8 *)buffer;
 int cursection = 0;
 long toread = frames * sizeof(int16) * 2;

 while(toread > 0)
 {
	#ifdef MSB_FIRST
	  int endianPack = 1;
	#else
	  int endianPack = 0;
	#endif
  long didread =
	#ifdef CONFIG_PACKAGE_LIBVORBIS
	 ov_read(&ovfile, (char*)tw_buf, toread, endianPack, 2, 1, &cursection);
	#else
	 ov_read(&ovfile, (char*)tw_buf, toread, &cursection);
	#endif

  if(didread == 0)
   break;

  tw_buf = (uint8 *)tw_buf + didread;
  toread -= didread;
 }

 return(frames - toread / sizeof(int16) / 2);
}

bool OggVorbisReader::Seek_(int64 frame_offset)
{
 ov_pcm_seek(&ovfile, frame_offset);
 return(true);
}

int64 OggVorbisReader::FrameCount(void)
{
 return(ov_pcm_total(&ovfile, -1));
}

/*
**
**
**
**
**
**
**
**
**
*/

#ifdef HAVE_LIBSNDFILE
class SFReader : public AudioReader
{
 public:

 SFReader(IO &fp);
 ~SFReader();

 int64 Read_(int16 *buffer, int64 frames);
 bool Seek_(int64 frame_offset);
 int64 FrameCount(void);

 private:
 SNDFILE *sf;
 SF_INFO sfinfo;
};

SFReader::SFReader(IO &fp)
{
 fp.seekS(0);

 memset(&sfinfo, 0, sizeof(sfinfo));
 if(!(sf = sf_open_virtual(&IOAPI::sndfile, SFM_READ, &sfinfo, &fp)))
	throw(0);
}

SFReader::~SFReader()
{
 sf_close(sf);
}

int64 SFReader::Read_(int16 *buffer, int64 frames)
{
 return(sf_read_short(sf, (short*)buffer, frames * 2) / 2);
}

bool SFReader::Seek_(int64 frame_offset)
{
 // FIXME error condition
 if(sf_seek(sf, frame_offset, SEEK_SET) != frame_offset)
  return(false);
 return(true);
}

int64 SFReader::FrameCount(void)
{
 return(sfinfo.frames);
}

#endif


AudioReader *AR_Open(IO &fp)
{
 try
 {
  return new OggVorbisReader(fp);
 }
 catch(int i)
 {
 }


#ifdef HAVE_LIBSNDFILE
 try
 {
	return new SFReader(fp);
 }
 catch(int i)
 {
 }
#endif

 return(NULL);
}

