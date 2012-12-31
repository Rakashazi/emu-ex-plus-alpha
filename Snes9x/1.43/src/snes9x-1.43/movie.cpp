/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

  Input recording/playback code
  (c) Copyright 2004 blip
 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#include <string.h>
#include <unistd.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>
#include <stdlib.h>

#if defined(__unix) || defined(__linux) || defined(__sun) || defined(__DJGPP)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <time.h>

#ifdef __WIN32__
#include <io.h>
#ifndef W_OK
#define W_OK 2
#endif
#endif

#include "movie.h"
#include "snes9x.h"
#include "cpuexec.h"
#include "snapshot.h"

#define SMV_MAGIC	0x1a564d53		// SMV0x1a
#define SMV_VERSION	1
#define SMV_HEADER_SIZE	32
#define CONTROLLER_DATA_SIZE	2
#define BUFFER_GROWTH_SIZE	4096

enum MovieState
{
	MOVIE_STATE_NONE=0,
	MOVIE_STATE_PLAY,
	MOVIE_STATE_RECORD
};

static struct SMovie
{
	enum MovieState State;
	char   Filename [_MAX_PATH];
	FILE*  File;
	uint32 SaveStateOffset;
	uint32 ControllerDataOffset;
	uint32 MovieId;
	uint32 CurrentFrame;
	uint32 MaxFrame;
	uint32 RerecordCount;
	uint8  ControllersMask;
	uint8  Opts;
	bool8  ReadOnly;
	uint32 BytesPerFrame;
	uint8* InputBuffer;
	uint32 InputBufferSize;
	uint8* InputBufferPtr;
	bool8  FrameDisplay;
	char   FrameDisplayString[256];
} Movie;

/*
	For illustration:
struct MovieFileHeader
{
	uint32	magic;		// SMV0x1a
	uint32	version;
	uint32	uid;			// used to match savestates to a particular movie
	uint32	rerecord_count;
	uint32	length_frames;
	uint8	flags[4];
	uint32	offset_to_savestate;	// smvs have an embedded savestate
	uint32	offset_to_controller_data;
	// after the header comes extra metadata
	// sizeof(metadata) = offset_to_savestate - sizeof(MovieFileHeader)
};
*/

static int bytes_per_frame()
{
	int i;
	int num_controllers;

	num_controllers=0;
	for(i=0; i<5; ++i)
	{
		if(Movie.ControllersMask & (1<<i))
		{
			++num_controllers;
		}
	}

	return CONTROLLER_DATA_SIZE*num_controllers;
}

static inline uint32 Read32(const uint8*& ptr)
{
	uint32 v=(ptr[0] | (ptr[1]<<8) | (ptr[2]<<16) | (ptr[3]<<24));
	ptr += 4;
	return v;
}

static inline uint16 Read16(const uint8*& ptr) /* const version */
{
	uint16 v=(ptr[0] | (ptr[1]<<8));
	ptr += 2;
	return v;
}

static inline uint16 Read16(uint8*& ptr) /* non-const version */
{
	uint16 v=(ptr[0] | (ptr[1]<<8));
	ptr += 2;
	return v;
}

static void Write32(uint32 v, uint8*& ptr)
{
	ptr[0]=(uint8)(v&0xff);
	ptr[1]=(uint8)((v>>8)&0xff);
	ptr[2]=(uint8)((v>>16)&0xff);
	ptr[3]=(uint8)((v>>24)&0xff);
	ptr += 4;
}

static void Write16(uint16 v, uint8*& ptr)
{
	ptr[0]=(uint8)(v&0xff);
	ptr[1]=(uint8)((v>>8)&0xff);
	ptr += 2;
}

static int read_movie_header(FILE* fd, SMovie* movie)
{
	uint8 header[SMV_HEADER_SIZE];
	if(fread(header, 1, SMV_HEADER_SIZE, fd) != SMV_HEADER_SIZE)
		return WRONG_FORMAT;

	const uint8* ptr=header;
	uint32 magic=Read32(ptr);
	if(magic!=SMV_MAGIC)
		return WRONG_FORMAT;

	uint32 version=Read32(ptr);
	if(version!=SMV_VERSION)
		return WRONG_VERSION;

	movie->MovieId=Read32(ptr);
	movie->RerecordCount=Read32(ptr);
	movie->MaxFrame=Read32(ptr);

	movie->ControllersMask=*ptr++;
	movie->Opts=*ptr++;
	ptr += 2;

	movie->SaveStateOffset=Read32(ptr);
	movie->ControllerDataOffset=Read32(ptr);

	return SUCCESS;
}

static void write_movie_header(FILE* fd, const SMovie* movie)
{
	uint8 header[SMV_HEADER_SIZE];
	uint8* ptr=header;

	Write32(SMV_MAGIC, ptr);
	Write32(SMV_VERSION, ptr);
	Write32(movie->MovieId, ptr);
	Write32(movie->RerecordCount, ptr);
	Write32(movie->MaxFrame, ptr);

	*ptr++=movie->ControllersMask;
	*ptr++=movie->Opts;
	*ptr++=0;
	*ptr++=0;

	Write32(movie->SaveStateOffset, ptr);
	Write32(movie->ControllerDataOffset, ptr);

	fwrite(header, 1, SMV_HEADER_SIZE, fd);
}

static void flush_movie()
{
	fseek(Movie.File, 0, SEEK_SET);
	write_movie_header(Movie.File, &Movie);
	fseek(Movie.File, Movie.ControllerDataOffset, SEEK_SET);
	fwrite(Movie.InputBuffer, 1, Movie.BytesPerFrame*(Movie.MaxFrame+1), Movie.File);
}

static void change_state(MovieState new_state)
{
	if(new_state==Movie.State)
		return;

	if(Movie.State==MOVIE_STATE_RECORD)
	{
		flush_movie();
	}

	Movie.State=new_state;

	if(new_state==MOVIE_STATE_NONE)
	{
		fclose(Movie.File);
		Movie.File=NULL;
		// FIXME: truncate movie to MaxFrame length
		/* truncate() could be used, if it's certain
		 * that the savestate block is never after
		 * the controller data block. It is not guaranteed
		 * by the format.
		 */
	}
}

static void reserve_buffer_space(uint32 space_needed)
{
	if(space_needed > Movie.InputBufferSize)
	{
		uint32 ptr_offset = Movie.InputBufferPtr - Movie.InputBuffer;
		uint32 alloc_chunks = space_needed / BUFFER_GROWTH_SIZE;
		Movie.InputBufferSize = BUFFER_GROWTH_SIZE * (alloc_chunks+1);
		Movie.InputBuffer = (uint8*)realloc(Movie.InputBuffer, Movie.InputBufferSize);
		Movie.InputBufferPtr = Movie.InputBuffer + ptr_offset;
	}
}

static void read_frame_controller_data()
{
	int i;
	for(i=0; i<5; ++i)
	{
		if(Movie.ControllersMask & (1<<i))
		{
			IPPU.Joypads[i]=(uint32)(Read16(Movie.InputBufferPtr)) | 0x80000000L;
		}
		else
		{
			IPPU.Joypads[i]=0;		// pretend the controller is disconnected
		}
	}
}

static void write_frame_controller_data()
{
	reserve_buffer_space((uint32)((Movie.InputBufferPtr+Movie.BytesPerFrame)-Movie.InputBuffer));

	int i;
	for(i=0; i<5; ++i)
	{
		if(Movie.ControllersMask & (1<<i))
		{
			Write16((uint16)(IPPU.Joypads[i] & 0xffff), Movie.InputBufferPtr);
		}
		else
		{
			IPPU.Joypads[i]=0;		// pretend the controller is disconnected
		}
	}
}

void S9xMovieInit ()
{
	memset(&Movie, 0, sizeof(Movie));
	Movie.State = MOVIE_STATE_NONE;
}

int S9xMovieOpen (const char* filename, bool8 read_only)
{
	FILE* fd;
	STREAM stream;
	int result;
	int fn;

	if(!(fd=fopen(filename, read_only ? "rb" : "rb+")))
		return FILE_NOT_FOUND;

	// stop current movie before opening
	change_state(MOVIE_STATE_NONE);

	// read header
	if((result=read_movie_header(fd, &Movie))!=SUCCESS)
	{
		fclose(fd);
		return result;
	}

	fn=dup(fileno(fd));
	fclose(fd);

	// apparently this lseek is necessary
	lseek(fn, Movie.SaveStateOffset, SEEK_SET);
	if(!(stream=REOPEN_STREAM(fn, "rb")))
		return FILE_NOT_FOUND;

	if(Movie.Opts & MOVIE_OPT_FROM_RESET)
	{
		S9xReset();
		// save only SRAM for a from-reset snapshot
		result=(READ_STREAM(SRAM, 0x20000, stream) == 0x20000) ? SUCCESS : WRONG_FORMAT;
	}
	else
	{
		result=S9xUnfreezeFromStream(stream);
	}
	CLOSE_STREAM(stream);

	if(result!=SUCCESS)
	{
		return result;
	}

	if(!(fd=fopen(filename, read_only ? "rb" : "rb+")))
		return FILE_NOT_FOUND;

	if(fseek(fd, Movie.ControllerDataOffset, SEEK_SET))
		return WRONG_FORMAT;

	// read controller data
	Movie.File=fd;
	Movie.BytesPerFrame=bytes_per_frame();
	Movie.InputBufferPtr=Movie.InputBuffer;
	uint32 to_read=Movie.BytesPerFrame * (Movie.MaxFrame+1);
	reserve_buffer_space(to_read);
	fread(Movie.InputBufferPtr, 1, to_read, fd);

	// read "baseline" controller data
	read_frame_controller_data();

	strncpy(Movie.Filename, filename, _MAX_PATH);
	Movie.Filename[_MAX_PATH-1]='\0';
	Movie.CurrentFrame=0;
	Movie.ReadOnly=read_only;
	change_state(MOVIE_STATE_PLAY);

	S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_REPLAY*/);
	return SUCCESS;
}

int S9xMovieCreate (const char* filename, uint8 controllers_mask, uint8 opts, const wchar_t* metadata, int metadata_length)
{
	FILE* fd;
	STREAM stream;
	int fn;

	if(controllers_mask==0)
		return WRONG_FORMAT;

	if(!(fd=fopen(filename, "wb")))
		return FILE_NOT_FOUND;

	// stop current movie before opening
	change_state(MOVIE_STATE_NONE);

	if(metadata_length>MOVIE_MAX_METADATA)
	{
		metadata_length=MOVIE_MAX_METADATA;
	}

	Movie.MovieId=(uint32)time(NULL);
	Movie.RerecordCount=0;
	Movie.MaxFrame=0;
	Movie.SaveStateOffset=SMV_HEADER_SIZE+(sizeof(uint16)*metadata_length);
	Movie.ControllerDataOffset=0;
	Movie.ControllersMask=controllers_mask;
	Movie.Opts=opts;
	if(Settings.PAL)
	{
		Movie.Opts |= MOVIE_OPT_PAL;
	}
	else
	{
		Movie.Opts &= ~MOVIE_OPT_PAL;
	}

	write_movie_header(fd, &Movie);

	// convert wchar_t metadata string/array to a uint16 array
	if(metadata_length>0)
	{
		uint8 meta_buf[MOVIE_MAX_METADATA * sizeof(uint16)];
		int i;

		for(i=0; i<metadata_length; ++i)
		{
			uint16 c=(uint16)metadata[i];
			meta_buf[i+i]  =(uint8)(c&0xff);
			meta_buf[i+i+1]=(uint8)((c>>8)&0xff);
		}

		fwrite(meta_buf, sizeof(uint16), metadata_length, fd);
	}

	// write snapshot
	fn=dup(fileno(fd));
	fclose(fd);

	// lseek(fn, Movie.SaveStateOffset, SEEK_SET);
	if(!(stream=REOPEN_STREAM(fn, "ab")))
		return FILE_NOT_FOUND;

	if(opts & MOVIE_OPT_FROM_RESET)
	{
		S9xReset();
		// save only SRAM for a from-reset snapshot
		WRITE_STREAM(SRAM, 0x20000, stream);
	}
	else
	{
		S9xFreezeToStream(stream);
	}
	CLOSE_STREAM(stream);

	if(!(fd=fopen(filename, "rb+")))
		return FILE_NOT_FOUND;

	fseek(fd, 0, SEEK_END);
	Movie.ControllerDataOffset=(uint32)ftell(fd);

	// write "baseline" controller data
	Movie.File=fd;
	Movie.BytesPerFrame=bytes_per_frame();
	Movie.InputBufferPtr=Movie.InputBuffer;
	write_frame_controller_data();

	strncpy(Movie.Filename, filename, _MAX_PATH);
	Movie.Filename[_MAX_PATH-1]='\0';
	Movie.CurrentFrame=0;
	Movie.ReadOnly=false;
	change_state(MOVIE_STATE_RECORD);

	S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_RECORD*/);
	return SUCCESS;
}

void S9xMovieUpdate ()
{
	switch(Movie.State)
	{
	case MOVIE_STATE_PLAY:
		if(Movie.CurrentFrame>=Movie.MaxFrame)
		{
			change_state(MOVIE_STATE_NONE);
			S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_END*/);
			return;
		}
		else
		{
			if(Movie.FrameDisplay)
			{
				sprintf(Movie.FrameDisplayString, "Playing frame: %d", Movie.CurrentFrame);
				S9xMessage (S9X_INFO, S9X_MOVIE_INFO, Movie.FrameDisplayString);
			}
			read_frame_controller_data();
			++Movie.CurrentFrame;
		}
		break;

	case MOVIE_STATE_RECORD:
		{
			if(Movie.FrameDisplay)
			{
				sprintf(Movie.FrameDisplayString, "Recording frame: %d", Movie.CurrentFrame);
				S9xMessage (S9X_INFO, S9X_MOVIE_INFO, Movie.FrameDisplayString);
			}
			write_frame_controller_data();
			++Movie.CurrentFrame;
			Movie.MaxFrame=Movie.CurrentFrame;
			fwrite((Movie.InputBufferPtr - Movie.BytesPerFrame), 1, Movie.BytesPerFrame, Movie.File);
		}
		break;

	default:
		break;
	}
}

void S9xMovieStop (bool8 suppress_message)
{
	if(Movie.State!=MOVIE_STATE_NONE)
	{
		change_state(MOVIE_STATE_NONE);

		if(!suppress_message)
			S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_STOP*/);
	}
}

int S9xMovieGetInfo (const char* filename, struct MovieInfo* info)
{
	FILE* fd;
	int result;
	SMovie local_movie;
	int metadata_length;

	memset(info, 0, sizeof(*info));
	if(!(fd=fopen(filename, "rb")))
		return FILE_NOT_FOUND;

	if((result=(read_movie_header(fd, &local_movie)))!=SUCCESS)
		return result;

	info->TimeCreated=(time_t)local_movie.MovieId;
	info->LengthFrames=local_movie.MaxFrame;
	info->RerecordCount=local_movie.RerecordCount;
	info->Opts=local_movie.Opts;
	info->ControllersMask=local_movie.ControllersMask;

	if(local_movie.SaveStateOffset > SMV_HEADER_SIZE)
	{
		uint8 meta_buf[MOVIE_MAX_METADATA * sizeof(uint16)];
		int i;

		metadata_length=((int)local_movie.SaveStateOffset-SMV_HEADER_SIZE)/sizeof(uint16);
		metadata_length=(metadata_length>=MOVIE_MAX_METADATA) ? MOVIE_MAX_METADATA-1 : metadata_length;
		metadata_length=(int)fread(meta_buf, sizeof(uint16), metadata_length, fd);

		for(i=0; i<metadata_length; ++i)
		{
			uint16 c=meta_buf[i+i] | (meta_buf[i+i+1] << 8);
			info->Metadata[i]=(wchar_t)c;
		}
		info->Metadata[i]='\0';
	}
	else
	{
		info->Metadata[0]='\0';
	}

	fclose(fd);

	if(access(filename, W_OK))
		info->ReadOnly=true;

	return SUCCESS;
}

bool8 S9xMovieActive ()
{
	return (Movie.State!=MOVIE_STATE_NONE);
}

bool8 S9xMovieReadOnly ()
{
	if(!S9xMovieActive())
		return false;

	return Movie.ReadOnly;
}

uint32 S9xMovieGetId ()
{
	if(!S9xMovieActive())
		return 0;

	return Movie.MovieId;
}

uint32 S9xMovieGetLength ()
{
	if(!S9xMovieActive())
		return 0;

	return Movie.MaxFrame;
}

uint32 S9xMovieGetFrameCounter ()
{
	if(!S9xMovieActive())
		return 0;

	return Movie.CurrentFrame;
}

void S9xMovieToggleFrameDisplay ()
{
	Movie.FrameDisplay = !Movie.FrameDisplay;
	if(!Movie.FrameDisplay)
	{
		GFX.InfoStringTimeout = 1;
	}
}

void S9xMovieFreeze (uint8** buf, uint32* size)
{
	// sanity check
	if(!S9xMovieActive())
	{
		return;
	}

	*buf = NULL;
	*size = 0;

	// compute size needed for the buffer
	uint32 size_needed = 4*3;			// room for MovieId, CurrentFrame, and MaxFrame
	size_needed += (uint32)(Movie.BytesPerFrame * (Movie.MaxFrame+1));
	*buf=new uint8[size_needed];
	*size=size_needed;

	uint8* ptr = *buf;
	if(!ptr)
	{
		return;
	}

	Write32(Movie.MovieId, ptr);
	Write32(Movie.CurrentFrame, ptr);
	Write32(Movie.MaxFrame, ptr);

	memcpy(ptr, Movie.InputBuffer, Movie.BytesPerFrame * (Movie.MaxFrame+1));
}

bool8 S9xMovieUnfreeze (const uint8* buf, uint32 size)
{
	// sanity check
	if(!S9xMovieActive())
	{
		return false;
	}

	const uint8* ptr = buf;
	if(size < 4*3)
	{
		return false;
	}

	uint32 movie_id = Read32(ptr);
	uint32 current_frame = Read32(ptr);
	uint32 max_frame = Read32(ptr);
	uint32 space_needed = (Movie.BytesPerFrame * (max_frame+1));

	if(movie_id != Movie.MovieId ||
		current_frame > max_frame ||
		space_needed > size)
	{
		return false;
	}

	if(!Movie.ReadOnly)
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentframe pointer
		change_state(MOVIE_STATE_RECORD);
		S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_RERECORD*/);

		Movie.CurrentFrame = current_frame;
		Movie.MaxFrame = max_frame;
		++Movie.RerecordCount;

		reserve_buffer_space(space_needed);
		memcpy(Movie.InputBuffer, ptr, space_needed);
		flush_movie();
		fseek(Movie.File, Movie.ControllerDataOffset+(Movie.BytesPerFrame * (Movie.CurrentFrame+1)), SEEK_SET);
	}
	else
	{
		// here, we are going to keep the input data from the movie file
		// and simply rewind to the currentframe pointer
		// this will cause a desync if the savestate is not in sync
		// with the on-disk recording data, but it's easily solved
		// by loading another savestate or playing the movie from the beginning

		// and older savestate might have a currentframe pointer past
		// the end of the input data, so check for that here
		if(current_frame > Movie.MaxFrame)
		{
			return false;
		}

		change_state(MOVIE_STATE_PLAY);
		S9xMessage(S9X_INFO, S9X_MOVIE_INFO, 0/*MOVIE_INFO_REWIND*/);

		Movie.CurrentFrame = current_frame;
	}

	Movie.InputBufferPtr = Movie.InputBuffer + (Movie.BytesPerFrame * Movie.CurrentFrame);
	read_frame_controller_data();

	return true;
}
