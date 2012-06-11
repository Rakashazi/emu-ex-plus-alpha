#pragma once

struct Rot_Comp
{
	constexpr Rot_Comp() { }

	// base registers
	unsigned int stampDataSize = 0;		// Stamp_Size
	unsigned int stampMapBaseAddr = 0;
	unsigned int imgBuffVCallSize = 0;
	unsigned int imgBuffStartAddr = 0;
	unsigned int imgBuffOffset = 0;
	unsigned int imgBuffHDotSize = 0;
	unsigned int imgBuffVDotSize = 0;		// V_Dot
	unsigned int tvba = 0; // Trace vector base address

	unsigned int Stamp_Map_Adr = 0;
	unsigned int Vector_Adr = 0;
	unsigned int Function = 0;		// Jmp_Adr;
	unsigned int Float_Part = 0;
	unsigned int Draw_Speed = 0;
	unsigned int YD = 0;
};


void gfx_cd_update(Rot_Comp &rot_comp);

unsigned int gfx_cd_read(Rot_Comp &rot_comp, unsigned int a);
void gfx_cd_write16(Rot_Comp &rot_comp, unsigned int a, unsigned int d);

void gfx_cd_reset(Rot_Comp &rot_comp);

void DmaSlowCell(unsigned int source, unsigned int a, int len, unsigned char inc);
