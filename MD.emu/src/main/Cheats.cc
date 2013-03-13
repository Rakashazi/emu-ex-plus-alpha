/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <main/Cheats.hh>
#include <io/sys.hh>
#include <EmuSystem.hh>
#include <MsgPopup.hh>
#include <TextEntry.hh>
#include <util/gui/ViewStack.hh>
#include "EmuCheatViews.hh"
#include "system.h"
#include "z80.h"
#include "loadrom.h"
#include "md_cart.h"
#include "genesis.h"
extern MsgPopup popup;
extern CollectTextInputView textInputView;
extern ViewStack viewStack;
StaticDLList<MdCheat, EmuCheats::MAX> cheatList;
StaticDLList<MdCheat*, EmuCheats::MAX> romCheatList;
StaticDLList<MdCheat*, EmuCheats::MAX> ramCheatList;
bool cheatsModified = 0;

// Decode cheat string into address/data components (derived from Genesis Plus GX function)
static uint decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData)
{
	static const char arvalidchars[] = "0123456789ABCDEF";

	// 16-bit Game Genie code (ABCD-EFGH)
	if((strlen(string) >= 9) && (string[4] == '-'))
	{
		// 16-bit system only
		if(!emuSystemIs16Bit())
		{
			return 0;
		}

		static const char ggvalidchars[] = "ABCDEFGHJKLMNPRSTVWXYZ0123456789";

		iterateTimes(8, i)
		{
			if(i == 4) string++;
			auto p = strchr(ggvalidchars, *string++);
			if(!p) return 0;
			auto n = p - ggvalidchars;

			switch (i)
			{
				case 0:
				data |= n << 3;
				break;

				case 1:
				data |= n >> 2;
				address |= (n & 3) << 14;
				break;

				case 2:
				address |= n << 9;
				break;

				case 3:
				address |= (n & 0xF) << 20 | (n >> 4) << 8;
				break;

				case 4:
				data |= (n & 1) << 12;
				address |= (n >> 1) << 16;
				break;

				case 5:
				data |= (n & 1) << 15 | (n >> 1) << 8;
				break;

				case 6:
				data |= (n >> 3) << 13;
				address |= (n & 7) << 5;
				break;

				case 7:
				address |= n;
				break;
			}
		}

		#ifdef LSB_FIRST
		if(!(data & 0xFF00))
		{
			address ^= 1;
		}
		#endif

		// code length
		return 9;
	}

	// 8-bit Game Genie code (DDA-AAA-XXX)
	else if((strlen(string) >= 11) && (string[3] == '-') && (string[7] == '-'))
	{
		// 8-bit system only
		if(emuSystemIs16Bit())
		{
			return 0;
		}

		// decode 8-bit data
		iterateTimes(2, i)
		{
			auto p = strchr(arvalidchars, *string++);
			if(!p) return 0;
			auto n = (p - arvalidchars) & 0xF;
			data |= (n  << ((1 - i) * 4));
		}

		// decode 16-bit address (low 12-bits)
		iterateTimes(3, i)
		{
			if(i==1) string++; // skip separator
			auto p = strchr (arvalidchars, *string++);
			if(!p) return 0;
			auto n = (p - arvalidchars) & 0xF;
			address |= (n  << ((2 - i) * 4));
		}

		// decode 16-bit address (high 4-bits)
		auto p = strchr (arvalidchars, *string++);
		if(!p) return 0;
		auto n = (p - arvalidchars) & 0xF;
		n ^= 0xF; // bits inversion
		address |= (n  << 12);

		// RAM address are also supported
		if(address >= 0xC000)
		{
			// convert to 24-bit Work RAM address
			address = 0xFF0000 | (address & 0x1FFF);
		}

		// decode reference 8-bit data
		uint8 ref = 0;
		iterateTimes(2, i)
		{
			string++; // skip separator and 2nd digit
			auto p = strchr (arvalidchars, *string++);
			if (p == NULL) return 0;
			auto n = (p - arvalidchars) & 0xF;
			ref |= (n  << ((1 - i) * 4));
		}
		ref = (ref >> 2) | ((ref & 0x03) << 6);  // 2-bit right rotation
		ref ^= 0xBA;  // XOR

		// update old data value
		originalData = ref;

		// code length
		return 11;
	}

	// Action Replay code
	else if (string[6] == ':')
	{
		if(emuSystemIs16Bit())
		{
			// 16-bit code (AAAAAA:DDDD)
			if(strlen(string) < 11) return 0;

			// decode 24-bit address
			iterateTimes(6, i)
			{
				auto p = strchr(arvalidchars, *string++);
				if(!p) return 0;
				auto n = (p - arvalidchars) & 0xF;
				address |= (n << ((5 - i) * 4));
			}

			// decode 16-bit data
			string++;
			iterateTimes(4, i)
			{
				auto p = strchr(arvalidchars, *string++);
				if(!p) return 0;
				auto n = (p - arvalidchars) & 0xF;
				data |= (n << ((3 - i) * 4));
			}

			#ifdef LSB_FIRST
			if(!(data & 0xFF00))
			{
				address ^= 1;
			}
			#endif

			// code length
			return 11;
		}
		else
		{
			// 8-bit code (xxAAAA:DD)
			if(strlen(string) < 9) return 0;

			// decode 16-bit address
			string+=2;
			iterateTimes(4, i)
			{
				auto p = strchr(arvalidchars, *string++);
				if(!p) return 0;
				auto n = (p - arvalidchars) & 0xF;
				address |= (n << ((3 - i) * 4));
			}

			// ROM addresses are not supported
			if(address < 0xC000) return 0;

			// convert to 24-bit Work RAM address
			address = 0xFF0000 | (address & 0x1FFF);

			// decode 8-bit data
			string++;
			iterateTimes(2, i)
			{
				auto p = strchr(arvalidchars, *string++);
				if(!p) return 0;
				auto n = (p - arvalidchars) & 0xF;
				data |= (n  << ((1 - i) * 4));
			}

			// code length
			return 9;
		}
	}

	// return code length (0 = invalid)
	return 0;
}

void applyCheats()
{
  forEachInDLList(&cheatList, e)
  {
  	assert(!e.isApplied()); // make sure cheats have been cleared beforehand
    if(e.isOn() && strlen(e.code))
    {
    	logMsg("applying cheat: %s", e.name);
      if(e.address < cart.romsize)
      {
        if(emuSystemIs16Bit())
        {
          // patch ROM data
        	e.origData = *(uint16a*)(cart.rom + (e.address & 0xFFFFFE));
          *(uint16a*)(cart.rom + (e.address & 0xFFFFFE)) = e.data;
        }
        else
        {
          // add ROM patch
        	romCheatList.addToEnd(&e);

          // get current banked ROM address
          auto ptr = &z80_readmap[(e.address) >> 10][e.address & 0x03FF];

          /* check if reference matches original ROM data */
          if (((uint8)e.origData) == *ptr)
          {
            /* patch data */
            *ptr = e.data;

            /* save patched ROM address */
            e.prev = ptr;
          }
          else
          {
            /* no patched ROM address yet */
            e.prev = nullptr;
          }
        }
      }
      else if(e.address >= 0xFF0000)
      {
        // add RAM patch
      	ramCheatList.addToEnd(&e);
      }
      e.setApplied(1);
    }
  }
  if(romCheatList.size || ramCheatList.size)
  {
  	logMsg("%d RAM cheats, %d ROM cheats active", ramCheatList.size, romCheatList.size);
  }
}

void clearCheats()
{
	romCheatList.removeAll();
	ramCheatList.removeAll();

  // disable cheats in reversed order in case the same address is used by multiple patches
  forEachInDLListReverse(&cheatList, e)
  {
    if(e.isApplied())
    {
    	logMsg("clearing cheat: %s", e.name);
      if(e.address < cart.romsize)
      {
        if(emuSystemIs16Bit())
        {
          // restore original ROM data
          *(uint16a*)(cart.rom + (e.address & 0xFFFFFE)) = e.origData;
        }
        else
        {
          /* check if previous banked ROM address has been patched */
          if(e.prev)
          {
            /* restore original data */
            *e.prev = e.origData;

            /* no more patched ROM address */
            e.prev = nullptr;
          }
        }
      }
      e.setApplied(0);
    }
  }
}

void clearCheatList()
{
	romCheatList.removeAll();
	ramCheatList.removeAll();
	cheatList.removeAll();
}

void updateCheats()
{
	clearCheats();
	applyCheats();
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	FsSys::cPath filename;
	sprintf(filename, "%s/%s.pat", EmuSystem::savePath(), EmuSystem::gameName);

	if(!cheatList.size)
	{
		logMsg("deleting cheats file %s", filename);
		FsSys::remove(filename);
		cheatsModified = 0;
		return;
	}

	auto file = IoSys::create(filename);
	if(!file)
	{
		logMsg("error creating cheats file %s", filename);
		return;
	}
	logMsg("writing cheats file %s", filename);

	forEachInDLList(&cheatList, e)
	{
		if(!strlen(e.code))
		{
			continue; // ignore incomplete code entries
		}
		file->fwrite(e.code, strlen(e.code), 1);
		file->writeVar('\t');
		file->fwrite(e.name, strlen(e.name), 1);
		file->writeVar('\n');
		if(e.isOn())
		{
			file->fwrite("ON\n", strlen("ON\n"), 1);
		}
	}
	file->close();
	cheatsModified = 0;
}

void readCheatFile()
{
	FsSys::cPath filename;
	sprintf(filename, "%s/%s.pat", EmuSystem::savePath(), EmuSystem::gameName);
	auto file = IoSys::open(filename);
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file %s", filename);

	char line[256];
	while(file->readLine(line, sizeof(line)) == OK)
	{
		logMsg("got line: %s", line);
		MdCheat cheat;
		auto items = sscanf(line, "%11s %" PP_STRINGIFY_EXP(MAX_CHEAT_NAME_CHARS) "c", cheat.code, cheat.name);

		if(items == 2) // code & name
		{
			string_toUpper(cheat.code);
			if(!decodeCheat(cheat.code, cheat.address, cheat.data, cheat.origData))
			{
				logWarn("Invalid code %s from cheat file", cheat.code);
				continue;
			}
			logMsg("read cheat %s : %s", cheat.name, cheat.code);
			if(!cheatList.addToEnd(cheat))
			{
				logErr("cheat list full while reading from file");
				break;
			}
		}
		else if(items == 1) // ON/OFF string
		{
			if(string_equal(cheat.code, "ON"))
			{
				auto lastCheat = cheatList.last();
				if(lastCheat)
				{
					lastCheat->toggleOn();
					logMsg("turned on cheat %s from file", lastCheat->name);
				}
			}
		}
	}
	file->close();
}

void RAMCheatUpdate()
{
	forEachDInDLList(&ramCheatList, e)
	{
		// apply RAM patch
		if(e.data & 0xFF00)
		{
			// word patch
			*(uint16*)(work_ram + (e.address & 0xFFFE)) = e.data;
		}
		else
		{
			// byte patch
			work_ram[e.address & 0xFFFF] = e.data;
		}
	}
}

void ROMCheatUpdate()
{
	forEachDInDLList(&romCheatList, e)
	{
		// check if previous banked ROM address was patched
		if (e.prev)
		{
			// restore original data
			*e.prev = e.origData;

			// no more patched ROM address
			e.prev = nullptr;
		}

		// get current banked ROM address
		auto ptr = &z80_readmap[(e.address) >> 10][e.address & 0x03FF];

		// check if reference matches original ROM data
		if (((uint8)e.origData) == *ptr)
		{
			// patch data
			*ptr = e.data;

			// save patched ROM address
			e.prev = ptr;
		}
	}
}

uint SystemEditCheatView::handleGgCodeFromTextInput(const char *str)
{
	if(str)
	{
		string_copy(cheat->code, str);
		string_toUpper(cheat->code);
		if(!decodeCheat(cheat->code, cheat->address, cheat->data, cheat->origData))
		{
			cheat->code[0]= 0;
			popup.postError("Invalid code");
			Base::displayNeedsUpdate();
			return 1;
		}

		cheatsModified = 1;
		updateCheats();
		ggCode.compile();
		Base::displayNeedsUpdate();
	}
	removeModalView();
	return 0;
}

void SystemEditCheatView::ggCodeHandler(DualTextMenuItem &item, const Input::Event &e)
{
	static const char *inputCode8BitStr = "Input xxx-xxx-xxx (GG) or xxxxxx:xx (AR) code";
	static const char *inputCode16BitStr = "Input xxxx-xxxx (GG) or xxxxxx:xxxx (AR) code";
	textInputView.init(emuSystemIs16Bit() ? inputCode16BitStr : inputCode8BitStr, cheat->code);
	textInputView.onTextDelegate().bind<template_mfunc(SystemEditCheatView, handleGgCodeFromTextInput)>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}

void SystemEditCheatView::renamed(const char *str)
{
	string_copy(cheat->name, str);
	cheatsModified = 1;
}

void SystemEditCheatView::removed()
{
	cheatList.remove(*cheat);
	cheatsModified = 1;
	refreshCheatViews();
	updateCheats();
}

void SystemEditCheatView::init(bool highlightFirst, MdCheat &cheat)
{
	this->cheat = &cheat;

	uint i = 0;
	loadNameItem(cheat.name, item, i);
	ggCode.init(cheat.code); item[i++] = &ggCode;
	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

uint EditCheatListView::handleNameFromTextInput(const char *str)
{
	if(str)
	{
		MdCheat c;
		string_copy(c.name, str);
		if(!cheatList.addToEnd(c))
		{
			logErr("error adding new cheat");
			removeModalView();
			return 0;
		}
		logMsg("added new cheat, %d total", cheatList.size);
		cheatsModified = 1;
		removeModalView();
		refreshCheatViews();
		editCheatView.init(0, *cheatList.last());
		viewStack.pushAndShow(&editCheatView);
	}
	else
	{
		removeModalView();
	}
	return 0;
}

void EditCheatListView::addGGGSHandler(TextMenuItem &item, const Input::Event &e)
{
	textInputView.init("Input description");
	textInputView.onTextDelegate().bind<EditCheatListView, &EditCheatListView::handleNameFromTextInput>(this);
	textInputView.placeRect(Gfx::viewportRect());
	modalView = &textInputView;
}


void EditCheatListView::cheatSelected(uint idx, const Input::Event &e)
{
	editCheatView.init(!e.isPointer(), *cheatList.index(idx));
	viewStack.pushAndShow(&editCheatView);
}

void EditCheatListView::loadAddCheatItems(MenuItem *item[], uint &items)
{
	addGGGS.init(); item[items++] = &addGGGS;
}

void EditCheatListView::loadCheatItems(MenuItem *item[], uint &items)
{
	int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
	auto it = cheatList.iterator();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = it.obj();
		cheat[c].init(thisCheat.name); item[items++] = &cheat[c];
		it.advance();
	}
}

void CheatsView::cheatSelected(uint idx, const Input::Event &e)
{
	cheat[idx].toggle();
	auto c = cheatList.index(idx);
	c->toggleOn();
	cheatsModified = 1;
	updateCheats();
}

void CheatsView::loadCheatItems(MenuItem *item[], uint &i)
{
	int cheats = IG::min(cheatList.size, (int)sizeofArray(cheat));
	auto it = cheatList.iterator();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = it.obj();
		cheat[c].init(thisCheat.name, thisCheat.isOn()); item[i++] = &cheat[c];
		logMsg("added cheat %s : %s", thisCheat.name, thisCheat.code);
		it.advance();
	}
}
