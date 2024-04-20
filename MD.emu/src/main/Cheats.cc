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

#include <emuframework/EmuApp.hh>
#include <emuframework/viewUtils.hh>
#include <main/Cheats.hh>
#include <z80.hh>
#include "EmuCheatViews.hh"
#include "MainSystem.hh"
#include <imagine/io/FileIO.hh>
#include <imagine/io/FileStream.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/mayAliasInt.h>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "system.h"
#include "loadrom.h"
#include "md_cart.h"
#include "genesis.h"
#include <ranges>

namespace EmuEx
{

StaticArrayList<MdCheat, maxCheats> cheatList;
StaticArrayList<MdCheat*, maxCheats> romCheatList;
StaticArrayList<MdCheat*, maxCheats> ramCheatList;
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

		for(auto i : iotaCount(8))
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
		for(auto i : iotaCount(2))
		{
			auto p = strchr(arvalidchars, *string++);
			if(!p) return 0;
			auto n = (p - arvalidchars) & 0xF;
			data |= (n  << ((1 - i) * 4));
		}

		// decode 16-bit address (low 12-bits)
		for(auto i : iotaCount(3))
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
		for(auto i : iotaCount(2))
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
			for(auto i : iotaCount(6))
			{
				auto p = strchr(arvalidchars, *string++);
				if(!p) return 0;
				auto n = (p - arvalidchars) & 0xF;
				address |= (n << ((5 - i) * 4));
			}

			// decode 16-bit data
			string++;
			for(auto i : iotaCount(4))
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
			for(auto i : iotaCount(4))
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
			for(auto i : iotaCount(2))
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
    if(e.isOn() && e.code.size())
    {
    	logMsg("applying cheat:%s", e.name.data());
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
  for(auto &e : std::views::reverse(cheatList))
  {
    if(e.isApplied())
    {
    	logMsg("clearing cheat:%s", e.name.data());
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

void writeCheatFile(EmuSystem &sys_)
{
	auto &sys = static_cast<MdSystem&>(sys_);
	auto ctx = sys.appContext();
	auto path = sys.userFilePath(sys.cheatsDir, ".pat");

	if(!cheatList.size())
	{
		logMsg("deleting cheats file %s", path.data());
		ctx.removeFileUri(path);
		return;
	}

	auto file = ctx.openFileUri(path, OpenFlags::testNewFile());
	if(!file)
	{
		logMsg("error creating cheats file %s", path.data());
		return;
	}
	logMsg("writing cheats file %s", path.data());

	for(auto &e : cheatList)
	{
		if(!e.code.size())
		{
			continue; // ignore incomplete code entries
		}
		file.write(e.code.data(), e.code.size());
		file.put('\t');
		file.write(e.name.data(), e.name.size());
		file.put('\n');
		if(e.isOn())
		{
			file.write("ON\n", strlen("ON\n"));
		}
	}
}

void readCheatFile(EmuSystem &sys_)
{
	auto &sys = static_cast<MdSystem&>(sys_);
	auto path = sys.userFilePath(sys.cheatsDir, ".pat");
	auto file = sys.appContext().openFileUri(path, {.test = true, .accessHint = IOAccessHint::All});
	if(!file)
	{
		return;
	}
	logMsg("reading cheats file %s", path.data());
	char line[256];
	IG::FileStream<FileIO> fileStream{std::move(file), "r"};
	while(fgets(line, sizeof(line), fileStream.filePtr()))
	{
		logMsg("got line: %s", line);
		MdCheat cheat{};
		std::array<char, 12> tempCode{};
		cheat.name.resize(MAX_CHEAT_NAME_CHARS);
		auto items = sscanf(line, "%11s %" PP_STRINGIFY_EXP(MAX_CHEAT_NAME_CHARS) "[^\n]", tempCode.data(), cheat.name.data());
		cheat.name.resize(strlen(cheat.name.data()));
		cheat.code = IG::toUpperCase<decltype(cheat.code)>(tempCode.data());

		if(items == 2) // code & name
		{
			if(cheatList.isFull())
			{
				logErr("cheat list full while reading from file");
				break;
			}
			if(!decodeCheat(cheat.code.data(), cheat.address, cheat.data, cheat.origData))
			{
				logWarn("Invalid code %s from cheat file", cheat.code.data());
				continue;
			}
			logMsg("read cheat %s : %s", cheat.name.data(), cheat.code.data());
			cheatList.push_back(cheat);
		}
		else if(items == 1) // ON/OFF string
		{
			if(cheat.code == "ON" && !cheatList.empty())
			{
				auto &lastCheat = cheatList.back();
				lastCheat.toggleOn();
				logMsg("turned on cheat %s from file", lastCheat.name.data());
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

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, MdCheat &cheat_, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Code",
		attach,
		cheat_.name,
		items,
		[this](TextMenuItem &, View &, Input::Event)
		{
			IG::eraseFirst(cheatList, *cheat);
			onCheatListChanged();
			updateCheats();
			writeCheatFile(system());
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	items{&name, &code, &remove},
	code
	{
		"Code",
		cheat_.code,
		attach,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, emuSystemIs16Bit() ? INPUT_CODE_16BIT_STR : INPUT_CODE_8BIT_STR, cheat->code,
				[this](CollectTextInputView&, auto str)
				{
					cheat->code = IG::toUpperCase<decltype(cheat->code)>(str);
					if(!decodeCheat(cheat->code.data(), cheat->address, cheat->data, cheat->origData))
					{
						cheat->code.clear();
						app().postMessage(true, "Invalid code");
						postDraw();
						return false;
					}
					updateCheats();
					writeCheatFile(system());
					code.set2ndName(str);
					code.compile();
					postDraw();
					return true;
				});
		}
	},
	cheat{&cheat_}
{}

std::string_view EmuEditCheatView::cheatNameString() const
{
	return std::string_view{cheat->name};
}

void EmuEditCheatView::renamed(std::string_view str)
{
	cheat->name = str;
	writeCheatFile(system());
}

void EmuEditCheatListView::loadCheatItems()
{
	auto cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	for(auto c : iotaCount(cheats))
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, attachParams(),
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
		[this](ItemMessage msg) -> ItemReply
		{
			return visit(overloaded
			{
				[&](const ItemsMessage &m) -> ItemReply { return 1 + cheat.size(); },
				[&](const GetItemMessage &m) -> ItemReply
				{
					switch(m.idx)
					{
						case 0: return &addCode;
						default: return &cheat[m.idx - 1];
					}
				},
			}, msg);
		}
	},
	addCode
	{
		"Add Game Genie / Action Replay Code", attach,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectTextInputView(attachParams(), e, emuSystemIs16Bit() ? INPUT_CODE_16BIT_STR : INPUT_CODE_8BIT_STR, "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(cheatList.isFull())
						{
							app().postMessage(true, "Cheat list is full");
							view.dismiss();
							return false;
						}
						if(strlen(str) > 11)
						{
							app().postMessage(true, "Code is too long");
							return true;
						}
						MdCheat c;
						c.code = IG::toUpperCase<decltype(c.code)>(str);
						if(!decodeCheat(c.code.data(), c.address, c.data, c.origData))
						{
							app().postMessage(true, "Invalid code");
							return true;
						}
						c.name = "Unnamed Cheat";
						cheatList.push_back(c);
						logMsg("added new cheat, %zu total", cheatList.size());
						updateCheats();
						onCheatListChanged();
						writeCheatFile(system());
						view.dismiss();
						pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[this](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									cheatList.back().name = str;
									onCheatListChanged();
									view.dismiss();
								}
								else
								{
									view.dismiss();
								}
								return false;
							});
					}
					else
					{
						view.dismiss();
					}
					return false;
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
	auto cheats = cheatList.size();
	cheat.clear();
	cheat.reserve(cheats);
	auto it = cheatList.begin();
	for(auto cIdx : iotaCount(cheats))
	{
		auto &thisCheat = *it;
		cheat.emplace_back(thisCheat.name, attachParams(), thisCheat.isOn(),
			[this, cIdx](BoolMenuItem &item, View &, Input::Event e)
			{
				item.flipBoolValue(*this);
				auto &c = cheatList[cIdx];
				c.toggleOn();
				updateCheats();
				writeCheatFile(system());
			});
		logMsg("added cheat %s : %s", thisCheat.name.data(), thisCheat.code.data());
		++it;
	}
}

}

void ROMCheatUpdate()
{
	for(auto &e : EmuEx::romCheatList)
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
