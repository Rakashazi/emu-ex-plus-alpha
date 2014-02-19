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

#include "../mednafen.h"
#include "audioreader.h"

#include <sys/types.h>
#include <sys/stat.h>

#define OV_EXCLUDE_STATIC_CALLBACKS

#ifdef ARCH_X86
#define CONFIG_PACKAGE_LIBVORBIS
#endif

#ifdef CONFIG_PACKAGE_LIBVORBIS
#include <vorbis/vorbisfile.h>
#else
#include <tremor/ivorbisfile.h>
#endif

#include <io/api/vorbis.hh>
#ifdef HAVE_LIBSNDFILE
#include <sndfile.h>
#include <io/api/sndfile.hh>
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

class OggVorbisReader : public AudioReader
{
 public:
 static OggVorbisReader *open(Io *fp)
 {
	 OggVorbisReader *o = new OggVorbisReader();
	 if(!o)
		 return 0;
	 o->init = 0;
	 fp->seekA(0);

  if(ov_open_callbacks(fp, &o->ovfile, NULL, 0, imagineVorbisIONoClose))
  {
	  delete o;
	return 0;
  }
  o->init = 1;
  return o;
 }

 ~OggVorbisReader()
 {
	 if(init)
		 ov_clear(&ovfile);
 }

 int64 Read_(int16 *buffer, int64 frames)
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

 bool Seek_(int64 frame_offset)
 {
  ov_pcm_seek(&ovfile, frame_offset);
  return(true);
 }

 int64 FrameCount(void)
 {
  return(ov_pcm_total(&ovfile, -1));
 }

 private:
 OggVorbis_File ovfile;
 bool init;
};

#ifdef HAVE_LIBSNDFILE



class SFReader : public AudioReader
{
 public:

 static SFReader *open(Io *fp)
 {
	 SFReader *o = new SFReader();
	 if(!o)
		 return 0;
	 o->init = 0;
  memset(&o->sfinfo, 0, sizeof(SF_INFO));
  fp->seekA(0);
  o->sf = sf_open_virtual(&imagineSndFileIO, SFM_READ, &o->sfinfo, fp);
  if(!o->sf)
  {
	  delete o;
	  return 0;
  }
  o->init = 1;
  return o;
 }

 ~SFReader() 
 {
	 if(init)
	 {
		 sf_close(sf);
	 }
 }

 int64 Read_(int16 *buffer, int64 frames)
 {
  return(sf_read_short(sf, (short*)buffer, frames * 2) / 2);
 }

 bool Seek_(int64 frame_offset)
 {
  // FIXME error condition
  if(sf_seek(sf, frame_offset, SEEK_SET) != frame_offset)
   return(false);
  return(true);
 }

 int64 FrameCount(void)
 {
  return(sfinfo.frames);
 }

 private:
 SNDFILE *sf;
 SF_INFO sfinfo;
 bool init;
};
#endif


AudioReader *AR_Open(Io *fp)
{
 AudioReader *AReader = NULL;

 /*if(!AReader)
 {
  try
  {
   AReader = new MPCReader(fp);
  }
  catch(int i)
  {
  }
 }*/

 if(!AReader)
 {
   AReader = OggVorbisReader::open(fp);
 }


#ifdef HAVE_LIBSNDFILE
 if(!AReader)
 {
   AReader = SFReader::open(fp);
 }
#endif

 return(AReader);
}

