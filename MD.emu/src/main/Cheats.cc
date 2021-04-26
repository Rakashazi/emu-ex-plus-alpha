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

#include <imagine/io/FileIO.hh>
#include <imagine/io/IOStream.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/util/mayAliasInt.h>
#include <imagine/util/ranges.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <emuframework/EmuApp.hh>
#include <main/Cheats.hh>
#include "EmuCheatViews.hh"
#include "system.h"
#include "z80.h"
#include "loadrom.h"
#include "md_cart.h"
#include "genesis.h"
StaticArrayList<MdCheat, EmuCheats::MAX> cheatList;
StaticArrayList<MdCheat*, EmuCheats::MAX> romCheatList;
StaticArrayList<MdCheat*, EmuCheats::MAX> ramCheatList;
bool cheatsModified = 0;
static const char *INPUT_CODE_8BIT_STR = "Input xxx-xxx-xxx (GG) or xxxxxx:xx (AR) code";
static const char *INPUT_CODE_16BIT_STR = "Input xxxx-xxxx (GG) or xxxxxx:xxxx (AR) code";

static bool strIs16BitGGCode(const char *str)
{
	return strlen(str) == 9 && str[4] == '-';
}

static bool strIs8BitGGCode(const char *str)
{
	return strlen(str) == 11 && str[3] == '-' && str[7] == '-';
}

static bool strIs16BitARCode(const char *str)
{
	return strlen(str) == 11 && str[6] == ':';
}

static bool strIs8BitARCode(const char *str)
{
	return strlen(str) == 9 && str[6] == ':';
}

static bool strIs8BitCode(const char *str)
{
	return strIs8BitGGCode(str) || strIs8BitARCode(str);
}

static bool strIs16BitCode(const char *str)
{
	return strIs16BitGGCode(str) || strIs16BitARCode(str);
}

// Decode cheat string into address/data components (derived from Genesis Plus GX function)
unsigned decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData)
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
	for(auto &e : cheatList)
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
        	romCheatList.push_back(&e);

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
      	ramCheatList.push_back(&e);
      }
      e.setApplied(1);
    }
  }
  if(romCheatList.size() || ramCheatList.size())
  {
  	logMsg("%zu RAM cheats, %zu ROM cheats active", ramCheatList.size(), romCheatList.size());
  }
}

void clearCheats()
{
	//logMsg("clearing cheats");
	romCheatList.clear();
	ramCheatList.clear();

	//logMsg("reversing applied cheats");
  // disable cheats in reversed order in case the same address is used by multiple patches
  for(auto &e : IG::makeReverseRange(cheatList))
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
          // check if previous banked ROM address has been patched
          if(e.prev)
          {
            // restore original data
            *e.prev = e.origData;

            // no more patched ROM address
            e.prev = nullptr;
          }
        }
      }
      e.setApplied(0);
    }
  }
  logMsg("done");
}

void clearCheatList()
{
	romCheatList.clear();
	ramCheatList.clear();
	cheatList.clear();
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

	auto filename = FS::makePathStringPrintf("%s/%s.pat", EmuSystem::savePath(), EmuSystem::gameName().data());

	if(!cheatList.size())
	{
		logMsg("deleting cheats file %s", filename.data());
		FS::remove(filename);
		cheatsModified = false;
		return;
	}

	FileIO file;
	file.create(filename.data());
	if(!file)
	{
		logMsg("error creating cheats file %s", filename.data());
		return;
	}
	logMsg("writing cheats file %s", filename.data());

	for(auto &e : cheatList)
	{
		if(!strlen(e.code))
		{
			continue; // ignore incomplete code entries
		}
		file.write(e.code, strlen(e.code));
		file.write('\t');
		file.write(e.name, strlen(e.name));
		file.write('\n');
		if(e.isOn())
		{
			file.write("ON\n", strlen("ON\n"));
		}
	}
	cheatsModified = false;
}

void readCheatFile()
{
	auto filename = FS::makePathStringPrintf("%s/%s.pat", EmuSystem::savePath(), EmuSystem::gameName().data());
	FileIO file;
	file.open(filename.data(), IO::AccessHint::ALL);
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file %s", filename.data());

	char line[256];
	IOStream<FileIO> fileStream{std::move(file), "r"};
	while(fgets(line, sizeof(line), static_cast<FILE*>(fileStream)))
	{
		logMsg("got line: %s", line);
		MdCheat cheat;
		auto items = sscanf(line, "%11s %" PP_STRINGIFY_EXP(MAX_CHEAT_NAME_CHARS) "[^\n]", cheat.code, cheat.name);

		if(items == 2) // code & name
		{
			if(cheatList.isFull())
			{
				logErr("cheat list full while reading from file");
				break;
			}
			string_toUpper(cheat.code);
			if(!decodeCheat(cheat.code, cheat.address, cheat.data, cheat.origData))
			{
				logWarn("Invalid code %s from cheat file", cheat.code);
				continue;
			}
			logMsg("read cheat %s : %s", cheat.name, cheat.code);
			cheatList.push_back(cheat);
		}
		else if(items == 1) // ON/OFF string
		{
			if(string_equal(cheat.code, "ON") && !cheatList.empty())
			{
				auto &lastCheat = cheatList.back();
				lastCheat.toggleOn();
				logMsg("turned on cheat %s from file", lastCheat.name);
			}
		}
	}
}

void RAMCheatUpdate()
{
	for(auto &e : ramCheatList)
	{
		// apply RAM patch
		if(e->data & 0xFF00)
		{
			// word patch
			*(uint16*)(work_ram + (e->address & 0xFFFE)) = e->data;
		}
		else
		{
			// byte patch
			work_ram[e->address & 0xFFFF] = e->data;
		}
	}
}

void ROMCheatUpdate()
{
	for(auto &e : romCheatList)
	{
		// check if previous banked ROM address was patched
		if (e->prev)
		{
			// restore original data
			*e->prev = e->origData;

			// no more patched ROM address
			e->prev = nullptr;
		}

		// get current banked ROM address
		auto ptr = &z80_readmap[(e->address) >> 10][e->address & 0x03FF];

		// check if reference matches original ROM data
		if (((uint8)e->origData) == *ptr)
		{
			// patch data
			*ptr = e->data;

			// save patched ROM address
			e->prev = ptr;
		}
	}
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, MdCheat &cheat_, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheat_.name,
		[this](const TableView &)
		{
			return 3;
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return name;
				case 1: return code;
				default: return remove;
			}
		},
		[this](TextMenuItem &, View &, Input::Event)
		{
			IG::eraseFirst(cheatList, *cheat);
			cheatsModified = 1;
			onCheatListChanged();
			updateCheats();
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	code
	{
		"Code",
		cheat_.code,
		&defaultFace(),
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, emuSystemIs16Bit() ? INPUT_CODE_16BIT_STR : INPUT_CODE_8BIT_STR, cheat->code,
				[this](EmuApp &, auto str)
				{
					string_copy(cheat->code, str);
					string_toUpper(cheat->code);
					if(!decodeCheat(cheat->code, cheat->address, cheat->data, cheat->origData))
					{
						cheat->code[0]= 0;
						app().postMessage(true, "Invalid code");
						postDraw();
						return false;
					}
					cheatsModified = 1;
					updateCheats();
					code.set2ndName(str);
					code.compile(renderer(), projP);
					postDraw();
					return true;
				});
		}
	},
	cheat{&cheat_}
{}

const char *EmuEditCheatView::cheatNameString() const
{
	return cheat->name;
}

void EmuEditCheatView::renamed(const char *str)
{
	string_copy(cheat->name, str);
	cheatsModified = 1;
}

void EmuEditCheatListView::loadCheatItems()
{
	unsigned cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, c)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, &defaultFace(),
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(cheatList[c], [this](){ onCheatListChanged(); }), e);
			});
		++it;
	}
}

EmuEditCheatListView::EmuEditCheatListView(ViewAttachParams attach):
	BaseEditCheatListView
	{
		attach,
		[this](const TableView &)
		{
			return 1 + cheat.size();
		},
		[this](const TableView &, unsigned idx) -> MenuItem&
		{
			switch(idx)
			{
				case 0: return addCode;
				default: return cheat[idx - 1];
			}
		}
	},
	addCode
	{
		"Add Game Genie / Action Replay Code", &defaultFace(),
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			app().pushAndShowNewCollectTextInputView(attachParams(), e, emuSystemIs16Bit() ? INPUT_CODE_16BIT_STR : INPUT_CODE_8BIT_STR, "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(cheatList.isFull())
						{
							app().postMessage(true, "Cheat list is full");
							view.dismiss();
							return 0;
						}
						MdCheat c;
						string_copy(c.code, str);
						string_toUpper(c.code);
						if(!decodeCheat(c.code, c.address, c.data, c.origData))
						{
							app().postMessage(true, "Invalid code");
							return 1;
						}
						string_copy(c.name, "Unnamed Cheat");
						cheatList.push_back(c);
						logMsg("added new cheat, %zu total", cheatList.size());
						cheatsModified = 1;
						updateCheats();
						onCheatListChanged();
						view.dismiss();
						app().pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[this](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									string_copy(cheatList.back().name, str);
									onCheatListChanged();
									view.dismiss();
								}
								else
								{
									view.dismiss();
								}
								return 0;
							});
					}
					else
					{
						view.dismiss();
					}
					return 0;
				});
		}
	}
{
	loadCheatItems();
}

EmuCheatsView::EmuCheatsView(ViewAttachParams attach): BaseCheatsView{attach}
{
	loadCheatItems();
}

void EmuCheatsView::loadCheatItems()
{
	unsigned cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	iterateTimes(cheats, cIdx)
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, &defaultFace(), thisCheat.isOn(),
			[this, cIdx](BoolMenuItem &item, View &, Input::Event e)
			{
				item.flipBoolValue(*this);
				auto &c = cheatList[cIdx];
				c.toggleOn();
				cheatsModified = 1;
				updateCheats();
			});
		logMsg("added cheat %s : %s", thisCheat.name, thisCheat.code);
		++it;
	}
}
