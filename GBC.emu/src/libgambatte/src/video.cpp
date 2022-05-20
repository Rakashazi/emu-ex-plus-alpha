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

#include "video.h"
#include "savestate.h"

#include <algorithm>

using namespace gambatte;

uint_least32_t gbcToRgb32(unsigned const bgr15, unsigned flags);

namespace {

// TODO: simplify cycle offsets.

long const mode1_irq_frame_cycle = 1l * lcd_vres * lcd_cycles_per_line - 2;
int const mode2_irq_line_cycle = lcd_cycles_per_line - 4;
int const mode2_irq_line_cycle_ly0 = lcd_cycles_per_line - 2;

unsigned long mode1IrqSchedule(LyCounter const &lyCounter, unsigned long cc) {
	return lyCounter.nextFrameCycle(mode1_irq_frame_cycle, cc);
}

unsigned long mode2IrqSchedule(unsigned const statReg,
		LyCounter const &lyCounter, unsigned long const cc) {
	if (!(statReg & lcdstat_m2irqen))
		return disabled_time;

	unsigned long const lastM2Fc = (lcd_vres - 1l) * lcd_cycles_per_line + mode2_irq_line_cycle;
	unsigned long const ly0M2Fc = (lcd_lines_per_frame - 1l) * lcd_cycles_per_line + mode2_irq_line_cycle_ly0;
	return lyCounter.frameCycles(cc) - lastM2Fc < ly0M2Fc - lastM2Fc || (statReg & lcdstat_m0irqen)
	? lyCounter.nextFrameCycle(ly0M2Fc, cc)
	: lyCounter.nextLineCycle(mode2_irq_line_cycle, cc);
}

unsigned long m0TimeOfCurrentLine(
		unsigned long nextLyTime,
		unsigned long lastM0Time,
		unsigned long nextM0Time) {
	return nextM0Time < nextLyTime ? nextM0Time : lastM0Time;
}

bool isHdmaPeriod(LyCounter const &lyCounter,
		unsigned long m0TimeOfCurrentLy, unsigned long cc) {
	return lyCounter.ly() < lcd_vres
	&& cc + 3 + 3 * lyCounter.isDoubleSpeed() < lyCounter.time()
	&& cc >= m0TimeOfCurrentLy;
}

void doCgbColorChange(unsigned char *pdata,
		unsigned long *palette, unsigned index, unsigned data, unsigned flags) {
	pdata[index] = data;
	index /= 2;
	palette[index] = gbcToRgb32(pdata[index * 2] | pdata[index * 2 + 1] * 0x100l, flags);
}

} // unnamed namespace.

void LCD::setDmgPalette(unsigned long palette[], unsigned long const dmgColors[], unsigned data) {
	for (int i = 0; i < num_palette_entries; ++i, data /= num_palette_entries)
		palette[i] = dmgColors[data % num_palette_entries];
}

LCD::LCD(unsigned char const *oamram, unsigned char const *vram,
         VideoInterruptRequester memEventRequester)
: ppu_(nextM0Time_, oamram, vram)
, bgpData_()
, objpData_()
, eventTimes_(memEventRequester)
, statReg_(0)
{
	for (std::size_t pno = 0; pno < sizeof dmgColorsRgb32_ / sizeof dmgColorsRgb32_[0]; ++pno)
	for (std::size_t i = 0; i < num_palette_entries; ++i)
		dmgColorsRgb32_[pno][i] = 85 * 0x010101l * (num_palette_entries - 1 - i);

	reset(oamram, vram, false);
	setVideoBuffer(0, lcd_hres);
}

void LCD::reset(unsigned char const *oamram, unsigned char const *vram, bool cgb) {
	ppu_.reset(oamram, vram, cgb);
	lycIrq_.setCgb(cgb);
	refreshPalettes();
}

void LCD::setStatePtrs(SaveState &state) {
	state.ppu.bgpData.set(  bgpData_, sizeof  bgpData_);
	state.ppu.objpData.set(objpData_, sizeof objpData_);
	ppu_.setStatePtrs(state);
}

void LCD::saveState(SaveState &state) const {
	state.mem.hdmaTransfer = hdmaIsEnabled();
	state.ppu.nextM0Irq = eventTimes_(memevent_m0irq) - ppu_.now();
	state.ppu.pendingLcdstatIrq = eventTimes_(memevent_oneshot_statirq) != disabled_time;

	lycIrq_.saveState(state);
	mstatIrq_.saveState(state);
	ppu_.saveState(state);
}

void LCD::loadState(SaveState const &state, unsigned char const *const oamram) {
	statReg_ = state.mem.ioamhram.get()[0x141];

	ppu_.loadState(state, oamram);
	lycIrq_.loadState(state);
	mstatIrq_.loadState(state);

	if (ppu_.lcdc() & lcdc_en) {
		nextM0Time_.predictNextM0Time(ppu_);
		lycIrq_.reschedule(ppu_.lyCounter(), ppu_.now());

		eventTimes_.setm<memevent_oneshot_statirq>(state.ppu.pendingLcdstatIrq
			? ppu_.now() + 1
			: 1 * disabled_time);
		eventTimes_.setm<memevent_oneshot_updatewy2>(
			state.ppu.oldWy != state.mem.ioamhram.get()[0x14A]
			? ppu_.now() + 2 - isDoubleSpeed()
			: 1 * disabled_time);
		eventTimes_.set<event_ly>(ppu_.lyCounter().time());
		eventTimes_.setm<memevent_spritemap>(
			SpriteMapper::schedule(ppu_.lyCounter(), ppu_.now()));
		eventTimes_.setm<memevent_lycirq>(lycIrq_.time());
		eventTimes_.setm<memevent_m1irq>(mode1IrqSchedule(ppu_.lyCounter(), ppu_.now()));
		eventTimes_.setm<memevent_m2irq>(
			mode2IrqSchedule(statReg_, ppu_.lyCounter(), ppu_.now()));
		eventTimes_.setm<memevent_m0irq>(statReg_ & lcdstat_m0irqen
			? ppu_.now() + state.ppu.nextM0Irq
			: 1 * disabled_time);
		eventTimes_.setm<memevent_hdma>(state.mem.hdmaTransfer
			? nextM0Time_.predictedNextM0Time()
			: 1 * disabled_time);
	} else for (int i = 0; i < num_memevents; ++i)
		eventTimes_.set(MemEvent(i), disabled_time);

	refreshPalettes();
}

void LCD::refreshPalettes() {
	if (ppu_.cgb()) {
		for (int i = 0; i < max_num_palettes * num_palette_entries; ++i) {
			ppu_.bgPalette()[i] = gbcToRgb32( bgpData_[2 * i] |  bgpData_[2 * i + 1] * 0x100l, colorConvFlags);
			ppu_.spPalette()[i] = gbcToRgb32(objpData_[2 * i] | objpData_[2 * i + 1] * 0x100l, colorConvFlags);
		}
	} else {
		setDmgPalette(ppu_.bgPalette(), dmgColorsRgb32_[0],  bgpData_[0]);
		setDmgPalette(ppu_.spPalette(), dmgColorsRgb32_[1], objpData_[0]);
		setDmgPalette(ppu_.spPalette() + num_palette_entries, dmgColorsRgb32_[2], objpData_[1]);
	}
}

namespace {

template <class Blend>
void blitOsdElement(uint_least32_t *d, uint_least32_t const *s,
		unsigned const width, unsigned h, std::ptrdiff_t const dpitch,
		Blend blend)
{
	for (; h > 0; --h, d += dpitch, s += width) {
		for (unsigned i = 0; i < width; ++i) {
			if (s[i] != OsdElement::pixel_transparent)
				d[i] = blend(s[i], d[i]);
		}
	}
}

template <unsigned weight>
struct Blend {
	enum { sw = weight - 1 };
	enum { lowmask = sw * 0x010101l };

	uint_least32_t operator()(uint_least32_t s, uint_least32_t d) const {
		return (s * sw + d - (((s & lowmask) * sw + (d & lowmask)) & lowmask)) / weight;
	}
};

template <typename T>
void clear(T *buf, unsigned long color, std::ptrdiff_t dpitch) {
	for (unsigned lines = lcd_vres; lines > 0; --lines, buf += dpitch)
		std::fill_n(buf, 1 * lcd_hres, color);
}

}

void LCD::updateScreen(bool const blanklcd, unsigned long const cycleCounter) {
	update(cycleCounter);

	if (blanklcd && ppu_.frameBuf().fb()) {
		unsigned long color = ppu_.cgb() ? 0xFFFFFFFF : dmgColorsRgb32_[0][0];
		clear(ppu_.frameBuf().fb(), color, ppu_.frameBuf().pitch());
	}

	#ifndef GAMBATTE_NO_OSD
	if (ppu_.frameBuf().fb() && osdElement_) {
		if (uint_least32_t const *const s = osdElement_->update()) {
			uint_least32_t *const d = ppu_.frameBuf().fb()
				+ static_cast<std::ptrdiff_t>(osdElement_->y()) * ppu_.frameBuf().pitch()
				+ osdElement_->x();

			switch (osdElement_->opacity()) {
			case OsdElement::seven_eighths:
				blitOsdElement(d, s, osdElement_->w(), osdElement_->h(),
					ppu_.frameBuf().pitch(), Blend<8>());
				break;
			case OsdElement::three_fourths:
				blitOsdElement(d, s, osdElement_->w(), osdElement_->h(),
					ppu_.frameBuf().pitch(), Blend<4>());
				break;
			}
		} else
			osdElement_.reset();
	}
	#endif
}

void LCD::resetCc(unsigned long const oldCc, unsigned long const newCc) {
	update(oldCc);
	ppu_.resetCc(oldCc, newCc);

	if (ppu_.lcdc() & lcdc_en) {
		unsigned long const dec = oldCc - newCc;

		nextM0Time_.invalidatePredictedNextM0Time();
		lycIrq_.reschedule(ppu_.lyCounter(), newCc);

		for (int i = 0; i < num_memevents; ++i) {
			if (eventTimes_(MemEvent(i)) != disabled_time)
				eventTimes_.set(MemEvent(i), eventTimes_(MemEvent(i)) - dec);
		}

		eventTimes_.set<event_ly>(ppu_.lyCounter().time());
	}
}

void LCD::speedChange(unsigned long const cc) {
	update(cc);
	ppu_.speedChange();

	if (ppu_.lcdc() & lcdc_en) {
		nextM0Time_.predictNextM0Time(ppu_);
		lycIrq_.reschedule(ppu_.lyCounter(), ppu_.now());

		eventTimes_.set<event_ly>(ppu_.lyCounter().time());
		eventTimes_.setm<memevent_spritemap>(SpriteMapper::schedule(ppu_.lyCounter(), ppu_.now()));
		eventTimes_.setm<memevent_lycirq>(lycIrq_.time());
		eventTimes_.setm<memevent_m1irq>(mode1IrqSchedule(ppu_.lyCounter(), ppu_.now()));
		eventTimes_.setm<memevent_m2irq>(mode2IrqSchedule(statReg_, ppu_.lyCounter(), ppu_.now()));

		if (eventTimes_(memevent_m0irq) != disabled_time) {
			eventTimes_.setm<memevent_m0irq>(ppu_.predictedNextXposTime(lcd_hres + 6));
		}
		if (hdmaIsEnabled()) {
			eventTimes_.setm<memevent_hdma>(nextM0Time_.predictedNextM0Time());
		}
	}
}

unsigned long LCD::m0TimeOfCurrentLine(unsigned long const cc) {
	if (cc >= nextM0Time_.predictedNextM0Time()) {
		update(cc);
		nextM0Time_.predictNextM0Time(ppu_);
	}

	return ::m0TimeOfCurrentLine(ppu_.lyCounter().time(), ppu_.lastM0Time(),
		nextM0Time_.predictedNextM0Time());
}

void LCD::enableHdma(unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	if (::isHdmaPeriod(ppu_.lyCounter(), m0TimeOfCurrentLine(cc), cc + 4))
		eventTimes_.flagHdmaReq();

	eventTimes_.setm<memevent_hdma>(nextM0Time_.predictedNextM0Time());
}

void LCD::disableHdma(unsigned long const cycleCounter) {
	if (cycleCounter >= eventTimes_.nextEventTime())
		update(cycleCounter);

	eventTimes_.setm<memevent_hdma>(disabled_time);
}

bool LCD::isHdmaPeriod(unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	return ::isHdmaPeriod(ppu_.lyCounter(), m0TimeOfCurrentLine(cc), cc);
}

bool LCD::vramReadable(unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	return !(ppu_.lcdc() & lcdc_en)
	|| ppu_.lyCounter().ly() >= lcd_vres
	|| ppu_.inactivePeriodAfterDisplayEnable(cc + 1 - ppu_.cgb() + isDoubleSpeed())
	|| ppu_.lyCounter().lineCycles(cc) + isDoubleSpeed() < 76u + 3 * ppu_.cgb()
	|| cc + 2 >= m0TimeOfCurrentLine(cc);
}

bool LCD::vramWritable(unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	return !(ppu_.lcdc() & lcdc_en)
	|| ppu_.lyCounter().ly() >= lcd_vres
	|| ppu_.inactivePeriodAfterDisplayEnable(cc + 1 - ppu_.cgb() + isDoubleSpeed())
	|| ppu_.lyCounter().lineCycles(cc) + isDoubleSpeed() < 79
	|| cc + 2 >= m0TimeOfCurrentLine(cc);
}

bool LCD::cgbpAccessible(unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	return !(ppu_.lcdc() & lcdc_en)
	|| ppu_.lyCounter().ly() >= lcd_vres
	|| ppu_.inactivePeriodAfterDisplayEnable(cc)
	|| ppu_.lyCounter().lineCycles(cc) + isDoubleSpeed() < 80
	|| cc >= m0TimeOfCurrentLine(cc) + 2;
}

void LCD::doCgbBgColorChange(unsigned index, unsigned data, unsigned long cc) {
	if (cgbpAccessible(cc)) {
		update(cc);
		doCgbColorChange(bgpData_, ppu_.bgPalette(), index, data, colorConvFlags);
	}
}

void LCD::doCgbSpColorChange(unsigned index, unsigned data, unsigned long cc) {
	if (cgbpAccessible(cc)) {
		update(cc);
		doCgbColorChange(objpData_, ppu_.spPalette(), index, data, colorConvFlags);
	}
}

bool LCD::oamReadable(unsigned long const cc) {
	if (!(ppu_.lcdc() & lcdc_en) || ppu_.inactivePeriodAfterDisplayEnable(cc + 4))
		return true;

	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	if (ppu_.lyCounter().lineCycles(cc) + 4 - isDoubleSpeed() >= lcd_cycles_per_line)
		return ppu_.lyCounter().ly() >= lcd_vres - 1 && ppu_.lyCounter().ly() < lcd_lines_per_frame - 1;

	return ppu_.lyCounter().ly() >= lcd_vres || cc + 2 >= m0TimeOfCurrentLine(cc);
}

bool LCD::oamWritable(unsigned long const cc) {
	if (!(ppu_.lcdc() & lcdc_en) || ppu_.inactivePeriodAfterDisplayEnable(cc + 4 + isDoubleSpeed()))
		return true;

	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	if (ppu_.lyCounter().lineCycles(cc) + 3 + ppu_.cgb() >= lcd_cycles_per_line)
		return ppu_.lyCounter().ly() >= lcd_vres - 1 && ppu_.lyCounter().ly() < lcd_lines_per_frame - 1;

	return ppu_.lyCounter().ly() >= lcd_vres || cc + 2 >= m0TimeOfCurrentLine(cc)
		|| (ppu_.lyCounter().lineCycles(cc) == 76 && !ppu_.cgb());
}

void LCD::mode3CyclesChange() {
	nextM0Time_.invalidatePredictedNextM0Time();

	if (eventTimes_(memevent_m0irq) != disabled_time
			&& eventTimes_(memevent_m0irq) > ppu_.now()) {
		unsigned long t = ppu_.predictedNextXposTime(lcd_hres + 6);
		eventTimes_.setm<memevent_m0irq>(t);
	}

	if (eventTimes_(memevent_hdma) != disabled_time
			&& eventTimes_(memevent_hdma) > ppu_.lastM0Time()) {
		nextM0Time_.predictNextM0Time(ppu_);
		eventTimes_.setm<memevent_hdma>(nextM0Time_.predictedNextM0Time());
	}
}

void LCD::wxChange(unsigned newValue, unsigned long cycleCounter) {
	update(cycleCounter + 1 + ppu_.cgb());
	ppu_.setWx(newValue);
	mode3CyclesChange();
}

void LCD::wyChange(unsigned const newValue, unsigned long const cc) {
	update(cc + 1 + ppu_.cgb());
	ppu_.setWy(newValue); 

	// mode3CyclesChange();
	// (should be safe to wait until after wy2 delay, because no mode3 events are
	// close to when wy1 is read.)

	// wy2 is a delayed version of wy for convenience (is this really simpler?).
	if (ppu_.cgb() && (ppu_.lcdc() & lcdc_en)) {
		eventTimes_.setm<memevent_oneshot_updatewy2>(cc + 6 - isDoubleSpeed());
	} else {
		update(cc + 2);
		ppu_.updateWy2();
		mode3CyclesChange();
	}
}

void LCD::scxChange(unsigned newScx, unsigned long cycleCounter) {
	update(cycleCounter + 2 * ppu_.cgb());
	ppu_.setScx(newScx);
	mode3CyclesChange();
}

void LCD::scyChange(unsigned newValue, unsigned long cycleCounter) {
	update(cycleCounter + 2 * ppu_.cgb());
	ppu_.setScy(newValue);
}

void LCD::oamChange(unsigned long cc) {
	if (ppu_.lcdc() & lcdc_en) {
		update(cc);
		ppu_.oamChange(cc);
		eventTimes_.setm<memevent_spritemap>(SpriteMapper::schedule(ppu_.lyCounter(), cc));
	}
}

void LCD::oamChange(unsigned char const *oamram, unsigned long cc) {
	update(cc);
	ppu_.oamChange(oamram, cc);

	if (ppu_.lcdc() & lcdc_en)
		eventTimes_.setm<memevent_spritemap>(SpriteMapper::schedule(ppu_.lyCounter(), cc));
}

void LCD::lcdcChange(unsigned const data, unsigned long const cc) {
	unsigned const oldLcdc = ppu_.lcdc();

	if ((oldLcdc ^ data) & lcdc_en) {
		update(cc);
		ppu_.setLcdc(data, cc);

		if (data & lcdc_en) {
			lycIrq_.lcdReset();
			mstatIrq_.lcdReset(lycIrq_.lycReg());
			nextM0Time_.predictNextM0Time(ppu_);
			lycIrq_.reschedule(ppu_.lyCounter(), cc);

			eventTimes_.set<event_ly>(ppu_.lyCounter().time());
			eventTimes_.setm<memevent_spritemap>(
				SpriteMapper::schedule(ppu_.lyCounter(), cc));
			eventTimes_.setm<memevent_lycirq>(lycIrq_.time());
			eventTimes_.setm<memevent_m1irq>(mode1IrqSchedule(ppu_.lyCounter(), cc));
			eventTimes_.setm<memevent_m2irq>(
				mode2IrqSchedule(statReg_, ppu_.lyCounter(), cc));
			if (statReg_ & lcdstat_m0irqen) {
				eventTimes_.setm<memevent_m0irq>(ppu_.predictedNextXposTime(lcd_hres + 6));
			}
			if (hdmaIsEnabled()) {
				eventTimes_.setm<memevent_hdma>(nextM0Time_.predictedNextM0Time());
			}
		} else for (int i = 0; i < num_memevents; ++i)
			eventTimes_.set(MemEvent(i), disabled_time);
	} else if (data & lcdc_en) {
		if (ppu_.cgb()) {
			update(cc + 1);
			ppu_.setLcdc((oldLcdc & ~(1u * lcdc_tdsel)) | (data & lcdc_tdsel), cc + 1);
			update(cc + 2);
			ppu_.setLcdc(data, cc + 2);
			if ((oldLcdc ^ data) & lcdc_obj2x) {
				unsigned long t = SpriteMapper::schedule(ppu_.lyCounter(), cc + 2);
				eventTimes_.setm<memevent_spritemap>(t);
			}
			if ((oldLcdc ^ data) & lcdc_we)
				mode3CyclesChange();
		} else {
			update(cc);
			ppu_.setLcdc((oldLcdc & lcdc_obj2x) | (data & ~(1u * lcdc_obj2x)), cc);
			if ((oldLcdc ^ data) & lcdc_obj2x) {
				update(cc + 2);
				ppu_.setLcdc(data, cc + 2);
				unsigned long t = SpriteMapper::schedule(ppu_.lyCounter(), cc + 2);
				eventTimes_.setm<memevent_spritemap>(t);
			}
			if ((oldLcdc ^ data) & (lcdc_we | lcdc_objen))
				mode3CyclesChange();
		}
	} else {
		update(cc);
		ppu_.setLcdc(data, cc);
	}
}

namespace {

struct LyCnt {
	unsigned ly; int timeToNextLy;
	LyCnt(unsigned ly, int timeToNextLy) : ly(ly), timeToNextLy(timeToNextLy) {}
};

LyCnt const getLycCmpLy(LyCounter const &lyCounter, unsigned long cc) {
	unsigned ly = lyCounter.ly();
	int timeToNextLy = lyCounter.time() - cc;

	if (ly == lcd_lines_per_frame - 1) {
		int const lineTime = lyCounter.lineTime();
		if ((timeToNextLy -= (lineTime - 6 - 6 * lyCounter.isDoubleSpeed())) <= 0)
			ly = 0, timeToNextLy += lineTime;
	} else if ((timeToNextLy -= (2 + 2 * lyCounter.isDoubleSpeed())) <= 0)
		++ly, timeToNextLy += lyCounter.lineTime();

	return LyCnt(ly, timeToNextLy);
}

bool statChangeTriggersM2IrqCgb(unsigned const old,
		unsigned const data, int const ly, int const timeToNextLy, bool const ds) {
	if ((old & lcdstat_m2irqen)
			|| (data & (lcdstat_m2irqen | lcdstat_m0irqen)) != lcdstat_m2irqen) {
		return false;
	}
	if (ly < lcd_vres - 1)
		return timeToNextLy <= (lcd_cycles_per_line - mode2_irq_line_cycle) * (1 + ds) && timeToNextLy > 2;
	if (ly == lcd_vres - 1)
		return timeToNextLy <= (lcd_cycles_per_line - mode2_irq_line_cycle) * (1 + ds) && timeToNextLy > 4 + 2 * ds;
	if (ly == lcd_lines_per_frame - 1)
		return timeToNextLy <= (lcd_cycles_per_line - mode2_irq_line_cycle_ly0) * (1 + ds) && timeToNextLy > 2;
	return false;
}

unsigned incLy(unsigned ly) { return ly == lcd_lines_per_frame - 1 ? 0 : ly + 1; }

} // unnamed namespace.

inline bool LCD::statChangeTriggersStatIrqDmg(unsigned const old, unsigned long const cc) {
	LyCnt const lycCmp = getLycCmpLy(ppu_.lyCounter(), cc);

	if (ppu_.lyCounter().ly() < lcd_vres) {
		int const m0_cycles_upper_bound = lcd_cycles_per_line - 80 - 160;
		unsigned long m0IrqTime = eventTimes_(memevent_m0irq);
		if (m0IrqTime == disabled_time && ppu_.lyCounter().time() - cc < m0_cycles_upper_bound) {
			update(cc);
			m0IrqTime = ppu_.predictedNextXposTime(lcd_hres + 6);
		}
		if (m0IrqTime == disabled_time || m0IrqTime < ppu_.lyCounter().time())
			return lycCmp.ly == lycIrq_.lycReg() && !(old & lcdstat_lycirqen);

		return !(old & lcdstat_m0irqen)
		&& !(lycCmp.ly == lycIrq_.lycReg() && (old & lcdstat_lycirqen));
	}

	return !(old & lcdstat_m1irqen)
	&& !(lycCmp.ly == lycIrq_.lycReg() && (old & lcdstat_lycirqen));
}

inline bool LCD::statChangeTriggersM0LycOrM1StatIrqCgb(
		unsigned const old, unsigned const data, bool const lycperiod,
		unsigned long const cc) {
	int const ly = ppu_.lyCounter().ly();
	int const timeToNextLy = ppu_.lyCounter().time() - cc;
	bool const ds = isDoubleSpeed();
	int const m1_irq_lc_inv = lcd_cycles_per_line - mode1_irq_frame_cycle % lcd_cycles_per_line;

	if (ly < lcd_vres - 1 || (ly == lcd_vres - 1 && timeToNextLy > m1_irq_lc_inv * (1 + ds))) {
		if (eventTimes_(memevent_m0irq) < ppu_.lyCounter().time()
				|| timeToNextLy <= (ly < lcd_vres - 1 ? 4 + 4 * ds : 4 + 2 * ds)) {
			return lycperiod && (data & lcdstat_lycirqen);
		}

		if (old & lcdstat_m0irqen)
			return false;

		return (data & lcdstat_m0irqen)
		|| (lycperiod && (data & lcdstat_lycirqen));
	}

	if (old & lcdstat_m1irqen && (ly < lcd_lines_per_frame - 1 || timeToNextLy > 3 + 3 * ds))
		return false;

	return ((data & lcdstat_m1irqen)
	        && (ly < lcd_lines_per_frame - 1 || timeToNextLy > 4 + 2 * ds))
	|| (lycperiod && (data & lcdstat_lycirqen));
}

inline bool LCD::statChangeTriggersStatIrqCgb(
		unsigned const old, unsigned const data, unsigned long const cc) {
	if (!(data & ~old & (  lcdstat_lycirqen
	                     | lcdstat_m2irqen
	                     | lcdstat_m1irqen
	                     | lcdstat_m0irqen))) {
		return false;
	}

	int const ly = ppu_.lyCounter().ly();
	int const timeToNextLy = ppu_.lyCounter().time() - cc;
	LyCnt const lycCmp = getLycCmpLy(ppu_.lyCounter(), cc);
	bool const lycperiod = lycCmp.ly == lycIrq_.lycReg()
		&& lycCmp.timeToNextLy > 2;
	if (lycperiod && (old & lcdstat_lycirqen))
		return false;

	return statChangeTriggersM0LycOrM1StatIrqCgb(old, data, lycperiod, cc)
	|| statChangeTriggersM2IrqCgb(old, data, ly, timeToNextLy, isDoubleSpeed());
}

inline bool LCD::statChangeTriggersStatIrq(unsigned old, unsigned data, unsigned long cc) {
	return ppu_.cgb()
	? statChangeTriggersStatIrqCgb(old, data, cc)
	: statChangeTriggersStatIrqDmg(old, cc);
}

void LCD::lcdstatChange(unsigned const data, unsigned long const cc) {
	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	unsigned const old = statReg_;
	statReg_ = data;
	lycIrq_.statRegChange(data, ppu_.lyCounter(), cc);

	if (ppu_.lcdc() & lcdc_en) {
		if ((data & lcdstat_m0irqen) && eventTimes_(memevent_m0irq) == disabled_time) {
			update(cc);
			eventTimes_.setm<memevent_m0irq>(ppu_.predictedNextXposTime(lcd_hres + 6));
		}

		eventTimes_.setm<memevent_m2irq>(mode2IrqSchedule(data, ppu_.lyCounter(), cc));
		eventTimes_.setm<memevent_lycirq>(lycIrq_.time());

		if (statChangeTriggersStatIrq(old, data, cc))
			eventTimes_.flagIrq(2);
	}

	mstatIrq_.statRegChange(data, eventTimes_(memevent_m0irq), eventTimes_(memevent_m1irq),
		eventTimes_(memevent_m2irq), cc, ppu_.cgb());
}

inline bool LCD::lycRegChangeStatTriggerBlockedByM0OrM1Irq(unsigned data, unsigned long cc) {
	int const timeToNextLy = ppu_.lyCounter().time() - cc;
	if (ppu_.lyCounter().ly() < lcd_vres) {
		return (statReg_ & lcdstat_m0irqen)
		&& eventTimes_(memevent_m0irq) > ppu_.lyCounter().time()
		&& data == ppu_.lyCounter().ly();
	}

	return (statReg_ & lcdstat_m1irqen)
	&& !(ppu_.lyCounter().ly() == lcd_lines_per_frame - 1
		&& timeToNextLy <= 2 + 2 * isDoubleSpeed() + 2 * ppu_.cgb());
}

bool LCD::lycRegChangeTriggersStatIrq(
		unsigned const old, unsigned const data, unsigned long const cc) {
	if (!(statReg_ & lcdstat_lycirqen) || data >= lcd_lines_per_frame
			|| lycRegChangeStatTriggerBlockedByM0OrM1Irq(data, cc)) {
		return false;
	}

	LyCnt lycCmp = getLycCmpLy(ppu_.lyCounter(), cc);
	if (lycCmp.timeToNextLy <= 4 + 4 * isDoubleSpeed() + 2 * ppu_.cgb()) {
		if (old == lycCmp.ly && lycCmp.timeToNextLy > 2 * ppu_.cgb())
			return false; // simultaneous ly/lyc inc. lyc flag never goes low -> no trigger.

		lycCmp.ly = incLy(lycCmp.ly);
	}

	return data == lycCmp.ly;
}

void LCD::lycRegChange(unsigned const data, unsigned long const cc) {
	unsigned const old = lycIrq_.lycReg();
	if (data == old)
		return;

	if (cc >= eventTimes_.nextEventTime())
		update(cc);

	lycIrq_.lycRegChange(data, ppu_.lyCounter(), cc);
	mstatIrq_.lycRegChange(data, eventTimes_(memevent_m0irq),
		eventTimes_(memevent_m2irq), cc, isDoubleSpeed(), ppu_.cgb());

	if (ppu_.lcdc() & lcdc_en) {
		eventTimes_.setm<memevent_lycirq>(lycIrq_.time());

		if (lycRegChangeTriggersStatIrq(old, data, cc)) {
			if (ppu_.cgb() && !isDoubleSpeed()) {
				eventTimes_.setm<memevent_oneshot_statirq>(cc + 5);
			} else
				eventTimes_.flagIrq(2);
		}
	}
}

unsigned LCD::getStat(unsigned const lycReg, unsigned long const cc) {
	unsigned stat = 0;

	if (ppu_.lcdc() & lcdc_en) {
		if (cc >= eventTimes_.nextEventTime())
			update(cc);

		unsigned const ly = ppu_.lyCounter().ly();
		int const timeToNextLy = ppu_.lyCounter().time() - cc;
		int const lineCycles = lcd_cycles_per_line - (timeToNextLy >> isDoubleSpeed());
		long const frameCycles = 1l * ly * lcd_cycles_per_line + lineCycles;
		if (frameCycles >= lcd_vres * lcd_cycles_per_line - 3 && frameCycles < lcd_cycles_per_frame - 3) {
			if (frameCycles >= lcd_vres * lcd_cycles_per_line - 2
					&& frameCycles < lcd_cycles_per_frame - 4 + isDoubleSpeed()) {
				stat = 1;
			}
		} else if (lineCycles < 77 || lineCycles >= lcd_cycles_per_line - 3) {
			if (!ppu_.inactivePeriodAfterDisplayEnable(cc + 1))
				stat = 2;
		} else if (cc + 2 < m0TimeOfCurrentLine(cc)) {
			if (!ppu_.inactivePeriodAfterDisplayEnable(cc + 1))
				stat = 3;
		}

		LyCnt const lycCmp = getLycCmpLy(ppu_.lyCounter(), cc);
		if (lycReg == lycCmp.ly && lycCmp.timeToNextLy > 2)
			stat |= lcdstat_lycflag;
	}

	return stat;
}

inline void LCD::doMode2IrqEvent() {
	unsigned const ly = eventTimes_(event_ly) - eventTimes_(memevent_m2irq) < 16
		? incLy(ppu_.lyCounter().ly())
		: ppu_.lyCounter().ly();
	if (mstatIrq_.doM2Event(ly, statReg_, lycIrq_.lycReg()))
		eventTimes_.flagIrq(2, eventTimes_(memevent_m2irq));

	bool const ds = isDoubleSpeed();
	unsigned long next = lcd_cycles_per_frame;
	if (!(statReg_ & lcdstat_m0irqen)) {
		next = lcd_cycles_per_line;
		if (ly == 0) {
			next -= mode2_irq_line_cycle_ly0 - mode2_irq_line_cycle;
		} else if (ly == lcd_vres) {
			next += lcd_cycles_per_line * (lcd_lines_per_frame - lcd_vres - 1)
				+ mode2_irq_line_cycle_ly0 - mode2_irq_line_cycle;
		}
	}
	eventTimes_.setm<memevent_m2irq>(eventTimes_(memevent_m2irq) + (next << ds));
}

inline void LCD::event() {
	switch (eventTimes_.nextEvent()) {
	case event_mem:
		switch (eventTimes_.nextMemEvent()) {
		case memevent_m1irq:
			eventTimes_.flagIrq(mstatIrq_.doM1Event(statReg_) ? 3 : 1,
				eventTimes_(memevent_m1irq));
			eventTimes_.setm<memevent_m1irq>(eventTimes_(memevent_m1irq)
				+ (lcd_cycles_per_frame << isDoubleSpeed()));
			break;

		case memevent_lycirq:
			if (lycIrq_.doEvent(ppu_.lyCounter()))
				eventTimes_.flagIrq(2, eventTimes_(memevent_lycirq));

			eventTimes_.setm<memevent_lycirq>(lycIrq_.time());
			break;

		case memevent_spritemap:
			eventTimes_.setm<memevent_spritemap>(
				ppu_.doSpriteMapEvent(eventTimes_(memevent_spritemap)));
			mode3CyclesChange();
			break;

		case memevent_hdma:
			eventTimes_.flagHdmaReq();
			nextM0Time_.predictNextM0Time(ppu_);
			eventTimes_.setm<memevent_hdma>(nextM0Time_.predictedNextM0Time());
			break;

		case memevent_m2irq:
			doMode2IrqEvent();
			break;

		case memevent_m0irq:
			if (mstatIrq_.doM0Event(ppu_.lyCounter().ly(), statReg_, lycIrq_.lycReg()))
				eventTimes_.flagIrq(2, eventTimes_(memevent_m0irq));

			eventTimes_.setm<memevent_m0irq>(statReg_ & lcdstat_m0irqen
				? ppu_.predictedNextXposTime(lcd_hres + 6)
				: 1 * disabled_time);
			break;

		case memevent_oneshot_statirq:
			eventTimes_.flagIrq(2);
			eventTimes_.setm<memevent_oneshot_statirq>(disabled_time);
			break;

		case memevent_oneshot_updatewy2:
			ppu_.updateWy2();
			mode3CyclesChange();
			eventTimes_.setm<memevent_oneshot_updatewy2>(disabled_time);
			break;
		}

		break;

	case event_ly:
		ppu_.doLyCountEvent();
		eventTimes_.set<event_ly>(ppu_.lyCounter().time());
		break;
	}
}

void LCD::update(unsigned long const cycleCounter) {
	if (!(ppu_.lcdc() & lcdc_en))
		return;

	while (cycleCounter >= eventTimes_.nextEventTime()) {
		ppu_.update(eventTimes_.nextEventTime());
		event();
	}

	ppu_.update(cycleCounter);
}

void LCD::setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
	ppu_.setFrameBuf(videoBuf, pitch);
}

void LCD::setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32) {
	if (palNum < sizeof dmgColorsRgb32_ / sizeof dmgColorsRgb32_[0]
			&& colorNum < num_palette_entries) {
		dmgColorsRgb32_[palNum][colorNum] = rgb32;
		refreshPalettes();
	}
}
