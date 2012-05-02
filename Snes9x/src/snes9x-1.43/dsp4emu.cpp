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

// Due recognition and credit are given on Overload's DSP website.
// Thank those contributors for their hard work on this chip.

#include "dsp4.h"

#define DSP4_READ_WORD(x) \
	READ_WORD(DSP4.parameters+x)

#define DSP4_WRITE_WORD(x,d) \
	WRITE_WORD(DSP4.output+x,d);

// used to wait for dsp i/o
#define DSP4_WAIT(x) \
	DSP4_Logic = x; return;

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void DSP4_Multiply(short Multiplicand, short Multiplier, int &Product)
{
	Product = (Multiplicand * Multiplier << 1) >> 1;
}

void DSP4_UnknownOP11(short A, short B, short C, short D, short &M)
{
	// 0x155 = 341 = Horizontal Width of the Screen
	M = ((A * 0x0155 >>  2) & 0xf000) | ((B * 0x0155 >>  6) & 0x0f00) | 
	    ((C * 0x0155 >> 10) & 0x00f0) | ((D * 0x0155 >> 14) & 0x000f); 
}

const unsigned short Op0A_Values[16] = {
	0x0000, 0x0030, 0x0060, 0x0090, 0x00c0, 0x00f0, 0x0120, 0x0150,
	0xfe80, 0xfeb0, 0xfee0, 0xff10, 0xff40, 0xff70, 0xffa0, 0xffd0
};

void DSP4_Op0A(short n2, short &o1, short &o2, short &o3, short &o4)
{
	o4 = Op0A_Values[(n2 & 0x000f)];
	o3 = Op0A_Values[(n2 & 0x00f0) >> 4];
	o2 = Op0A_Values[(n2 & 0x0f00) >> 8];
	o1 = Op0A_Values[(n2 & 0xf000) >> 12];
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

#if OP==0x0009
#define PRINT
#endif

void DSP4_Op03()
{
#ifdef PRINT
	printf("Op03\n");
#endif

	MaxTilesPerRow = 33;

	for (int i=0; i < 32; i++)
		RowCount[i] = 0;
	
	// reset op09 data
	op09_mode = 0;
}

void DSP4_Op0E()
{
#ifdef PRINT
	printf("Op0E\n");
#endif

	MaxTilesPerRow = 16;

	for (int i=0; i < 32; i++)
		RowCount[i] = 0;
	
	// reset op09 data
	op09_mode = 1;
}

#undef PRINT

void DSP4_Op06(bool8 size, bool8 msb)
{
	// save post-oam table data for future retrieval
	op06_OAM[op06_index] |= (msb<<(op06_offset+0));
	op06_OAM[op06_index] |= (size<<(op06_offset+1));
	op06_offset += 2;

	if(op06_offset==8)
	{
		// move to next byte in buffer
		op06_offset=0;
		op06_index++;
	}
}

#if OP==0x0001
#define PRINT
#endif

void DSP4_Op01()
{
	uint16 command;

	DSP4.waiting4command = FALSE;

	// op flow control
	switch(DSP4_Logic) {
		case 1: goto resume1; break;
		case 2: goto resume2; break;
	}

	////////////////////////////////////////////////////
	// process initial inputs

	// sort inputs
	// 0x00 = DSP4_READ_WORD(0x00);
	project_focaly = DSP4_READ_WORD(0x02);
	raster = DSP4_READ_WORD(0x04);
	viewport_top = DSP4_READ_WORD(0x06);
	project_y = DSP4_READ_WORD(0x08);
	viewport_bottom = DSP4_READ_WORD(0x0a);
	project_x1low = DSP4_READ_WORD(0x0c);
	project_focalx = DSP4_READ_WORD(0x0e);
	project_centerx = DSP4_READ_WORD(0x10);
	project_ptr = DSP4_READ_WORD(0x12);
	// (envelope?) 0xc0 = DSP4_READ_WORD(0x14);
	project_pitchylow = DSP4_READ_WORD(0x16);
	project_pitchy = DSP4_READ_WORD(0x18);
	project_pitchxlow = DSP4_READ_WORD(0x1a);
	project_pitchx = DSP4_READ_WORD(0x1c);
	far_plane = DSP4_READ_WORD(0x1e);
	// ? = DSP4_READ_WORD(0x20);
	project_y1low = DSP4_READ_WORD(0x22);

	// pre-compute
	view_plane = PLANE_START;

	// find starting projection points
	project_x1 = project_focalx;
	project_y -= viewport_bottom;

	// multi-op storage
	multi_index1 = 0;
	multi_index2 = 0;

	// debug
	block=0;

	////////////////////////////////////////////////////
	// command check

	do {
		// scan next command
		DSP4.in_count = 2;

		DSP4_WAIT(1) resume1:

		// inspect input
		command = DSP4_READ_WORD(0);

		// check for termination
		if(command == 0x8000) break;

		// already have 2 bytes in queue
		DSP4.in_index = 2;
		DSP4.in_count = 8;

		DSP4_WAIT(2) resume2:

		////////////////////////////////////////////////////
		// process one iteration of projection

		// inspect inputs
		int16 plane = DSP4_READ_WORD(0);
		int16 index, lcv;
		int16 py_dy=0, px_dx=0;
		int16 y_out, x_out;
		int16 envelope = DSP4_READ_WORD(6);
		int16 project_x;

		// ignore invalid data
		if((uint16) plane == 0x8001) continue;

		// one-time init
		if(far_plane)
		{
			// setup final parameters
			project_focalx += plane;
			project_x1 = project_focalx;
			project_y1 = project_focaly;
			plane = far_plane;
			far_plane = 0;

			// track occlusion
			multi_farplane[1] = plane;
			multi_raster[1] = viewport_bottom;
		}

		// use proportional triangles to project new coords
		project_x2 = project_focalx * plane / view_plane;
		project_y2 = project_focaly * plane / view_plane;

		// quadratic regression (rough)
		if(project_focaly>=-0x0f) 
			py_dy = project_focaly * project_focaly * -0.20533553
							- 1.08330005 * project_focaly - 69.61094639;
		else
			py_dy = project_focaly * project_focaly * -0.000657035759
							- 1.07629051 * project_focaly - 65.69315963;

		// approximate # of raster lines
		segments = abs(project_y2-project_y1);

		// prevent overdraw
		if(project_y2>=raster) segments=0;
		else raster=project_y2;

		// don't draw outside the window
		if(project_y2<viewport_top) segments=0;

		// project new positions
		if(segments>0)
		{
			// interpolate between projected points
			px_dx = ((project_x2-project_x1)<<8)/segments;
		}

		// debug
		++block;
#ifdef PRINT
		printf("(line %d) Op01 check %02X, plane %04X, focal_y %04X, y2 %04X, y1 %04X\n",c,(uint16)segments,(uint16)(plane),(uint16)project_focaly,(uint16)project_y2,(uint16)project_y1);
#endif

		// prepare output
		DSP4.out_count=8+2+6*segments;

		// pre-block data
		DSP4_WRITE_WORD(0,project_focalx);
		DSP4_WRITE_WORD(2,project_x2);
		DSP4_WRITE_WORD(4,project_focaly);
		DSP4_WRITE_WORD(6,project_y2);
		DSP4_WRITE_WORD(8,segments);
		
#if 0
		DSP4_WRITE_WORD(0,-1);
		DSP4_WRITE_WORD(2,-1);
		DSP4_WRITE_WORD(4,-1);
		DSP4_WRITE_WORD(6,-1);
		DSP4_WRITE_WORD(8,-1);
#endif

		index=10;

		project_x = project_centerx + project_x1;

		// iterate through each point
		for( lcv=0; lcv<segments; lcv++ )
		{
			// step through the projected line
			y_out = project_y+((py_dy*lcv)>>8);
			x_out = project_x+((px_dx*lcv)>>8);

			// factor in dynamic track changes
			y_out += envelope;

#if 0
			project_ptr=-1;
			y_out=-1;
			//x_out=-1;
#endif

			// data
			DSP4_WRITE_WORD(index+0,project_ptr);
			DSP4_WRITE_WORD(index+2,y_out);
			DSP4_WRITE_WORD(index+4,x_out);
			index += 6;

			// post-update
			project_ptr -= 4;
		}

		// post-update
		project_y += ((py_dy*lcv)>>8);

		// new positions
		if(segments>0)
		{
			project_x1 = project_x2;
			project_y1 = project_y2;

			// storage of focals for op07
			multi_focaly[multi_index2++] = project_focaly;

			// track occlusion: can't see anything below it
			multi_farplane[1] = plane;
			multi_raster[1] -= segments;
		}

		// update projection points
		project_pitchy += (int8)DSP4.parameters[3];
		project_pitchx += (int8)DSP4.parameters[5];

		project_focaly += project_pitchy;
		project_focalx += project_pitchx;
	} while (1);

	// terminate op
	DSP4.waiting4command = TRUE;
	DSP4.out_count = 0;
}

#undef PRINT

#if OP==0x0007
#define PRINT
#endif

void DSP4_Op07()
{
	uint16 command;

	DSP4.waiting4command = FALSE;

	// op flow control
	switch(DSP4_Logic) {
		case 1: goto resume1; break;
		case 2: goto resume2; break;
	}

	////////////////////////////////////////////////////
	// sort inputs

	// 0x00 = DSP4_READ_WORD(0x00);
	project_focaly = DSP4_READ_WORD(0x02);
	raster = DSP4_READ_WORD(0x04);
	viewport_top = DSP4_READ_WORD(0x06);
	project_y = DSP4_READ_WORD(0x08);
	viewport_bottom = DSP4_READ_WORD(0x0a);
	project_x1low = DSP4_READ_WORD(0x0c);
	project_x1 = DSP4_READ_WORD(0x0e);
	project_centerx = DSP4_READ_WORD(0x10);
	project_ptr = DSP4_READ_WORD(0x12);
	// (envelope?) 0xc0 = DSP4_READ_WORD(0x14);

	// pre-compute
	view_plane = PLANE_START;

	// find projection targets
	project_y1 = project_focaly;
	project_y -= viewport_bottom;

	// multi-op storage
	multi_index2 = 0;

	// debug
	block=0;

#ifdef PRINT
	printf("(line %d) Op07 data %04X\n",c,(uint16)project_y1);
#endif

	////////////////////////////////////////////////////
	// command check

	do {
		// scan next command
		DSP4.in_count = 2;

		DSP4_WAIT(1) resume1:

		// inspect input
		command = DSP4_READ_WORD(0);

		// check for opcode termination
		if(command == 0x8000) break;

			// already have 2 bytes in queue
		DSP4.in_index = 2;
		DSP4.in_count = 12;

		DSP4_WAIT(2) resume2:

		////////////////////////////////////////////////////
		// process one loop of projection

		int16 plane;
		int16 index,lcv;
		int16 y_out,x_out;
		int16 py_dy=0,px_dx=0;
		int16 project_x;

		// debug
		++block;

		// inspect inputs
		plane = DSP4_READ_WORD(0);
		project_y2 = DSP4_READ_WORD(2);
		// ? = DSP4_READ_WORD(4);
		project_x2 = DSP4_READ_WORD(6);
		int16 envelope = DSP4_READ_WORD(8);

		// ignore invalid data
		if((uint16) plane == 0x8001) continue;

		// multi-op storage
		project_focaly = multi_focaly[multi_index2];

		// quadratic regression (rough)
		if(project_focaly>=-0x0f) 
			py_dy = project_focaly * project_focaly * -0.20533553
							- 1.08330005 * project_focaly - 69.61094639;
		else
			py_dy = project_focaly * project_focaly * -0.000657035759
							- 1.07629051 * project_focaly - 65.69315963;

		// approximate # of raster lines
		segments = abs(project_y2-project_y1);

		// prevent overdraw
		if(project_y2>=raster) segments=0;
		else raster=project_y2;

		// don't draw outside the window
		if(project_y2<viewport_top) segments=0;

		// project new positions
		if(segments>0)
		{
			// interpolate between projected points
			px_dx = ((project_x2-project_x1)<<8)/segments;
		}

#ifdef PRINT
		printf("(line %d) Op07 block %d, loc %04X, out %02X, project_x2 %04X\n",c,block,plane,segments,(uint16)project_x2);
#endif

		// prepare pre-output
		DSP4.out_count=4+2+6*segments;

		DSP4_WRITE_WORD(0,project_x2);
		DSP4_WRITE_WORD(2,project_y2);
		DSP4_WRITE_WORD(4,segments);

#if 0
		DSP4_WRITE_WORD(0,-1);
		DSP4_WRITE_WORD(2,-1);
		DSP4_WRITE_WORD(4,-1);
#endif

		index=6;

		project_x = project_centerx + project_x1;

		for( lcv=0; lcv<segments; lcv++ )
		{
			// pre-compute
			y_out = project_y+((py_dy*lcv)>>8);
			x_out = project_x+((px_dx*lcv)>>8);

			// factor in dynamic track changes
			y_out += envelope;

#if 0
			project_ptr = -1;
			//y_out = -1;
			x_out = -1;
#endif

			// data
			DSP4_WRITE_WORD(index+0,project_ptr);
			DSP4_WRITE_WORD(index+2,y_out);
			DSP4_WRITE_WORD(index+4,x_out);
			index += 6;

			// post-update
			project_ptr -= 4;
		}

		// update internal variables
		project_y += ((py_dy*lcv)>>8);

		// new positions
		if(segments>0)
		{
			project_x1 = project_x2;
			project_y1 = project_y2;

			// multi-op storage
			multi_index2++;
		}
	} while(1);

	DSP4.waiting4command = TRUE;
	DSP4.out_count = 0;
}

#undef PRINT

#if OP==0x0008
#define PRINT
#endif

void DSP4_Op08()
{
	uint16 command;

	DSP4.waiting4command = FALSE;

	// op flow control
	switch(DSP4_Logic) {
		case 1: goto resume1; break;
		case 2: goto resume2; break;
	}

	////////////////////////////////////////////////////
	// process initial inputs

	// clip values
	path_clipRight[0] = DSP4_READ_WORD(0x00);
	path_clipRight[1] = DSP4_READ_WORD(0x02);
	path_clipRight[2] = DSP4_READ_WORD(0x04);
	path_clipRight[3] = DSP4_READ_WORD(0x06);

	path_clipLeft[0] = DSP4_READ_WORD(0x08);
	path_clipLeft[1] = DSP4_READ_WORD(0x0a);
	path_clipLeft[2] = DSP4_READ_WORD(0x0c);
	path_clipLeft[3] = DSP4_READ_WORD(0x0e);

	// unknown (constant)
	// unknown (constant)

	// path positions
	path_pos[0] = DSP4_READ_WORD(0x20);
	path_pos[1] = DSP4_READ_WORD(0x22);
	path_pos[2] = DSP4_READ_WORD(0x24);
	path_pos[3] = DSP4_READ_WORD(0x26);

	// data locations
	path_ptr[0] = DSP4_READ_WORD(0x28);
	path_ptr[1] = DSP4_READ_WORD(0x2a);
	path_ptr[2] = DSP4_READ_WORD(0x2c);
	path_ptr[3] = DSP4_READ_WORD(0x2e);

	// project_y1 lines
	path_raster[0] = DSP4_READ_WORD(0x30);
	path_raster[1] = DSP4_READ_WORD(0x32);
	path_raster[2] = DSP4_READ_WORD(0x34);
	path_raster[3] = DSP4_READ_WORD(0x36);

	// viewport_top
	path_top[0] = DSP4_READ_WORD(0x38);
	path_top[1] = DSP4_READ_WORD(0x3a);
	path_top[2] = DSP4_READ_WORD(0x3c);
	path_top[3] = DSP4_READ_WORD(0x3e);

	// unknown (constants)

	view_plane = PLANE_START;

	// debug
	block=0;

	////////////////////////////////////////////////////
	// command check

	do {
		// scan next command
		DSP4.in_count = 2;

		DSP4_WAIT(1) resume1:
	
		// inspect input
		command = DSP4_READ_WORD(0);

		// terminate op
		if(command == 0x8000) break;

		// already have 2 bytes in queue
		DSP4.in_index = 2;
		DSP4.in_count = 18;

		DSP4_WAIT(2) resume2:

		////////////////////////////////////////////////////
		// projection begins

		// debug
		++block;

		// used in envelope shaping
		int16 x1_final;
		int16 x2_final;

		// look at guidelines
		int16 plane = DSP4_READ_WORD(0x00);
		int16 x_left = DSP4_READ_WORD(0x02);
		int16 y_left = DSP4_READ_WORD(0x04);
		int16 x_right = DSP4_READ_WORD(0x06);
		int16 y_right = DSP4_READ_WORD(0x08);

		// envelope guidelines (one frame only)
		int16 envelope1 = DSP4_READ_WORD(0x0a);
		int16 envelope2 = DSP4_READ_WORD(0x0c);
		int16 envelope3 = DSP4_READ_WORD(0x0e);
		int16 envelope4 = DSP4_READ_WORD(0x10);

		// ignore invalid data
		if((uint16) plane == 0x8001) continue;

		// first init
		if(plane == 0x7fff)
		{
			int pos1,pos2;

			// initialize projection
			path_x[0] = x_left;
			path_x[1] = x_right;

			path_y[0] = y_left;
			path_y[1] = y_right;

			// update coordinates
			path_pos[0]-=x_left;
			path_pos[1]-=x_left;
			path_pos[2]-=x_right;
			path_pos[3]-=x_right;

			pos1 = path_pos[0]+envelope1;
			pos2 = path_pos[1]+envelope2;

			// clip offscreen data
			if(pos1<path_clipLeft[0]) pos1 = path_clipLeft[0];
			if(pos1>path_clipRight[0]) pos1 = path_clipRight[0];
			if(pos2<path_clipLeft[1]) pos2 = path_clipLeft[1];
			if(pos2>path_clipRight[1]) pos2 = path_clipRight[1];

#if 0
			pos1=-1;
			//pos2=-1;
#endif

			path_plane[0] = plane;
			path_plane[1] = plane;

			// initial output
			DSP4.out_count = 2;
			DSP4.output[0]=pos1&0xFF;
			DSP4.output[1]=pos2&0xFF;

#ifdef PRINT
			printf("(line %d) Op08 x_left %04X\n",c,(uint16)x_left);
#endif
		}
		// proceed with projection
		else
		{
			int16 index=0, lcv;
			int16 left_inc=0,right_inc=0;
			int16 dx1,dx2,dx3,dx4;

			// # segments to traverse
			segments = abs(y_left - path_y[0]);

			// prevent overdraw
			if(y_left>=path_raster[0]) segments=0;
			else path_raster[0]=y_left;

			// don't draw outside the window
			if(path_raster[0]<path_top[0]) segments=0;

			// proceed if visibility rules apply
			if(segments>0)
			{
				// use previous data
				dx1 = (envelope1 * path_plane[0] / view_plane);
				dx2 = (envelope2 * path_plane[0] / view_plane);

				// use temporary envelope pitch (this frame only)
				dx3 = (envelope1 * plane / view_plane);
				dx4 = (envelope2 * plane / view_plane);

				// project new shapes (left side)
				x1_final = x_left+dx1;
				x2_final = path_x[0]+dx3;

				// interpolate between projected points with shaping
				left_inc = ((x2_final-x1_final)<<8)/segments;

				// project new shapes (right side)
				x1_final = x_left+dx2;
				x2_final = path_x[0]+dx4;

				// interpolate between projected points with shaping
				right_inc = ((x2_final-x1_final)<<8)/segments;

				path_plane[0] = plane;
			}

#ifdef PRINT
			printf("(line %d) Op08 block %d, out %02X, raster %02X\n",c,block,segments,(uint16)y_left);
#endif

			// zone 1
			DSP4.out_count = (2+4*segments);
			DSP4_WRITE_WORD(index,segments); index+=2;

			for( lcv=1; lcv<=segments; lcv++ )
			{
				int16 pos1,pos2;

				// pre-compute
				pos1 = path_pos[0]+((left_inc*lcv)>>8)+dx1;
				pos2 = path_pos[1]+((right_inc*lcv)>>8)+dx2;

				// clip offscreen data
				if(pos1<path_clipLeft[0]) pos1 = path_clipLeft[0];
				if(pos1>path_clipRight[0]) pos1 = path_clipRight[0];
				if(pos2<path_clipLeft[1]) pos2 = path_clipLeft[1];
				if(pos2>path_clipRight[1]) pos2 = path_clipRight[1];

#if 0
				if(pos1==0x00ff) pos1=0;
				if(pos2==0x00ff) pos2=0;
				path_ptr[0] = -1;
				pos1 = -1;
				pos2 = -1;
#endif

				// data
				DSP4_WRITE_WORD(index,path_ptr[0]); index+=2;
				DSP4.output[index++]=pos1&0xFF;
				DSP4.output[index++]=pos2&0xFF;

				// post-update
				path_ptr[0] -= 4;
				path_ptr[1] -= 4;
			}
			lcv--;

			if(segments>0)
			{
				// project points w/out the envelopes
				int16 inc = ((path_x[0]-x_left)<<8)/segments;

				// post-store
				path_pos[0] += ((inc*lcv)>>8);
				path_pos[1] += ((inc*lcv)>>8);

				path_x[0] = x_left;
				path_y[0] = y_left;
			}

			//////////////////////////////////////////////
			// zone 2
			segments = abs(y_right - path_y[1]);

			// prevent overdraw
			if(y_right>=path_raster[2]) segments=0;
			else path_raster[2]=y_right;

			// don't draw outside the window
			if(path_raster[2]<path_top[2]) segments=0;

			// proceed if visibility rules apply
			if(segments>0)
			{
				// use previous data
				dx1 = (envelope1 * path_plane[1] / view_plane);
				dx2 = (envelope2 * path_plane[1] / view_plane);

				// use temporary envelope pitch (this frame only)
				dx3 = (envelope1 * plane / view_plane);
				dx4 = (envelope2 * plane / view_plane);

				// project new shapes (left side)
				x1_final = x_left+dx1;
				x2_final = path_x[1]+dx3;

				// interpolate between projected points with shaping
				left_inc = ((x2_final-x1_final)<<8)/segments;

				// project new shapes (right side)
				x1_final = x_left+dx2;
				x2_final = path_x[1]+dx4;

				// interpolate between projected points with shaping
				right_inc = ((x2_final-x1_final)<<8)/segments;

				path_plane[1] = plane;
			}

			// write out results
			DSP4.out_count += (2+4*segments);
			DSP4_WRITE_WORD(index,segments); index+=2;

			for( lcv=1; lcv<=segments; lcv++ )
			{
				int16 pos1,pos2;

				// pre-compute
				pos1 = path_pos[2]+((left_inc*lcv)>>8)+dx1;
				pos2 = path_pos[3]+((right_inc*lcv)>>8)+dx2;

				// clip offscreen data
				if(pos1<path_clipLeft[2]) pos1 = path_clipLeft[2];
				if(pos1>path_clipRight[2]) pos1 = path_clipRight[2];
				if(pos2<path_clipLeft[3]) pos2 = path_clipLeft[3];
				if(pos2>path_clipRight[3]) pos2 = path_clipRight[3];

#if 0
				if(pos1==0x00ff) pos1=0;
				if(pos2==0x00ff) pos2=0;
				path_ptr[2] = -1;
				//pos1 = -1;
				pos2 = -1;
#endif

				// data
				DSP4_WRITE_WORD(index,path_ptr[2]); index+=2;
				DSP4.output[index++]=pos1&0xFF;
				DSP4.output[index++]=pos2&0xFF;

				// post-update
				path_ptr[2] -= 4;
				path_ptr[3] -= 4;
			}
			lcv--;

			if(segments>0)
			{
				// project points w/out the envelopes
				int16 inc = ((path_x[1]-x_right)<<8)/segments;

				// post-store
				path_pos[2] += ((inc*lcv)>>8);
				path_pos[3] += ((inc*lcv)>>8);

				path_x[1] = x_right;
				path_y[1] = y_right;
			}
		}
	} while(1);

	DSP4.waiting4command = TRUE;
	DSP4.out_count = 2;
	DSP4_WRITE_WORD(0,0);
}

#undef PRINT

#if OP==0x000D
#define PRINT
#endif

void DSP4_Op0D()
{
	uint16 command;

	DSP4.waiting4command = FALSE;

	// op flow control
	switch(DSP4_Logic) {
		case 1: goto resume1; break;
		case 2: goto resume2; break;
	}

	////////////////////////////////////////////////////
	// process initial inputs

	// sort inputs
	// 0x00 = DSP4_READ_WORD(0x00);
	project_focaly = DSP4_READ_WORD(0x02);
	raster = DSP4_READ_WORD(0x04);
	viewport_top = DSP4_READ_WORD(0x06);
	project_y = DSP4_READ_WORD(0x08);
	viewport_bottom = DSP4_READ_WORD(0x0a);
	project_x1low = DSP4_READ_WORD(0x0c);
	project_x1 = DSP4_READ_WORD(0x0e);
	project_focalx = DSP4_READ_WORD(0x0e);
	project_centerx = DSP4_READ_WORD(0x10);
	project_ptr = DSP4_READ_WORD(0x12);
	// 0xc0 = DSP4_READ_WORD(0x14);
	project_pitchylow = DSP4_READ_WORD(0x16);
	project_pitchy = DSP4_READ_WORD(0x18);
	project_pitchxlow = DSP4_READ_WORD(0x1a);
	project_pitchx = DSP4_READ_WORD(0x1c);
	far_plane = DSP4_READ_WORD(0x1e);
	// ? = DSP4_READ_WORD(0x20);

	// multi-op storage
	multi_index1++;
	multi_index1%=4;

	// remap 0D->09 window data ahead of time
	// index starts at 1-3,0
	//
	// Op0D: BL,TL,BR,TR
	// Op09: TL,TR,BL,BR (1,2,3,0)
	switch(multi_index1)
	{
		case 1: multi_index2=3; break;
		case 2: multi_index2=1; break;
		case 3: multi_index2=0; break;
		case 0: multi_index2=2; break;
	}

	// pre-compute
	view_plane = PLANE_START;

	// figure out projection data
	project_y -= viewport_bottom;

	// debug
	block=0;

	////////////////////////////////////////////////////
	// command check

	do {
		// scan next command
		DSP4.in_count = 2;
		
		DSP4_WAIT(1) resume1:

		// inspect input
		command = DSP4_READ_WORD(0);

		// terminate op
		if(command == 0x8000) break;

		// already have 2 bytes in queue
		DSP4.in_index = 2;
		DSP4.in_count = 8;

		DSP4_WAIT(2) resume2:

		////////////////////////////////////////////////////
		// project section of the track

		// inspect inputs
		int16 plane = DSP4_READ_WORD(0);
		int16 index, lcv;
		int16 py_dy=0, px_dx=0;
		int16 y_out, x_out;
		int16 envelope = DSP4_READ_WORD(6);
		int16 project_x;

		// ignore invalid data
		if((uint16) plane == 0x8001) continue;

		// one-time init
		if(far_plane)
		{
			// setup final data
			// low16=plane
			project_x1 = project_focalx;
			project_y1 = project_focaly;
			plane = far_plane;
			far_plane = 0;

			// track occlusion
			multi_farplane[multi_index2] = plane;
			multi_raster[multi_index2] = viewport_bottom;
		}

		// use proportional triangles to project new coords
		project_x2 = project_focalx * plane / view_plane;
		project_y2 = project_focaly * plane / view_plane;

		// quadratic regression (rough)
		if(project_focaly>=-0x0f) 
			py_dy = project_focaly * project_focaly * -0.20533553
							- 1.08330005 * project_focaly - 69.61094639;
		else
			py_dy = project_focaly * project_focaly * -0.000657035759
							- 1.07629051 * project_focaly - 65.69315963;

		// approximate # of raster lines
		segments = abs(project_y2-project_y1);

		// prevent overdraw
		if(project_y2>=raster) segments=0;
		else raster=project_y2;

		// don't draw outside the window
		if(project_y2<viewport_top) segments=0;

		// project new positions
		if(segments>0)
		{
			// interpolate between projected points
			px_dx = ((project_x2-project_x1)<<8)/segments;
		}

		// debug
		++block;

#ifdef PRINT
		printf("(line %d) Op0D check %02X, plane %04X\n",c,(uint16)segments,(uint16)(plane));
#endif

		// prepare output
		DSP4.out_count=8+2+6*segments;

		DSP4_WRITE_WORD(0,project_focalx);
		DSP4_WRITE_WORD(2,project_x2);
		DSP4_WRITE_WORD(4,project_focaly);
		DSP4_WRITE_WORD(6,project_y2);
		DSP4_WRITE_WORD(8,segments);
#if 0
		DSP4_WRITE_WORD(0,-1);
		DSP4_WRITE_WORD(2,-1);
		DSP4_WRITE_WORD(4,-1);
		//DSP4_WRITE_WORD(6,-1);
		DSP4_WRITE_WORD(8,-1);
#endif

		index=10;

		project_x = project_centerx + project_x1;

		// iterate through each point
		for( lcv=0; lcv<segments; lcv++ )
		{
			// step through the projected line
			y_out = project_y+((py_dy*lcv)>>8);
			x_out = project_x+((px_dx*lcv)>>8);

			// factor in dynamic track changes
			y_out += envelope;

#if 0
			project_ptr=-1;
			y_out=-1;
			x_out=-1;
#endif

			// data
			DSP4_WRITE_WORD(index+0,project_ptr);
			DSP4_WRITE_WORD(index+2,y_out);
			DSP4_WRITE_WORD(index+4,x_out);
			index += 6;

			// post-update
			project_ptr -= 4;
		}

		// post-update
		project_y += ((py_dy*lcv)>>8);

		if(segments > 0)
		{
			project_x1 = project_x2;
			project_y1 = project_y2;

			// track occlusion: can't see anything below it
			multi_farplane[multi_index2] = plane;
			multi_raster[multi_index2] -= segments;
		}

		// update focal projection points
		project_pitchy += (int8)DSP4.parameters[3];
		project_pitchx += (int8)DSP4.parameters[5];

		project_focaly += project_pitchy;
		project_focalx += project_pitchx;
	} while(1);
	
	DSP4.waiting4command = TRUE;
	DSP4.out_count = 0;
}

#undef PRINT

#if OP==0x0009
#define PRINT
#endif

#if OP==0x0006
#define PRINT
#endif

void DSP4_Op09()
{
	uint16 command;

	DSP4.waiting4command = FALSE;

	// op flow control
	switch(DSP4_Logic) {
		case 1: goto resume1; break;
		case 2: goto resume2; break;
		case 3: goto resume3; break;
		case 4: goto resume4; break;
		case 5: goto resume5; break;
		case 6: goto resume6; break;
		case 7: goto resume7; break;
	}

	////////////////////////////////////////////////////
	// process initial inputs

	// debug
	block=0;

	// grab screen information
	view_plane = PLANE_START;
	center_x = DSP4_READ_WORD(0x00);
	center_y = DSP4_READ_WORD(0x02);
	// 0x00 = DSP4_READ_WORD(0x04);
	viewport_left = DSP4_READ_WORD(0x06);
	viewport_right = DSP4_READ_WORD(0x08);
	viewport_top = DSP4_READ_WORD(0x0a);
	viewport_bottom = DSP4_READ_WORD(0x0c);

#ifdef PRINT2
	printf("Window: (%04X,%04X) (%04X,%04X)\n",
		viewport_left,viewport_right,viewport_top,viewport_bottom);
#endif

	// cycle through viewport window data
	multi_index1++;
	multi_index1%=4;

	goto no_sprite;

	do {
		////////////////////////////////////////////////////
		// check for new sprites

		do {
			uint16 second;

			DSP4.in_count = 4;
			DSP4.in_index = 2;

			DSP4_WAIT(1) resume1:

			// try to classify sprite
			second = DSP4_READ_WORD(2);

			// op termination
			if(second == 0x8000) goto terminate;

			second >>= 8;
			sprite_type = 0;

			// vehicle sprite
			if(second == 0x90)
			{
				sprite_type = 1;
				break;
			}
			// terrain sprite
			else if(second != 0)
			{
				sprite_type = 2;
				break;
			}

no_sprite:
			// no sprite. try again

			DSP4.in_count = 2;

			DSP4_WAIT(2) resume2:
			;
		} while (1);

		////////////////////////////////////////////////////
		// process projection information

sprite_found:
		// vehicle sprite
		if(sprite_type == 1)
		{
			int16 plane;
			int16 car_left, car_right, car_left_a;
			int16 focal_back, focal_front;
			uint8 distance/*, id*/;

			// we already have 4 bytes we want
			DSP4.in_count = 6+12;
			DSP4.in_index = 4;

			DSP4_WAIT(3) resume3:

			// filter inputs
			project_y1 = DSP4_READ_WORD(0x00);
			// 0x9000 = DSP4_READ_WORD(0x02);
			//id = DSP4.parameters[0x04];
			distance = DSP4.parameters[0x05];
			focal_back = DSP4_READ_WORD(0x06);
			focal_front = DSP4_READ_WORD(0x08);
			car_left_a = DSP4_READ_WORD(0x0a);
			car_left = DSP4_READ_WORD(0x0c);
			plane = DSP4_READ_WORD(0x0e);
			car_right = DSP4_READ_WORD(0x10);

			// calculate car's x-center
			project_focalx = car_right-car_left;

			// determine how far into the screen to project
			project_focaly = focal_back;
			project_x = project_focalx * plane / view_plane;
			segments = 0x33 - project_focaly * plane / view_plane;
			far_plane = plane;

			// prepare memory
			sprite_x = center_x+project_x;
			sprite_y = viewport_bottom-segments;
			far_plane = plane;

			// debug
			++block;
#ifdef PRINT
			printf("(line %d) Op09 vehicle block %d, Loop %04X\n",c,block,(uint16)project_y1);
			//printf("%04X %04X %04X %04X / ",focal_back,focal_front,car_left_a,car_left);
			//printf("%02X %02X ", distance, id);
#endif

			// make the car's x-center available
			DSP4.out_count = 2;
			DSP4_WRITE_WORD(0,project_focalx);

#if 0
			DSP4_WRITE_WORD(0,-1);
#endif

			// grab a few remaining vehicle values
			DSP4.in_count = 4;

			DSP4_WAIT(4) resume4:

			// store final values
			int height = DSP4_READ_WORD(0);
			sprite_offset = DSP4_READ_WORD(2);

			// vertical lift factor
			sprite_y += height;

#ifdef PRINT_09
			printf("%04X\n",sprite_offset);
#endif
		}
		// terrain sprite
		else if(sprite_type == 2)
		{
			int16 plane;

			// we already have 4 bytes we want
			DSP4.in_count = 6+6+2;
			DSP4.in_index = 4;

			DSP4_WAIT(5) resume5:

			// sort loop inputs
			project_y1 = DSP4_READ_WORD(0x00);
			plane = DSP4_READ_WORD(0x02);
			project_centerx = DSP4_READ_WORD(0x04);
			//project_y1 = DSP4_READ_WORD(0x06);
			project_focalx = DSP4_READ_WORD(0x08);
			project_focaly = DSP4_READ_WORD(0x0a);
			sprite_offset = DSP4_READ_WORD(0x0c);

			// determine distances into virtual world
			segments = 0x33 - project_y1;
			project_x = project_focalx * plane / view_plane;
			project_y = project_focaly * plane / view_plane;

			// prepare memory
			sprite_x = center_x+project_x-project_centerx;
			sprite_y = viewport_bottom-segments+project_y;
			far_plane = plane;

			// debug
			++block;
#ifdef PRINT
			printf("(line %d) Op09 terrain block %d, Loop %04X\n",c,block,(uint16)project_y1);
#endif
		}

		// default sprite size: 16x16
		sprite_size = 1;

		////////////////////////////////////////////////////
		// convert tile data to OAM

		do {
			DSP4.in_count = 2;

			DSP4_WAIT(6) resume6:

			command = DSP4_READ_WORD(0);

			// opcode termination
			if(command == 0x8000) goto terminate;

			// toggle sprite size
			if(command == 0x0000)
			{
				sprite_size = !sprite_size;
#ifdef PRINT
				//printf("TOGGLE=%02X\n",(uint8)sprite_size);
#endif
				continue;
			}

			// new sprite information
			command >>= 8;
			if(command != 0x20 && command != 0x40 &&
				 command != 0x60 && command != 0xa0 &&
				 command != 0xc0 && command != 0xe0)
				break;

			DSP4.in_count = 6;
			DSP4.in_index = 2;

			DSP4_WAIT(7) resume7:

			/////////////////////////////////////
			// process tile data

			bool8 clip;
			int16 sp_x, sp_y, sp_oam, sp_msb;
			int16 sp_dx, sp_dy;
			int16 expand = sprite_size ? 15:7;

			// sprite deltas
			sp_dy = DSP4_READ_WORD(2);
			sp_dx = DSP4_READ_WORD(4);

			// update coordinates
			sp_y = sprite_y + sp_dy;
			sp_x = sprite_x + sp_dx;

			// reject points outside the clipping window
			clip = FALSE;

			// window clipping
			if(sp_x < viewport_left-expand || sp_x > viewport_right) clip=TRUE;
			if(sp_y < viewport_top || sp_y > viewport_bottom) clip=TRUE;

			// track occlusion:
			// clip any tiles that are below the road
			if(far_plane <= multi_farplane[multi_index1] &&
				 (sp_y>>3) >= (multi_raster[multi_index1]>>3)) clip=TRUE;

			// tile limitations
			if ((sp_y >= -expand) && ((sp_y < 0) || ((sp_y & 0x01ff) < 0x00eb)) && !clip)
			{
				short Row = (sp_y >> 3) & 0x1f;

				if(!sprite_size)
				{
					// 1x1 tile
					if (RowCount[Row] < MaxTilesPerRow)
						RowCount[Row]++;
					else
						clip=TRUE;
				}
				else
				{
					// 2x2 tile
					if ((RowCount[Row+0]+1 < MaxTilesPerRow) &&
						  (RowCount[Row+1]+1 < MaxTilesPerRow))
					{
						RowCount[Row+0]+=2;
						RowCount[Row+1]+=2;
					}
					else
						clip=TRUE;
				}
			}
	
#ifdef PRINT
			printf("(line %d) %04X, %04X, %04X / %04X %04X\n",line,
				(uint16)sp_x,(uint16)sp_y,(uint16)far_plane,(uint16)multi_farplane[multi_index1],(uint16)multi_raster[multi_index1]);
#endif

			// don't draw offscreen coordinates
			DSP4.out_count = 0;
			if(!clip)
			{
				int16 out_index = 0;
				int16 offset = DSP4_READ_WORD(0);

				// update sprite nametable/attribute information
				sp_oam = sprite_offset + offset;
				sp_msb = (sp_x<0 || sp_x>255);

#ifdef PRINT2
			printf("(line %d) %04X, %04X, %04X, %04X, %04X\n",line,
				(uint16)sp_oam,(uint16)sprite_offset,(uint16)offset,
				(uint16)sp_x,(uint16)sp_y);
#endif

				// emit transparency information
				if(
						(sprite_offset&0x08) &&
						((sprite_type==1 && sp_y>=0xcc) ||
						 (sprite_type==2 && sp_y>=0xbb))
					)
				{
					DSP4.out_count = 6;

					// one block of OAM data
					DSP4_WRITE_WORD(0,1);

					// OAM: x,y,tile,no attr
					DSP4.output[2] = sp_x&0xFF;
					DSP4.output[3] = (sp_y+6)&0xFF;
					DSP4_WRITE_WORD(4,0xEE);

					out_index = 6;

					// OAM: size,msb data
					DSP4_Op06(sprite_size,sp_msb);

#if 0
					//DSP4_WRITE_WORD(0,-1);
					DSP4_WRITE_WORD(2,-1);
					DSP4_WRITE_WORD(4,-1);
#endif
				}

				// normal data
				DSP4.out_count += 8;

				// one block of OAM data
				DSP4_WRITE_WORD(out_index+0,1);

				// OAM: x,y,tile,attr
				DSP4.output[out_index+2] = sp_x&0xFF;
				DSP4.output[out_index+3] = sp_y&0xFF;
				DSP4_WRITE_WORD(out_index+4,sp_oam);

				// no following OAM data
				DSP4_WRITE_WORD(out_index+6,0);

				// OAM: size,msb data
				DSP4_Op06(sprite_size,sp_msb);

#if 0
				//DSP4_WRITE_WORD(out_index+0,-1);
				DSP4_WRITE_WORD(out_index+2,-1);
				DSP4_WRITE_WORD(out_index+4,-1);
#endif
			}
			
			// no sprite information
			if(DSP4.out_count == 0)
			{
				DSP4.out_count = 2;
				DSP4_WRITE_WORD(0,0);
			}
		} while (1);

		/////////////////////////////////////
		// special cases: plane == 0x0000

		// special vehicle case
		if(command == 0x90)
		{
			sprite_type = 1;

			// shift bytes
			DSP4.parameters[2] = DSP4.parameters[0];
			DSP4.parameters[3] = DSP4.parameters[1];
			DSP4.parameters[0] = 0;
			DSP4.parameters[1] = 0;

			goto sprite_found;
		}
		// special terrain case
		else if(command != 0x00 && command != 0xff)
		{
			sprite_type = 2;

			// shift bytes
			DSP4.parameters[2] = DSP4.parameters[0];
			DSP4.parameters[3] = DSP4.parameters[1];
			DSP4.parameters[0] = 0;
			DSP4.parameters[1] = 0;

			goto sprite_found;
		}
	} while (1);

terminate:
	DSP4.waiting4command = TRUE;
	DSP4.out_count=0;
}

#undef PRINT
