//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef CPU_H
#define CPU_H

#include "memory.h"

namespace gambatte {

class CPU {
public:
	CPU();
	long runFor(unsigned long cycles) __attribute__ ((hot));;
	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state);
	void loadState(SaveState const &state);
	void loadSavedata() { mem_.loadSavedata(); }
	void saveSavedata() { mem_.saveSavedata(); }
	std::span<unsigned char> srambank() { return mem_.srambank(); }
	std::optional<std::time_t> rtcTime() const { return mem_.rtcTime(); }
	void setRtcTime(std::time_t time) { mem_.setRtcTime(time); }

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		mem_.setVideoBuffer(videoBuf, pitch);
	}

	void setInputGetter(InputGetter *getInput) {
		mem_.setInputGetter(getInput);
	}

	void setSaveDir(std::string const &sdir) {
		mem_.setSaveDir(sdir);
	}

	std::string const saveBasePath() const {
		return mem_.saveBasePath();
	}

	#ifndef GAMBATTE_NO_OSD
	void setOsdElement(transfer_ptr<OsdElement> osdElement) {
		mem_.setOsdElement(osdElement);
	}
	#endif

	LoadRes load(const void *romdata, std::size_t size, std::string const &romfilename, bool forceDmg, bool multicartCompat) {
		return mem_.loadROM(romdata, size, romfilename, forceDmg, multicartCompat);
	}

	bool loaded() const { return mem_.loaded(); }
	char const * romTitle() const { return mem_.romTitle(); }
	PakInfo const pakInfo(bool multicartCompat) const { return mem_.pakInfo(multicartCompat); }
	void setSoundBuffer(uint_least32_t *buf) { mem_.setSoundBuffer(buf); }
	std::size_t fillSoundBuffer() { return mem_.fillSoundBuffer(cycleCounter_); }
	bool isCgb() const { return mem_.isCgb(); }

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		mem_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}

	void refreshPalettes() { mem_.refreshPalettes(); }
	void setColorConversionFlags(unsigned flags) { mem_.setColorConversionFlags(flags); }

	void setGameGenie(std::string const &codes) { mem_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { mem_.setGameShark(codes); }

private:
	Memory mem_;
	unsigned long cycleCounter_;
	unsigned short pc_;
	unsigned short sp;
	unsigned hf1, hf2, zf, cf;
	unsigned char a_, b, c, d, e, /*f,*/ h, l;
	unsigned char opcode_;
	bool prefetched_;

	void process(unsigned long cycles) __attribute__ ((hot));;
};

}

#endif
