/***************************************************************************
 *   Copyright (C) 2007-2010 by Sindre Aamås                               *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "cartridge.h"
#include "../file/file.h"
#include "../savestate.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <logger/interface.h>

namespace gambatte {

Cartridge::Cartridge()
: rombank(1),
  rambank(0),
  enableRam(false),
  rambankMode(false),
  multi64rom(false)
{
}

enum Cartridgetype { PLAIN, MBC1, MBC2, MBC3, MBC5 };

static unsigned adjustedRombank(unsigned bank, const Cartridgetype romtype) {
	if ((romtype == MBC1 && !(bank & 0x1F)) || (romtype == MBC5 && !bank))
		++bank;
	
	return bank;
}

void Cartridge::setStatePtrs(SaveState &state) {
	state.mem.sram.set(memptrs.rambankdata(), memptrs.rambankdataend() - memptrs.rambankdata());
	state.mem.wram.set(memptrs.wramdata(0), memptrs.wramdataend() - memptrs.wramdata(0));
}

void Cartridge::saveState(SaveState &state) const {
	state.mem.rombank = rombank;
	state.mem.rambank = rambank;
	state.mem.enableRam = enableRam;
	state.mem.rambankMode = rambankMode;

	rtc.saveState(state);
}

static bool hasRtc(const unsigned headerByte0x147) {
	switch (headerByte0x147) {
	case 0x0F:
	case 0x10: return true;
	default: return false;
	}
}

static Cartridgetype cartridgeType(const unsigned headerByte0x147) {
	static const unsigned char typeLut[] = {
		/* [0xFF] = */ MBC1,
		/* [0x00] = */ PLAIN,
		/* [0x01] = */ MBC1,
		/* [0x02] = */ MBC1,
		/* [0x03] = */ MBC1,
		/* [0x04] = */ 0,
		/* [0x05] = */ MBC2,
		/* [0x06] = */ MBC2,
		/* [0x07] = */ 0,
		/* [0x08] = */ PLAIN,
		/* [0x09] = */ PLAIN,
		/* [0x0A] = */ 0,
		/* [0x0B] = */ 0,
		/* [0x0C] = */ 0,
		/* [0x0D] = */ 0,
		/* [0x0E] = */ 0,
		/* [0x0F] = */ MBC3,
		/* [0x10] = */ MBC3,
		/* [0x11] = */ MBC3,
		/* [0x12] = */ MBC3,
		/* [0x13] = */ MBC3,
		/* [0x14] = */ 0,
		/* [0x15] = */ 0,
		/* [0x16] = */ 0,
		/* [0x17] = */ 0,
		/* [0x18] = */ 0,
		/* [0x19] = */ MBC5,
		/* [0x1A] = */ MBC5,
		/* [0x1B] = */ MBC5,
		/* [0x1C] = */ MBC5,
		/* [0x1D] = */ MBC5,
		/* [0x1E] = */ MBC5
	};

	return static_cast<Cartridgetype>(typeLut[(headerByte0x147 + 1) & 0x1F]);
}

static unsigned toMulti64Rombank(const unsigned rombank) {
	return (rombank >> 1 & 0x30) | (rombank & 0xF);
}

void Cartridge::loadState(const SaveState &state) {
	rtc.loadState(state, hasRtc(memptrs.romdata()[0x147]) ? state.mem.enableRam : false);

	rombank = state.mem.rombank;
	rambank = state.mem.rambank;
	enableRam = state.mem.enableRam;
	rambankMode = state.mem.rambankMode;
	memptrs.setRambank(enableRam, rtc.getActive(), rambank & (rambanks() - 1));
	
	if (rambankMode && multi64rom) {
		const unsigned rb = toMulti64Rombank(rombank);
		memptrs.setRombank0(rb & 0x30);
		memptrs.setRombank(adjustedRombank(rb, cartridgeType(memptrs.romdata()[0x147])));
	} else {
		memptrs.setRombank0(0);
		memptrs.setRombank(adjustedRombank(rombank & (rombanks() - 1), cartridgeType(memptrs.romdata()[0x147])));
	}
}

void Cartridge::mbcWrite(const unsigned P, const unsigned data) {
	const Cartridgetype romtype = cartridgeType(memptrs.romdata()[0x147]);

	switch (P >> 12 & 0x7) {
	case 0x0:
	case 0x1: //Most MBCs write 0x?A to addresses lower than 0x2000 to enable ram.
		if (romtype == MBC2 && (P & 0x0100)) break;

		enableRam = (data & 0x0F) == 0xA;

		if (hasRtc(memptrs.romdata()[0x147]))
			rtc.setEnabled(enableRam);

		memptrs.setRambank(enableRam, rtc.getActive(), rambank & (rambanks() - 1));
		break;
		//MBC1 writes ???n nnnn to address area 0x2000-0x3FFF, ???n nnnn makes up the lower digits to determine which rombank to load.
		//MBC3 writes ?nnn nnnn to address area 0x2000-0x3FFF, ?nnn nnnn makes up the lower digits to determine which rombank to load.
		//MBC5 writes nnnn nnnn to address area 0x2000-0x2FFF, nnnn nnnn makes up the lower digits to determine which rombank to load.
		//MBC5 writes bit8 of the number that determines which rombank to load to address 0x3000-0x3FFF.
	case 0x2:
		switch (romtype) {
		case PLAIN:
			return;
		case MBC5:
			rombank = (rombank & 0x100) | data;
			memptrs.setRombank(adjustedRombank(rombank & (rombanks() - 1), romtype));
			return;
		default:
			break; //Only supposed to break one level.
		}
	case 0x3:
		switch (romtype) {
		case MBC1:
			rombank = rambankMode && !multi64rom ? data & 0x1F : (rombank & 0x60) | (data & 0x1F);
			
			if (rambankMode && multi64rom) {
				memptrs.setRombank(adjustedRombank(toMulti64Rombank(rombank), romtype));
				return;
			}
			
			break;
		case MBC2:
			if (P & 0x0100) {
				rombank = data & 0x0F;
				break;
			}

			return;
		case MBC3:
			rombank = data & 0x7F;
			break;
		case MBC5:
			rombank = (data & 0x1) << 8 | (rombank & 0xFF);
			break;
		default:
			return;
		}

		memptrs.setRombank(adjustedRombank(rombank & (rombanks() - 1), romtype));
		break;
		//MBC1 writes ???? ??nn to area 0x4000-0x5FFF either to determine rambank to load, or upper 2 bits of the rombank number to load, depending on rom-mode.
		//MBC3 writes ???? ??nn to area 0x4000-0x5FFF to determine rambank to load
		//MBC5 writes ???? nnnn to area 0x4000-0x5FFF to determine rambank to load
	case 0x4:
	case 0x5:
		switch (romtype) {
		case MBC1:
			if (rambankMode) {
				if (multi64rom) {
					rombank = (data & 0x03) << 5 | (rombank & 0x1F);
					
					const unsigned rb = toMulti64Rombank(rombank);
					memptrs.setRombank0(rb & 0x30);
					memptrs.setRombank(adjustedRombank(rb, romtype));
					return;
				}
				
				rambank = data & 0x03;
				break;
			}

			rombank = (data & 0x03) << 5 | (rombank & 0x1F);
			memptrs.setRombank(adjustedRombank(rombank & (rombanks() - 1), romtype));
			return;
		case MBC3:
			if (hasRtc(memptrs.romdata()[0x147]))
				rtc.swapActive(data);

			rambank = data & 0x03;
			break;
		case MBC5:
			rambank = data & 0x0F;
			break;
		default:
			return;
		}

		memptrs.setRambank(enableRam, rtc.getActive(), rambank & (rambanks() - 1));
		break;
		//MBC1: If ???? ???1 is written to area 0x6000-0x7FFFF rom will be set to rambank mode.
	case 0x6:
	case 0x7:
		switch (romtype) {
		case MBC1:
			rambankMode = data & 0x01;
			
			if (multi64rom) {
				if (rambankMode) {
					const unsigned rb = toMulti64Rombank(rombank);
					memptrs.setRombank0(rb & 0x30);
					memptrs.setRombank(adjustedRombank(rb, romtype));
				} else {
					memptrs.setRombank0(0);
					memptrs.setRombank(adjustedRombank(rombank & (rombanks() - 1), romtype));
				}
			}
			
			break;
		case MBC3:
			rtc.latch(data);
			break;
		default:
			break;
		}

		break;
	}
}

static const std::string stripExtension(const std::string &str) {
	const std::string::size_type lastDot = str.find_last_of('.');
	const std::string::size_type lastSlash = str.find_last_of('/');

	if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastSlash < lastDot))
		return str.substr(0, lastDot);

	return str;
}

static const std::string stripDir(const std::string &str) {
	const std::string::size_type lastSlash = str.find_last_of('/');

	if (lastSlash != std::string::npos)
		return str.substr(lastSlash + 1);

	return str;
}

const std::string Cartridge::saveBasePath() const {
	return saveDir.empty() ? defaultSaveBasePath : saveDir + stripDir(defaultSaveBasePath);
}

void Cartridge::setSaveDir(const std::string &dir) {
	saveDir = dir;

	if (!saveDir.empty() && saveDir[saveDir.length() - 1] != '/')
		saveDir += '/';
}

static void enforce8bit(unsigned char *data, unsigned long sz) {
	if (static_cast<unsigned char>(0x100))
		while (sz--)
			*data++ &= 0xFF;
}

static unsigned pow2ceil(unsigned n) {
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	++n;

	return n;
}

bool Cartridge::loadROM(const std::string &romfile, const bool forceDmg, const bool multicartCompat) {
	const std::auto_ptr<File> rom(newFileInstance(romfile));

	if (rom->fail())
		return 1;
	
	unsigned rambanks = 1;
	unsigned rombanks = 2;
	bool cgb = false;

	{
		unsigned char header[0x150];
		rom->read(reinterpret_cast<char*>(header), sizeof(header));

		switch (header[0x0147]) {
		case 0x00: logger_printf(0, "Plain ROM loaded."); break;
		case 0x01: logger_printf(0, "MBC1 ROM loaded."); break;
		case 0x02: logger_printf(0, "MBC1 ROM+RAM loaded."); break;
		case 0x03: logger_printf(0, "MBC1 ROM+RAM+BATTERY loaded."); break;
		case 0x05: logger_printf(0, "MBC2 ROM loaded."); break;
		case 0x06: logger_printf(0, "MBC2 ROM+BATTERY loaded."); break;
		case 0x08: logger_printf(0, "Plain ROM with additional RAM loaded."); break;
		case 0x09: logger_printf(0, "Plain ROM with additional RAM and Battery loaded."); break;
		case 0x0B: logger_printf(0, "MM01 ROM not supported."); return 1;
		case 0x0C: logger_printf(0, "MM01 ROM not supported."); return 1;
		case 0x0D: logger_printf(0, "MM01 ROM not supported."); return 1;
		case 0x0F: logger_printf(0, "MBC3 ROM+TIMER+BATTERY loaded."); break;
		case 0x10: logger_printf(0, "MBC3 ROM+TIMER+RAM+BATTERY loaded."); break;
		case 0x11: logger_printf(0, "MBC3 ROM loaded."); break;
		case 0x12: logger_printf(0, "MBC3 ROM+RAM loaded."); break;
		case 0x13: logger_printf(0, "MBC3 ROM+RAM+BATTERY loaded."); break;
		case 0x15: logger_printf(0, "MBC4 ROM not supported."); return 1;
		case 0x16: logger_printf(0, "MBC4 ROM not supported."); return 1;
		case 0x17: logger_printf(0, "MBC4 ROM not supported."); return 1;
		case 0x19: logger_printf(0, "MBC5 ROM loaded."); break;
		case 0x1A: logger_printf(0, "MBC5 ROM+RAM loaded."); break;
		case 0x1B: logger_printf(0, "MBC5 ROM+RAM+BATTERY loaded."); break;
		case 0x1C: logger_printf(0, "MBC5+RUMBLE ROM not supported."); break;
		case 0x1D: logger_printf(0, "MBC5+RUMBLE+RAM ROM not suported."); break;
		case 0x1E: logger_printf(0, "MBC5+RUMBLE+RAM+BATTERY ROM not supported."); break;
		case 0xFC: logger_printf(0, "Pocket Camera ROM not supported."); return 1;
		case 0xFD: logger_printf(0, "Bandai TAMA5 ROM not supported."); return 1;
		case 0xFE: logger_printf(0, "HuC3 ROM not supported."); return 1;
		case 0xFF: logger_printf(0, "HuC1 ROM+RAM+BATTERY loaded."); break;
		default: logger_printf(0, "Wrong data-format, corrupt or unsupported ROM."); return 1;
		}

		/*switch (header[0x0148]) {
		case 0x00: rombanks = 2; break;
		case 0x01: rombanks = 4; break;
		case 0x02: rombanks = 8; break;
		case 0x03: rombanks = 16; break;
		case 0x04: rombanks = 32; break;
		case 0x05: rombanks = 64; break;
		case 0x06: rombanks = 128; break;
		case 0x07: rombanks = 256; break;
		case 0x08: rombanks = 512; break;
		case 0x52: rombanks = 72; break;
		case 0x53: rombanks = 80; break;
		case 0x54: rombanks = 96; break;
		default: return 1;
		}

		std::printf("rombanks: %u\n", rombanks);*/

		switch (header[0x0149]) {
		case 0x00: /*logger_printf(0, "No RAM");*/ rambanks = cartridgeType(header[0x0147]) == MBC2; break;
		case 0x01: /*logger_printf(0, "2kB RAM");*/ /*rambankrom=1; break;*/
		case 0x02: /*logger_printf(0, "8kB RAM");*/
			rambanks = 1;
			break;
		case 0x03: /*logger_printf(0, "32kB RAM");*/
			rambanks = 4;
			break;
		case 0x04: /*logger_printf(0, "128kB RAM");*/
			rambanks = 16;
			break;
		case 0x05: /*logger_printf(0, "undocumented kB RAM");*/
			rambanks = 16;
			break;
		default: /*logger_printf(0, "Wrong data-format, corrupt or unsupported ROM loaded.");*/
			rambanks = 16;
			break;
		}
		
		defaultSaveBasePath = stripExtension(romfile);

		cgb = header[0x0143] >> 7 & (1 ^ forceDmg);
		logger_printfn(0, "cgb: %d\n", cgb);
	}

	logger_printfn(0, "rambanks: %u\n", rambanks);

	const std::size_t filesize = rom->size();
	rombanks = pow2ceil(filesize / 0x4000);
	logger_printfn(0, "rombanks: %u\n", (unsigned int)(filesize / 0x4000));
	
	ggUndoList.clear();
	memptrs.reset(rombanks, rambanks, cgb ? 8 : 2);

	rom->rewind();
	rom->read(reinterpret_cast<char*>(memptrs.romdata()), (filesize / 0x4000) * 0x4000ul);
	// In case rombanks isn't a power of 2, allocate a disabled area for invalid rombank addresses.
	std::memset(memptrs.romdata() + (filesize / 0x4000) * 0x4000ul, 0xFF, (rombanks - filesize / 0x4000) * 0x4000ul);
	enforce8bit(memptrs.romdata(), rombanks * 0x4000ul);
	
	if ((multi64rom = !rambanks && rombanks == 64 && cartridgeType(memptrs.romdata()[0x147]) == MBC1 && multicartCompat))
		logger_printf(0, "Multi-ROM \"MBC1\" presumed");
	

	if (rom->fail()) {
		defaultSaveBasePath.clear(); // indicates no valid ROM loaded.
		return 1;
	}

	return 0;
}

static bool hasBattery(const unsigned char headerByte0x147) {
	switch (headerByte0x147) {
	case 0x03:
	case 0x06:
	case 0x09:
	case 0x0F:
	case 0x10:
	case 0x13:
	case 0x1B:
	case 0x1E:
	case 0xFF: return true;
	default: return false;
	}
}

void Cartridge::loadSavedata() {
	const std::string &sbp = saveBasePath();

	if (hasBattery(memptrs.romdata()[0x147])) {
		std::ifstream file((sbp + ".sav").c_str(), std::ios::binary | std::ios::in);

		if (file.is_open()) {
			file.read(reinterpret_cast<char*>(memptrs.rambankdata()), memptrs.rambankdataend() - memptrs.rambankdata());
			enforce8bit(memptrs.rambankdata(), memptrs.rambankdataend() - memptrs.rambankdata());
		}
	}

	if (hasRtc(memptrs.romdata()[0x147])) {
		std::ifstream file((sbp + ".rtc").c_str(), std::ios::binary | std::ios::in);

		if (file.is_open()) {
			unsigned long basetime = file.get() & 0xFF;

			basetime = basetime << 8 | (file.get() & 0xFF);
			basetime = basetime << 8 | (file.get() & 0xFF);
			basetime = basetime << 8 | (file.get() & 0xFF);

			rtc.setBaseTime(basetime);
		}
	}
}

void Cartridge::saveSavedata() {
	const std::string &sbp = saveBasePath();

	if (hasBattery(memptrs.romdata()[0x147])) {
		std::ofstream file((sbp + ".sav").c_str(), std::ios::binary | std::ios::out);

		file.write(reinterpret_cast<const char*>(memptrs.rambankdata()), memptrs.rambankdataend() - memptrs.rambankdata());
	}

	if (hasRtc(memptrs.romdata()[0x147])) {
		std::ofstream file((sbp + ".rtc").c_str(), std::ios::binary | std::ios::out);
		const unsigned long basetime = rtc.getBaseTime();

		file.put(basetime >> 24 & 0xFF);
		file.put(basetime >> 16 & 0xFF);
		file.put(basetime >>  8 & 0xFF);
		file.put(basetime       & 0xFF);
	}
}

#define asHex(c) cartridge_asHex(c)
static int asHex(const char c) {
	return c >= 'A' ? c - 'A' + 0xA : c - '0';
}

void Cartridge::applyGameGenie(const std::string &code) {
	if (6 < code.length()) {
		const unsigned val = (asHex(code[0]) << 4 | asHex(code[1])) & 0xFF;
		const unsigned addr = (asHex(code[2]) << 8 | asHex(code[4]) << 4 | asHex(code[5]) | (asHex(code[6]) ^ 0xF) << 12) & 0x7FFF;
		unsigned cmp = 0xFFFF;
		
		if (10 < code.length()) {
			cmp = (asHex(code[8]) << 4 | asHex(code[10])) ^ 0xFF;
			cmp = ((cmp >> 2 | cmp << 6) ^ 0x45) & 0xFF;
		}
		
		for (unsigned bank = 0; bank < static_cast<std::size_t>(memptrs.romdataend() - memptrs.romdata()) / 0x4000; ++bank) {
			if ((addr < 0x4000) == ((bank & (multi64rom ? 0xFu : ~0u)) == 0)
					&& (cmp > 0xFF || memptrs.romdata()[bank * 0x4000ul + (addr & 0x3FFF)] == cmp)) {
				ggUndoList.push_back(AddrData(bank * 0x4000ul + (addr & 0x3FFF), memptrs.romdata()[bank * 0x4000ul + (addr & 0x3FFF)]));
				memptrs.romdata()[bank * 0x4000ul + (addr & 0x3FFF)] = val;
			}
		}
	}
}
#undef asHex

void Cartridge::setGameGenie(const std::string &codes) {
	if (loaded()) {
		for (std::vector<AddrData>::reverse_iterator it = ggUndoList.rbegin(), end = ggUndoList.rend(); it != end; ++it) {
			if (memptrs.romdata() + it->addr < memptrs.romdataend())
				memptrs.romdata()[it->addr] = it->data;
		}
		
		ggUndoList.clear();
		
		std::string code;
		for (std::size_t pos = 0; pos < codes.length()
				&& (code = codes.substr(pos, codes.find(';', pos) - pos), true); pos += code.length() + 1) {
			applyGameGenie(code);
		}
	}
}

}
