//
//   Copyright (C) 2007-2010 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "memptrs.h"

#include <algorithm>

using namespace gambatte;

namespace {

template <OamDmaSrc src, bool cgb> struct OamDmaConflictMap;
template <bool cgb> struct OamDmaConflictMap<oam_dma_src_rom, cgb> { enum { r = 0xFCFF }; };
template <bool cgb> struct OamDmaConflictMap<oam_dma_src_sram, cgb> { enum { r = 0xFCFF }; };
template <bool cgb> struct OamDmaConflictMap<oam_dma_src_vram, cgb> { enum { r = 0x0300 }; };
template <bool cgb> struct OamDmaConflictMap<oam_dma_src_wram, cgb> { enum { r = cgb ? 0xF000 : 0xFCFF }; };
template <bool cgb> struct OamDmaConflictMap<oam_dma_src_invalid, cgb> { enum { r = cgb ? 0xFCFF : 0x0000 }; };

template <bool cgb>
bool isInOamDmaConflictArea(OamDmaSrc src, unsigned p)
{
	static unsigned short const m[] = {
		OamDmaConflictMap<oam_dma_src_rom, cgb>::r,
		OamDmaConflictMap<oam_dma_src_sram, cgb>::r,
		OamDmaConflictMap<oam_dma_src_vram, cgb>::r,
		OamDmaConflictMap<oam_dma_src_wram, cgb>::r,
		OamDmaConflictMap<oam_dma_src_invalid, cgb>::r,
		0 };
	return p < mm_oam_begin && (m[src] >> (p >> 12) & 1);
}

template <OamDmaSrc src, bool cgb>
void disconnectOamDmaAreas(unsigned char const *(&rmem)[0x10], unsigned char *(&wmem)[0x10])
{
	if (OamDmaConflictMap<src, cgb>::r & 0x00FF)
		std::fill_n(rmem, 8, static_cast<unsigned char *>(0));
	if (OamDmaConflictMap<src, cgb>::r & 0x0C00)
		rmem[0xB] = rmem[0xA] = wmem[0xB] = wmem[0xA] = 0;
	if (OamDmaConflictMap<src, cgb>::r & 0x7000)
		rmem[0xE] = rmem[0xD] = rmem[0xC] = wmem[0xE] = wmem[0xD] = wmem[0xC] = 0;
}

template <bool cgb>
void disconnectOamDmaAreas(unsigned char const *(&rmem)[0x10], unsigned char *(&wmem)[0x10],
		OamDmaSrc src)
{
	switch (src) {
	case oam_dma_src_rom: disconnectOamDmaAreas<oam_dma_src_rom, cgb>(rmem, wmem); break;
	case oam_dma_src_sram: disconnectOamDmaAreas<oam_dma_src_sram, cgb>(rmem, wmem); break;
	case oam_dma_src_vram: disconnectOamDmaAreas<oam_dma_src_vram, cgb>(rmem, wmem); break;
	case oam_dma_src_wram: disconnectOamDmaAreas<oam_dma_src_wram, cgb>(rmem, wmem); break;
	case oam_dma_src_invalid: disconnectOamDmaAreas<oam_dma_src_invalid, cgb>(rmem, wmem); break;
	case oam_dma_src_off: break;
	}
}

} // unnamed namespace.

MemPtrs::MemPtrs()
: rmem_()
, wmem_()
, romdata_()
, wramdata_()
, vrambankptr_(0)
, rsrambankptr_(0)
, wsrambankptr_(0)
, rambankdata_(0)
, wramdataend_(0)
, oamDmaSrc_(oam_dma_src_off)
{
}

void MemPtrs::reset(unsigned const rombanks, unsigned const rambanks, unsigned const wrambanks) {
	int const num_disabled_ram_areas = 2;
	memchunk_.reset(
		  pre_rom_pad_size()
		+ rombanks * rombank_size()
		+ max_num_vrambanks * vrambank_size()
		+ rambanks * rambank_size()
		+ wrambanks * wrambank_size()
		+ num_disabled_ram_areas * rambank_size());

	romdata_[0] = romdata();
	rambankdata_ = romdata_[0] + rombanks * rombank_size() + max_num_vrambanks * vrambank_size();
	wramdata_[0] = rambankdata_ + rambanks * rambank_size();
	wramdataend_ = wramdata_[0] + wrambanks * wrambank_size();

	std::fill_n(rdisabledRamw(), rambank_size(), 0xFF);

	oamDmaSrc_ = oam_dma_src_off;
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	rmem_[0xC] = wmem_[0xC] = wramdata_[0] - mm_wram_begin;
	rmem_[0xE] = wmem_[0xE] = wramdata_[0] - mm_wram_mirror_begin;
	setRombank(1);
	setRambank(0, 0);
	setVrambank(0);
	setWrambank(1);
}

void MemPtrs::setRombank0(unsigned bank) {
	romdata_[0] = romdata() + bank * rombank_size();
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	disconnectOamDmaAreas();
}

void MemPtrs::setRombank(unsigned bank) {
	romdata_[1] = romdata() + bank * rombank_size() - mm_rom1_begin;
	rmem_[0x7] = rmem_[0x6] = rmem_[0x5] = rmem_[0x4] = romdata_[1];
	disconnectOamDmaAreas();
}

void MemPtrs::setRambank(unsigned const flags, unsigned const rambank) {
	unsigned char *srambankptr = 0;
	if (!(flags & rtc_en)) {
		srambankptr = rambankdata() != rambankdataend()
			? rambankdata_ + rambank * rambank_size()
			: wdisabledRam();
	}

	rsrambankptr_ = (flags & read_en) && srambankptr != wdisabledRam()
		? srambankptr - mm_sram_begin
		: rdisabledRamw() - mm_sram_begin;
	wsrambankptr_ = flags & write_en
		? srambankptr - mm_sram_begin
		: wdisabledRam() - mm_sram_begin;
	rmem_[0xB] = rmem_[0xA] = rsrambankptr_;
	wmem_[0xB] = wmem_[0xA] = wsrambankptr_;
	disconnectOamDmaAreas();
}

void MemPtrs::setWrambank(unsigned bank) {
	wramdata_[1] = wramdata_[0] + (bank & 0x07 ? bank & 0x07 : 1) * wrambank_size();
	rmem_[0xD] = wmem_[0xD] = wramdata_[1] - mm_wram1_begin;
	disconnectOamDmaAreas();
}

void MemPtrs::setOamDmaSrc(OamDmaSrc oamDmaSrc) {
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	rmem_[0x7] = rmem_[0x6] = rmem_[0x5] = rmem_[0x4] = romdata_[1];
	rmem_[0xB] = rmem_[0xA] = rsrambankptr_;
	wmem_[0xB] = wmem_[0xA] = wsrambankptr_;
	rmem_[0xC] = wmem_[0xC] = wramdata_[0] - mm_wram_begin;
	rmem_[0xD] = wmem_[0xD] = wramdata_[1] - mm_wram1_begin;
	rmem_[0xE] = wmem_[0xE] = wramdata_[0] - mm_wram_mirror_begin;

	oamDmaSrc_ = oamDmaSrc;
	disconnectOamDmaAreas();
}

void MemPtrs::disconnectOamDmaAreas() {
	return isCgb(*this)
	? ::disconnectOamDmaAreas<true>(rmem_, wmem_, oamDmaSrc_)
	: ::disconnectOamDmaAreas<false>(rmem_, wmem_, oamDmaSrc_);
}

bool MemPtrs::isInOamDmaConflictArea(unsigned p) const
{
	return isCgb(*this)
	? ::isInOamDmaConflictArea<true>(oamDmaSrc_, p)
	: ::isInOamDmaConflictArea<false>(oamDmaSrc_, p);
}
