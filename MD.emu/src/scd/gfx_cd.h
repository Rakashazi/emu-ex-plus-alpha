#pragma once

struct Rot_Comp
{
	constexpr Rot_Comp(): stampDataSize(0),
	stampMapBaseAddr(0),
	imgBuffVCallSize(0),
	imgBuffStartAddr(0),
	imgBuffOffset(0),
	imgBuffHDotSize(0),
	imgBuffVDotSize(0),
	tvba(0),
	Stamp_Map_Adr(0),
	Vector_Adr(0),
	Function(0),
	Float_Part(0),
	Draw_Speed(0),
	YD(0)
	{ }

	// base registers
	unsigned int stampDataSize;		// Stamp_Size
	unsigned int stampMapBaseAddr;
	unsigned int imgBuffVCallSize;
	unsigned int imgBuffStartAddr;
	unsigned int imgBuffOffset;
	unsigned int imgBuffHDotSize;
	unsigned int imgBuffVDotSize;		// V_Dot
	unsigned int tvba; // Trace vector base address

	unsigned int Stamp_Map_Adr;
	unsigned int Vector_Adr;
	unsigned int Function;		// Jmp_Adr;
	unsigned int Float_Part;
	unsigned int Draw_Speed;
	unsigned int YD;
};


void gfx_cd_update(Rot_Comp &rot_comp);

unsigned int gfx_cd_read(Rot_Comp &rot_comp, unsigned int a);
void gfx_cd_write16(Rot_Comp &rot_comp, unsigned int a, unsigned int d);

void gfx_cd_reset(Rot_Comp &rot_comp);

void DmaSlowCell(unsigned int source, unsigned int a, int len, unsigned char inc);
