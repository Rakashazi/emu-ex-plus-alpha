// This is a direct rewrite of gfx_cd.asm (x86 asm to C).
// You can even find some x86 register names :)
// Original code (c) 2002 by Stéphane Dallongeville

// (c) Copyright 2007, Grazvydas "notaz" Ignotas


#include "scd.h"
#include <imagine/logger/logger.h>
#include <string.h>

static const int Table_Rot_Time[] =
{
	0x00054000, 0x00048000, 0x00040000, 0x00036000,          //; 008-032               ; briefing - sprite
	0x0002E000, 0x00028000, 0x00024000, 0x00022000,          //; 036-064               ; arbre souvent
	0x00021000, 0x00020000, 0x0001E000, 0x0001B800,          //; 068-096               ; map thunderstrike
	0x00019800, 0x00017A00, 0x00015C00, 0x00013E00,          //; 100-128               ; logo défoncé

	0x00012000, 0x00011800, 0x00011000, 0x00010800,          //; 132-160               ; briefing - map
	0x00010000, 0x0000F800, 0x0000F000, 0x0000E800,          //; 164-192
	0x0000E000, 0x0000D800, 0x0000D000, 0x0000C800,          //; 196-224
	0x0000C000, 0x0000B800, 0x0000B000, 0x0000A800,          //; 228-256               ; batman visage

	0x0000A000, 0x00009F00, 0x00009E00, 0x00009D00,          //; 260-288
	0x00009C00, 0x00009B00, 0x00009A00, 0x00009900,          //; 292-320
	0x00009800, 0x00009700, 0x00009600, 0x00009500,          //; 324-352
	0x00009400, 0x00009300, 0x00009200, 0x00009100,          //; 356-384

	0x00009000, 0x00008F00, 0x00008E00, 0x00008D00,          //; 388-416
	0x00008C00, 0x00008B00, 0x00008A00, 0x00008900,          //; 420-448
	0x00008800, 0x00008700, 0x00008600, 0x00008500,          //; 452-476
	0x00008400, 0x00008300, 0x00008200, 0x00008100,          //; 480-512
};


static void gfx_cd_start(Rot_Comp &rot_comp)
{
	int upd_len;

	// rot_comp.XD_Mul = ((rot_comp.imgBuffVCallSize & 0x1f) + 1) * 4; // unused
	rot_comp.Function = (rot_comp.stampDataSize & 7) | (sCD.gate[3] & 0x18);	// Jmp_Adr
	// rot_comp.Buffer_Adr = (rot_comp.imgBuffStartAddr & 0xfff8) << 2; // unused?
	rot_comp.YD = (rot_comp.imgBuffOffset >> 3) & 7;
	rot_comp.Vector_Adr = (rot_comp.tvba & 0xfffe) << 2;

	upd_len = (rot_comp.imgBuffHDotSize >> 3) & 0x3f;
	upd_len = Table_Rot_Time[upd_len];
	rot_comp.Draw_Speed = rot_comp.Float_Part = upd_len;

	rot_comp.stampDataSize |= 0x8000;	// Stamp_Size,  we start a new GFX operation

	switch (rot_comp.stampDataSize & 6)	// Scr_16?
	{
		case 0:	// ?
			rot_comp.Stamp_Map_Adr = (rot_comp.stampMapBaseAddr & 0xff80) << 2;
			break;
		case 2: // .Dot_32
			rot_comp.Stamp_Map_Adr = (rot_comp.stampMapBaseAddr & 0xffe0) << 2;
			break;
		case 4: // .Scr_16
			rot_comp.Stamp_Map_Adr = 0x20000;
			break;
		case 6: // .Scr_16_Dot_32
			rot_comp.Stamp_Map_Adr = (rot_comp.stampMapBaseAddr & 0xe000) << 2;
			break;
	}

	//logMsg("gfx_cd_start, stamp_map_addr=%06x", rot_comp.Stamp_Map_Adr);

	gfx_cd_update(rot_comp);
}


static void gfx_completed(Rot_Comp &rot_comp)
{
	rot_comp.stampDataSize &= 0x7fff;	// Stamp_Size
	rot_comp.imgBuffVDotSize  = 0;
	if (sCD.gate[0x33] & (1<<1))
	{
		//logMsg("gfx_cd irq 1");
		scd_interruptSubCpu(1);
	}
}


static void gfx_do(Rot_Comp &rot_comp, unsigned int func, unsigned short *stamp_base, unsigned int H_Dot)
{
	//logMsg("func 0x%X", func);
	unsigned int eax, ebx, ecx, edx, esi, edi, pixel;
	unsigned int XD, Buffer_Adr;
	int DYXS;

	XD = rot_comp.imgBuffOffset & 7;
	Buffer_Adr = ((rot_comp.imgBuffStartAddr & 0xfff8) + rot_comp.YD) << 2;
	//if(rot_comp.imgBuffVDotSize == 40)
		//logMsg("gfx buff 0x%X", Buffer_Adr);
	ecx = *(uint32a*)(sCD.word.ram2M + rot_comp.Vector_Adr);
	//logMsg("H dot %d", H_Dot);
	edx = ecx >> 16;
	ecx = (ecx & 0xffff) << 8;
	edx <<= 8;
	DYXS = *(int32a*)(sCD.word.ram2M + rot_comp.Vector_Adr + 4);
	//logMsg("DYXS 0x%X", DYXS);
	rot_comp.Vector_Adr += 8;

	// MAKE_IMAGE_LINE
	while (H_Dot)
	{
		// MAKE_IMAGE_PIXEL
		if (!(func & 1))	// NOT TILED
		{
			//logMsg("not tiled");
			int mask = (func & 4) ? 0x00800000 : 0x00f80000;
			if ((ecx | edx) & mask)
			{
				//logMsg("dxs 0x%X dys 0x%X", ecx, edx);
				if (func & 0x18) goto Next_Pixel;
				pixel = 0;
				goto Pixel_Out;
			}
		}

		if (func & 2)		// mode 32x32 dot
		{
			if (func & 4)	// 16x16 screen
			{
				ebx = ((ecx >> (11+5)) & 0x007f) |
				      ((edx >> (11-2)) & 0x3f80);
			}
			else		// 1x1 screen
			{
				ebx = ((ecx >> (11+5)) & 0x07) |
				      ((edx >> (11+2)) & 0x38);
			}
		}
		else			// mode 16x16 dot
		{
			if (func & 4)	// 16x16 screen
			{
				//logMsg("16x16 screen");
				ebx = ((ecx >> (11+4)) & 0x00ff) |
				      ((edx >> (11-4)) & 0xff00);
			}
			else		// 1x1 screen
			{
				ebx = ((ecx >> (11+4)) & 0x0f) |
				      ((edx >> (11+0)) & 0xf0);
			}
		}

		edi = stamp_base[ebx];
		//logMsg("stamp base 0x%X", edi);
		esi = (edi & 0x7ff) << 7;
		if (!esi) { pixel = 0; goto Pixel_Out; }
		edi >>= (11+1);
		edi &= (0x1c>>1);
		eax = ecx;
		ebx = edx;
		if (func & 2) edi |= 1;	// 32 dots?
		switch (edi)
		{
			case 0x00:	// No_Flip_0, 16x16 dots
				ebx = (ebx >> 9) & 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 8) & 0x40) + ebx;
				break;
			case 0x01:	// No_Flip_0, 32x32 dots
				ebx = (ebx >> 9) & 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 7) & 0x180) + ebx;
				break;
			case 0x02:	// No_Flip_90, 16x16 dots
				eax = (eax >> 9) & 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap
				eax += ((ebx >> 8) & 0x40) ^ 0x40;
				break;
			case 0x03:	// No_Flip_90, 32x32 dots
				eax = (eax >> 9) & 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap
				eax += ((ebx >> 7) & 0x180) ^ 0x180;
				break;
			case 0x04:	// No_Flip_180, 16x16 dots
				ebx = ((ebx >> 9) & 0x3c) ^ 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap and flip
				eax = (((eax >> 8) & 0x40) ^ 0x40) + ebx;
				break;
			case 0x05:	// No_Flip_180, 32x32 dots
				ebx = ((ebx >> 9) & 0x7c) ^ 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap and flip
				eax = (((eax >> 7) & 0x180) ^ 0x180) + ebx;
				break;
			case 0x06:	// No_Flip_270, 16x16 dots
				eax = ((eax >> 9) & 0x3c) ^ 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap
				eax += (ebx >> 8) & 0x40;
				break;
			case 0x07:	// No_Flip_270, 32x32 dots
				eax = ((eax >> 9) & 0x7c) ^ 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap
				eax += (ebx >> 7) & 0x180;
				break;
			case 0x08:	// Flip_0, 16x16 dots
				ebx = (ebx >> 9) & 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap, flip
				eax = (((eax >> 8) & 0x40) ^ 0x40) + ebx;
				break;
			case 0x09:	// Flip_0, 32x32 dots
				ebx = (ebx >> 9) & 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap, flip
				eax = (((eax >> 7) & 0x180) ^ 0x180) + ebx;
				break;
			case 0x0a:	// Flip_90, 16x16 dots
				eax = ((eax >> 9) & 0x3c) ^ 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap, flip
				eax += ((ebx >> 8) & 0x40) ^ 0x40;
				break;
			case 0x0b:	// Flip_90, 32x32 dots
				eax = ((eax >> 9) & 0x7c) ^ 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap, flip
				eax += ((ebx >> 7) & 0x180) ^ 0x180;
				break;
			case 0x0c:	// Flip_180, 16x16 dots
				ebx = ((ebx >> 9) & 0x3c) ^ 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 8) & 0x40) + ebx;
				break;
			case 0x0d:	// Flip_180, 32x32 dots
				ebx = ((ebx >> 9) & 0x7c) ^ 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 7) & 0x180) + ebx;
				break;
			case 0x0e:	// Flip_270, 16x16 dots
				eax = (eax >> 9) & 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap, flip
				eax += (ebx >> 8) & 0x40;
				break;
			case 0x0f:	// Flip_270, 32x32 dots
				eax = (eax >> 9) & 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap, flip
				eax += (ebx >> 7) & 0x180;
				break;
		}

		pixel = *(sCD.word.ram2M + (edi >> 12) + eax);
		if (!(edi & 0x800)) pixel >>= 4;
		else pixel &= 0x0f;

Pixel_Out:
		if (!pixel && (func & 0x18)) goto Next_Pixel;
		esi = Buffer_Adr + ((XD>>1)^1);				// pixel addr
		eax = *(sCD.word.ram2M + esi);			// old pixel
		if (XD & 1)
		{
			if ((eax & 0x0f) && (func & 0x18) == 0x08) goto Next_Pixel; // underwrite
			*(sCD.word.ram2M + esi) = pixel | (eax & 0xf0);
		}
		else
		{
			if ((eax & 0xf0) && (func & 0x18) == 0x08) goto Next_Pixel; // underwrite
			*(sCD.word.ram2M + esi) = (pixel << 4) | (eax & 0xf);
		}

Next_Pixel:
		ecx += (DYXS << 16) >> 16;	// rot_comp.DXS;
		edx +=  DYXS >> 16;		// rot_comp.DYS;
		XD++;
		if (XD >= 8)
		{
			Buffer_Adr += ((rot_comp.imgBuffVCallSize & 0x1f) + 1) << 5;
			XD = 0;
		}
		H_Dot--;
	}
	// end while

// nothing_to_draw:
	rot_comp.YD++;
	// rot_comp.V_Dot--; // will be done by caller
}


void gfx_cd_update(Rot_Comp &rot_comp)
{
	int V_Dot = rot_comp.imgBuffVDotSize & 0xff;
	int jobs;

	//logMsg("gfx_cd_update, imgBuffVDotSize = %04x", rot_comp.imgBuffVDotSize);

	if (!V_Dot)
	{
		gfx_completed(rot_comp);
		return;
	}

	jobs = rot_comp.Float_Part >> 16;

	if (!jobs)
	{
		rot_comp.Float_Part += rot_comp.Draw_Speed;
		return;
	}

	rot_comp.Float_Part &= 0xffff;
	rot_comp.Float_Part += rot_comp.Draw_Speed;

	const bool gfxSupported = 1;
	if (gfxSupported)
	{
		unsigned int func = rot_comp.Function;
		unsigned int H_Dot = rot_comp.imgBuffHDotSize & 0x1ff;
		unsigned short *stamp_base = (unsigned short *) (sCD.word.ram2M + rot_comp.Stamp_Map_Adr);

		//logMsg("%d gfx jobs", jobs);
		while (jobs--)
		{
			gfx_do(rot_comp, func, stamp_base, H_Dot);	// jmp [Jmp_Adr]:

			V_Dot--;				// dec byte [V_Dot]
			if (V_Dot == 0)
			{
				// GFX_Completed:
				gfx_completed(rot_comp);
				return;
			}
		}
	}
	else
	{
		if (jobs >= V_Dot)
		{
			gfx_completed(rot_comp);
			return;
		}
		V_Dot -= jobs;
	}

	rot_comp.imgBuffVDotSize = V_Dot;
}


unsigned int gfx_cd_read(Rot_Comp &rot_comp, unsigned int a)
{
	unsigned int d = 0;

	switch (a) {
		case 0x58: d = rot_comp.stampDataSize; break;
		case 0x5A: d = rot_comp.stampMapBaseAddr; break;
		case 0x5C: d = rot_comp.imgBuffVCallSize; break;
		case 0x5E: d = rot_comp.imgBuffStartAddr; break;
		case 0x60: d = rot_comp.imgBuffOffset; break;
		case 0x62: d = rot_comp.imgBuffHDotSize; break;
		case 0x64: d = rot_comp.imgBuffVDotSize; break;
		case 0x66: break;
		default: logMsg("gfx_cd_read FIXME: unexpected address: %02x", a); break;
	}

	//logMsg("gfx_cd_read(%02x) = %04x", a, d);

	return d;
}

void gfx_cd_write16(Rot_Comp &rot_comp, unsigned int a, unsigned int d)
{
	//logMsg("gfx_cd_write16(%x, %04x)", a, d);

	switch (a) {
		case 0x58: // .Reg_Stamp_Size
			rot_comp.stampDataSize = d & 7;
			return;

		case 0x5A: // .Reg_Stamp_Adr
			rot_comp.stampMapBaseAddr = d & 0xffe0;
			return;

		case 0x5C: // .Reg_IM_VCell_Size
			rot_comp.imgBuffVCallSize = d & 0x1f;
			//logMsg("set VCell size %d", rot_comp.imgBuffVCallSize);
			return;

		case 0x5E: // .Reg_IM_Adr
			rot_comp.imgBuffStartAddr = d & 0xFFF8;
			return;

		case 0x60: // .Reg_IM_Offset
			rot_comp.imgBuffOffset = d & 0x3f;
			return;

		case 0x62: // .Reg_IM_HDot_Size
			rot_comp.imgBuffHDotSize = d & 0x1ff;
			//logMsg("set HDot size %d", rot_comp.imgBuffHDotSize);
			return;

		case 0x64: // .Reg_IM_VDot_Size
			rot_comp.imgBuffVDotSize = d & 0xff;	// V_Dot, must be 32bit?
			//logMsg("set VDot size %d", rot_comp.imgBuffVDotSize);
			return;

		case 0x66: // .Reg_Vector_Adr
			rot_comp.tvba = d & 0xfffe;
			if (sCD.gate[3]&4) return; // can't do tanformations in 1M mode
			gfx_cd_start(rot_comp);
			return;

		default: logMsg("gfx_cd_write16 FIXME: unexpected address: %02x", a); return;
	}
}


void gfx_cd_reset(Rot_Comp &rot_comp)
{
	memset(&rot_comp, 0, sizeof(rot_comp));
}
