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
#ifdef CONFIG_PACKAGE_LIBVORBIS
	#include <vorbis/vorbisfile.h>
#else
	#include <tremor/ivorbisfile.h>
#endif
#include <io/api/vorbis.hh>
//#include <mpcdec/mpcdec.h>
#ifdef HAVE_LIBSNDFILE
#include <sndfile.h>
#include <io/api/sndfile.hh>
#endif

#include <string.h>
#include <errno.h>
#include <time.h>
#include <trio/trio.h>
#include <fcntl.h>

#include "../general.h"
#include "../endian.h"

AudioReader::AudioReader()
{

}

AudioReader::~AudioReader()
{

}

int64 AudioReader::Read(int16 *buffer, int64 frames)
{
 //abort();
 return(false);
}

bool AudioReader::Seek(int64 frame_offset)
{
 //abort();
 return(false);
}

int64 AudioReader::FrameCount(void)
{
 //abort();
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
	 fp->seekAbs(0);

  if(ov_open_callbacks(fp, &o->ovfile, NULL, 0, imagineVorbisIO))
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

 int64 Read(int16 *buffer, int64 frames)
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

 bool Seek(int64 frame_offset)
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

/*class MPCReader : public AudioReader
{
 public:
 MPCReader(FILE *fp)
 {
	fseek(fp, 0, SEEK_SET);
	lseek(fileno(fp), 0, SEEK_SET);

	memset(&MPCReaderFile, 0, sizeof(MPCReaderFile));
	memset(&MPCStreamInfo, 0, sizeof(MPCStreamInfo));
	memset(&MPCDecoder, 0, sizeof(MPCDecoder));
	memset(MPCBuffer, 0, sizeof(MPCBuffer));

	mpc_streaminfo_init(&MPCStreamInfo);
	mpc_reader_setup_file_reader(&MPCReaderFile, fp);

        if(mpc_streaminfo_read(&MPCStreamInfo, &MPCReaderFile.reader) != ERROR_CODE_OK)
        {
         throw(0);
        }

        mpc_decoder_setup(&MPCDecoder, &MPCReaderFile.reader);
        if(!mpc_decoder_initialize(&MPCDecoder, &MPCStreamInfo))
        {
         MDFN_printf(_("Error initializing MusePack decoder!\n"));
         throw(0);
        }
 }

 ~MPCReader()
 {
  // TODO
 }

 int64 Read(int16 *buffer, int64 frames)
 {
      //  MPC_SAMPLE_FORMAT MPCBuffer[MPC_DECODER_BUFFER_LENGTH];
      //MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];
      //  uint32 MPCBufferIn;
      int16 *cowbuf = (int16 *)buffer;
      int32 toread = frames * 2;

      while(toread > 0)
      {
       int32 tmplen;

       if(!MPCBufferIn)
       {
        int status = mpc_decoder_decode(&MPCDecoder, MPCBuffer, 0, 0);
	if(status < 0)
	 break;

        MPCBufferIn = status * 2;
	MPCBufferOffs = 0;
       }

       tmplen = MPCBufferIn;

       if(tmplen >= toread)
        tmplen = toread;

       for(int x = 0; x < tmplen; x++)
       {
	int32 samp = MPCBuffer[MPCBufferOffs + x] >> 14;

	//if(samp < - 32768 || samp > 32767) // This happens with some MPCs of ripped games I've tested, and it's not just 1 or 2 over, and I don't know why!
	// printf("MPC Sample out of range: %d\n", samp);
        *cowbuf = (int16)samp;
        cowbuf++;
       }
      
       MPCBufferOffs += tmplen;
       toread -= tmplen;
       MPCBufferIn -= tmplen;
      }

  return(frames - toread / 2);
 }

 bool Seek(int64 frame_offset)
 {
  mpc_decoder_seek_sample(&MPCDecoder, frame_offset);

  return(true);
 }

 int64 FrameCount(void)
 {
  return((MPCStreamInfo.frames - 1) * MPC_FRAME_LENGTH + MPCStreamInfo.last_frame_samples);
 }

 private:
 mpc_decoder MPCDecoder;
 mpc_streaminfo MPCStreamInfo;
 mpc_reader_file MPCReaderFile;
 MPC_SAMPLE_FORMAT MPCBuffer[MPC_DECODER_BUFFER_LENGTH];

 uint32 MPCBufferIn;
 uint32 MPCBufferOffs;
};*/

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
  fp->seekAbs(0);
  o->sf = sf_open_virtual(&imagineSndFileIO, SFM_READ, &o->sfinfo, fp);
  if(!o->sf)
  {
	  delete o;
	  return 0;
  }
  o->io = fp;
  o->init = 1;
  return o;
 }

 ~SFReader() 
 {
	 if(init)
	 {
		 sf_close(sf);
		 io->close();
	 }
 }

 int64 Read(int16 *buffer, int64 frames)
 {
  return(sf_read_short(sf, (short*)buffer, frames * 2) / 2);
 }

 bool Seek(int64 frame_offset)
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
 Io *io;
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

