/* our copyright! */

#include "chunk.h"
#include "sound.h"
#include "interrupt.h"
#include "dma.h"
#include "flash.h"
#include "mem.h"
#include "TLCS900h_registers.h"
#include "Z80_interface.h"

#define HEADER "\0\x60\0\0NGPS"
#define HEADER_SIZE	8

#define SIZE_CHUNK	8
#define SIZE_EOD	0
#define SIZE_RAM	0xC000
#define SIZE_REGS	418
#define SIZE_ROM	(rom.length)
#define SIZE_ROMH	64
#define SIZE_TIME	4

static uint8 read1(const uint8 *);
static uint16 read2(const uint8 *);
static uint32 read4(const uint8 *);
static uint8 *read_chunk_data(FILE *, uint32);
static void read_soundchip(SoundChip *, const uint8 **);
static void read_REGS(const uint8 *);

static void write1(uint8 *, uint8);
static void write2(uint8 *, uint16);
static void write4(uint8 *, uint32);
static bool write_chunk(FILE *, uint32, const uint8 *, uint32);
static void write_soundchip(const SoundChip *, uint8 **);
static bool write_FLSH(FILE *, const uint8 *, uint32);
static bool write_RAM(FILE *);
static bool write_REGS(FILE *);
static bool write_ROM(FILE *);
static bool write_ROMH(FILE *);
static bool write_TIME(FILE *);


bool read_chunk(FILE *fp, uint32 *tagp, uint32 *sizep)
{
	uint8 buf[SIZE_CHUNK];
	
	if (fread(buf, 1, SIZE_CHUNK, fp) != SIZE_CHUNK)
		return FALSE;
	
	*tagp = read4(buf);
	*sizep = read4(buf+4);
	
	return TRUE;
}

bool read_header(FILE *fp)
{
	uint8 buf[HEADER_SIZE];

	if (fread(buf, 1, HEADER_SIZE, fp) != HEADER_SIZE)
		return FALSE;

	if (memcmp(buf, HEADER, HEADER_SIZE) != 0)
		return FALSE;

	return TRUE;
}

bool read_SNAP(FILE *fp, uint32 size)
{
	uint8 *data, *end, *p;
	#define new new_SNAP
	int got, new, subsize;
	
	if ((data=read_chunk_data(fp, size)) == NULL)
		return FALSE;
	
	got = 0;
	end = data+size;
	for (p=data; p<end; p += subsize+SIZE_CHUNK) {
		subsize = read4(p+4);
		switch (read4(p)) {
		case TAG_FLSH:
			new = OPT_FLSH;
			break;
		case TAG_RAM:
			if (subsize != SIZE_RAM)
				new = -1;
			else
				new = OPT_RAM;
			break;
		case TAG_REGS:
			if (subsize != SIZE_REGS)
				new = -1;
			else
				new = OPT_REGS;
			break;
		case TAG_ROM:
			new = OPT_ROM;
			break;
		case TAG_ROMH:
			if (subsize != SIZE_ROMH)
				new = -1;
			else
				new = OPT_ROMH;
			if (memcmp(rom_header, p+SIZE_CHUNK,
				   sizeof(RomHeader)) != 0) {
				system_message(system_get_string(IDS_WRONGROM));
				free(data);
				return FALSE;
			}
			break;
		case TAG_TIME:
			if (subsize != SIZE_TIME)
				new = -1;
			else
				new = OPT_TIME;
			break;
		default:
			new = 0;
		}
		
		if (new == -1 || (got & new)) {
			/* illegal chunk or duplicate chunk */
			free(data);
			return FALSE;
		}
		got |= new;
	}
	
	if (p != end) {
		/* chunk overruns SNAP chunk */
		free(data);
		return FALSE;
	}
	
	if (((got & (OPT_REGS|OPT_RAM)) != (OPT_REGS|OPT_RAM))
	    || (got & (OPT_ROM|OPT_ROMH)) == (OPT_ROM|OPT_ROMH)) {
		/* missing chunks or ROM and ROMH */
		free(data);
		return FALSE;
	}
	
	
	/* apply state */
	
	reset();
	
	for (p=data; p<end; p += subsize) {
		subsize = read4(p+4);
		p += SIZE_CHUNK;
		switch (read4(p-SIZE_CHUNK)) {
		case TAG_FLSH:
			/* XXX: load flash */
			break;
		case TAG_RAM:
			memcpy(ram, p, SIZE_RAM);
			break;
		case TAG_REGS:
			read_REGS(p);
			break;
		case TAG_ROM:
			/* XXX: load rom */
			break;
		case TAG_ROMH:
			/* handled in verify loop */
			break;
		case TAG_TIME:
			frame_count = read4(p);
			break;
		}
	}
	
	#undef new
	free(data);
	system_sound_chipreset(); // reset sound chip again or sample_chip_noise() can hang
	return TRUE;
}


bool write_header(FILE *fp)
{
	if (fwrite(HEADER, 1, HEADER_SIZE, fp) != HEADER_SIZE)
		return FALSE;

	return TRUE;
}

bool write_EOD(FILE *fp)
{
	return write_chunk(fp, TAG_EOD, NULL, SIZE_EOD);
}

bool write_SNAP(FILE *fp, int options)
{
	uint32 size;
	int flash_size;
	uint8 *flash;
	bool ret;

	if (options & OPT_ROMH && options & OPT_ROM)
		return FALSE;

	flash = NULL;
	flash_size = 0;
	if (options & OPT_FLSH)
		flash = flash_prepare(&flash_size);
	
	size = SIZE_RAM + SIZE_REGS + SIZE_CHUNK*2;
	if (options & OPT_TIME)
		size += SIZE_TIME + SIZE_CHUNK;
	if (options & OPT_ROM)
		size += SIZE_ROM + SIZE_CHUNK;
	if (options & OPT_ROMH)
		size += SIZE_ROMH + SIZE_CHUNK;
	if (options & OPT_FLSH)
		size += flash_size + SIZE_CHUNK;

	ret = write_chunk(fp, TAG_SNAP, NULL, size);

	if (options & OPT_TIME)
		ret &= write_TIME(fp);
	if (options & OPT_ROM)
		ret &= write_ROM(fp);
	if (options & OPT_ROMH)
		ret &= write_ROMH(fp);
	if (options & OPT_FLSH) {
		ret &= write_FLSH(fp, flash, flash_size);
		free(flash);
	}

	ret &= write_RAM(fp);
	ret &= write_REGS(fp);

	return ret;
}


static uint8 read1(const uint8 *d)
{
	return d[0];
}

static uint16 read2(const uint8 *d)
{
	return (d[0]<<8)|d[1];
}

static uint32 read4(const uint8 *d)
{
	return (d[0]<<24)|(d[1]<<16)|(d[2]<<8)|d[3];
}

static uint8 *read_chunk_data(FILE *fp, uint32 size)
{
	uint8 *data;

	if ((data=(uint8*)malloc(size)) == NULL)
		return NULL;

	if (fread(data, 1, size, fp) != size) {
		free(data);
		return NULL;
	}

	return data;
}

static void read_soundchip(SoundChip *chip, const uint8 **pp)
{
	const uint8 *p;
	int i;

	p = *pp;
	
	chip->LastRegister = read4(p), p+=4;
	for (i=0; i<8; i++)
		chip->Register[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		chip->Volume[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		chip->Period[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		chip->Count[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		chip->Output[i] = read4(p), p+=4;
	chip->RNG = read4(p), p+=4;
	chip->NoiseFB = read4(p), p+=4;

	*pp = p;
}

static void read_REGS(const uint8 *p)
{
	int i, j;
	
	pc = read4(p), p+=4;
	sr = read2(p), p+=2;
	for (i=0; i<4; i++)	
		for (j=0; j<4; j++)
			gprBank[i][j] = read4(p), p+=4;
	for (i=0; i<4; i++)
		gpr[i] = read4(p), p+=4;
	f_dash = read1(p), p+=1;
	eepromStatusEnable = read1(p), p+=1;
	Z80_regs.AF.W = read2(p), p+=2;
	Z80_regs.BC.W = read2(p), p+=2;
	Z80_regs.DE.W = read2(p), p+=2;
	Z80_regs.HL.W = read2(p), p+=2;
	Z80_regs.IX.W = read2(p), p+=2;
	Z80_regs.IY.W = read2(p), p+=2;
	Z80_regs.PC.W = read2(p), p+=2;
	Z80_regs.SP.W = read2(p), p+=2;
	Z80_regs.AF1.W = read2(p), p+=2;
	Z80_regs.BC1.W = read2(p), p+=2;
	Z80_regs.DE1.W = read2(p), p+=2;
	Z80_regs.HL1.W = read2(p), p+=2;
	Z80_regs.IFF = read1(p), p+=1;
	Z80_regs.I = read1(p), p+=1;
	Z80_regs.R = read1(p), p+=1;
	Z80_regs.IPeriod = read4(p), p+=4;
	Z80_regs.ICount = read4(p), p+=4;
	Z80_regs.IBackup = read4(p), p+=4;
	Z80_regs.IRequest = read2(p), p+=2;
	Z80_regs.IAutoReset = read1(p), p+=1;
	Z80_regs.TrapBadOps = read1(p), p+=1;
	Z80_regs.Trap = read2(p), p+=2;
	Z80_regs.Trace = read1(p), p+=1;
	timer_hint = read4(p), p+=4;
	for (i=0; i<4; i++)
		timer[i] = read1(p), p+=1;
	timer_clock0 = read4(p), p+=4;
	timer_clock1 = read4(p), p+=4;
	timer_clock2 = read4(p), p+=4;
	timer_clock3 = read4(p), p+=4;
	read_soundchip(&toneChip, &p);
	read_soundchip(&noiseChip, &p);
	for (i=0; i<4; i++)
		dmaS[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		dmaD[i] = read4(p), p+=4;
	for (i=0; i<4; i++)
		dmaC[i] = read2(p), p+=2;
	for (i=0; i<4; i++)
		dmaM[i] = read1(p), p+=1;
}


static void write1(uint8 *p, uint8 val)
{
	p[0] = val;
}

static void write2(uint8 *p, uint16 val)
{
	p[0] = (val>>8) & 0xff;
	p[1] = val & 0xff;
}

static void write4(uint8 *p, uint32 val)
{
	p[0] = (val>>24) & 0xff;
	p[1] = (val>>16) & 0xff;
	p[2] = (val>>8) & 0xff;
	p[3] = val & 0xff;
}

static bool write_chunk(FILE *fp, uint32 name, const uint8 *data, uint32 size)
{
	uint8 buf[SIZE_CHUNK], *p;
	int ret;

	p = buf;
	write4(p, name), p+=4;
	write4(p, size);

	ret = fwrite(buf, 1, SIZE_CHUNK, fp) == SIZE_CHUNK;

	if (data && size > 0)
	    ret &= fwrite(data, 1, size, fp) == size;

	return ret;
}

static void write_soundchip(const SoundChip *chip, uint8 **pp)
{
	uint8 *p;
	int i;

	p = *pp;

	write4(p, chip->LastRegister), p+=4;
	for (i=0; i<8; i++)
		write4(p, chip->Register[i]), p+=4;
	for (i=0; i<4; i++)
		write4(p, chip->Volume[i]), p+=4;
	for (i=0; i<4; i++)
		write4(p, chip->Period[i]), p+=4;
	for (i=0; i<4; i++)
		write4(p, chip->Count[i]), p+=4;
	for (i=0; i<4; i++)
		write4(p, chip->Output[i]), p+=4;
	write4(p, chip->RNG), p+=4;
	write4(p, chip->NoiseFB), p+=4;
}

static bool write_FLSH(FILE *fp, const uint8 *data, uint32 size)
{
	return write_chunk(fp, TAG_FLSH, data, size);
}

static bool write_RAM(FILE *fp)
{
	return write_chunk(fp, TAG_RAM, ram, SIZE_RAM);
}

static bool write_REGS(FILE *fp)
{
	uint8 data[SIZE_REGS], *p;
	int i, j;

	p = data;

	write4(p, pc), p+=4;
	write2(p, sr), p+=2;
	for (i=0; i<4; i++)	
		for (j=0; j<4; j++)
			write4(p, gprBank[i][j]), p+=4;
	for (i=0; i<4; i++)
		write4(p, gpr[i]), p+=4;
	write1(p, f_dash), p+=1;
	write1(p, eepromStatusEnable), p+=1;
	write2(p, Z80_regs.AF.W), p+=2;
	write2(p, Z80_regs.BC.W), p+=2;
	write2(p, Z80_regs.DE.W), p+=2;
	write2(p, Z80_regs.HL.W), p+=2;
	write2(p, Z80_regs.IX.W), p+=2;
	write2(p, Z80_regs.IY.W), p+=2;
	write2(p, Z80_regs.PC.W), p+=2;
	write2(p, Z80_regs.SP.W), p+=2;
	write2(p, Z80_regs.AF1.W), p+=2;
	write2(p, Z80_regs.BC1.W), p+=2;
	write2(p, Z80_regs.DE1.W), p+=2;
	write2(p, Z80_regs.HL1.W), p+=2;
	write1(p, Z80_regs.IFF), p+=1;
	write1(p, Z80_regs.I), p+=1;
	write1(p, Z80_regs.R), p+=1;
	write4(p, Z80_regs.IPeriod), p+=4;
	write4(p, Z80_regs.ICount), p+=4;
	write4(p, Z80_regs.IBackup), p+=4;
	write2(p, Z80_regs.IRequest), p+=2;
	write1(p, Z80_regs.IAutoReset), p+=1;
	write1(p, Z80_regs.TrapBadOps), p+=1;
	write2(p, Z80_regs.Trap), p+=2;
	write1(p, Z80_regs.Trace), p+=1;
	write4(p, timer_hint), p+=4;
	for (i=0; i<4; i++)
		write1(p, timer[i]), p+=1;
	write4(p, timer_clock0), p+=4;
	write4(p, timer_clock1), p+=4;
	write4(p, timer_clock2), p+=4;
	write4(p, timer_clock3), p+=4;
	write_soundchip(&toneChip, &p);
	write_soundchip(&noiseChip, &p);
	for (i=0; i<4; i++)
		write4(p, dmaS[i]), p+=4;
	for (i=0; i<4; i++)
		write4(p, dmaD[i]), p+=4;
	for (i=0; i<4; i++)
		write2(p, dmaC[i]), p+=2;
	for (i=0; i<4; i++)
		write1(p, dmaM[i]), p+=1;

	return write_chunk(fp, TAG_REGS, data, SIZE_REGS);
}

static bool write_ROM(FILE *fp)
{
	return write_chunk(fp, TAG_ROM, rom.data, SIZE_ROM);
}

static bool write_ROMH(FILE *fp)
{
	return write_chunk(fp, TAG_ROMH, (uint8*)rom_header, SIZE_ROMH);
}

static bool write_TIME(FILE *fp)
{
	uint8 data[SIZE_TIME];

	write4(data, frame_count);
	return write_chunk(fp, TAG_TIME, data, SIZE_TIME);
}
