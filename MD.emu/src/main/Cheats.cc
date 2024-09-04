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

constexpr SystemLogger log{"MD.emu"};

static auto codePromptString()
{
	return emuSystemIs16Bit() ? "Input xxxx-xxxx (GG) or xxxxxx:xxxx (AR) code"
		: "Input xxx-xxx-xxx (GG) or xxxxxx:xx (AR) code";
}

static auto editCodePromptString()
{
	return emuSystemIs16Bit() ? "Input xxxx-xxxx (GG) or xxxxxx:xxxx (AR) code, or blank to delete"
		: "Input xxx-xxx-xxx (GG) or xxxxxx:xx (AR) code, or blank to delete";
}

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

void MdSystem::applyCheats()
{
	for(auto &e : cheatList)
  {
  	assert(!e.applied); // make sure cheats have been cleared beforehand
    if(e.on)
    {
    	for(auto& code: e.codes)
    	{
				logMsg("applying cheat:%s", e.name.data());
				if(code.address < cart.romsize)
				{
					if(emuSystemIs16Bit())
					{
						// patch ROM data
						code.origData = *(uint16a*)(cart.rom + (code.address & 0xFFFFFE));
						*(uint16a*)(cart.rom + (code.address & 0xFFFFFE)) = code.data;
					}
					else
					{
						// add ROM patch
						romCheatList.push_back(&code);

						// get current banked ROM address
						auto ptr = &z80_readmap[(code.address) >> 10][code.address & 0x03FF];

						/* check if reference matches original ROM data */
						if (((uint8)code.origData) == *ptr)
						{
							/* patch data */
							*ptr = code.data;

							/* save patched ROM address */
							code.prev = ptr;
						}
						else
						{
							/* no patched ROM address yet */
							code.prev = nullptr;
						}
					}
				}
				else if(code.address >= 0xFF0000)
				{
					// add RAM patch
					ramCheatList.push_back(&code);
				}
    	}
			e.applied = 1;
    }
  }
  if(romCheatList.size() || ramCheatList.size())
  {
  	logMsg("%zu RAM cheats, %zu ROM cheats active", ramCheatList.size(), romCheatList.size());
  }
}

void MdSystem::clearCheats()
{
	//logMsg("clearing cheats");
	romCheatList.clear();
	ramCheatList.clear();

	//logMsg("reversing applied cheats");
  // disable cheats in reversed order in case the same address is used by multiple patches
  for(auto &e : std::views::reverse(cheatList))
  {
    if(e.applied)
    {
    	log.info("clearing cheat:{}", e.name);
    	for(auto& code: e.codes)
    	{
				if(code.address < cart.romsize)
				{
					if(emuSystemIs16Bit())
					{
						// restore original ROM data
						*(uint16a*)(cart.rom + (code.address & 0xFFFFFE)) = code.origData;
					}
					else
					{
						// check if previous banked ROM address has been patched
						if(code.prev)
						{
							// restore original data
							*code.prev = code.origData;

							// no more patched ROM address
							code.prev = nullptr;
						}
					}
				}
    	}
      e.applied = 0;
    }
  }
  logMsg("done");
}

void MdSystem::clearCheatList()
{
	romCheatList.clear();
	ramCheatList.clear();
	cheatList.clear();
}

void MdSystem::updateCheats()
{
	clearCheats();
	applyCheats();
}

void MdSystem::writeCheatFile()
{
	auto ctx = appContext();
	auto path = userFilePath(cheatsDir, ".pat");

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
		for(auto& code: e.codes)
		{
			if(code.text.empty())
			{
				continue; // ignore incomplete code entries
			}
			file.write(code.text.data(), code.text.size());
			file.put('\t');
			file.write(e.name.data(), e.name.size());
			file.put('\n');
		}
		if(e.on)
		{
			file.write("ON\n", strlen("ON\n"));
		}
	}
}

static Cheat& prepareCheat(auto& cheatList, std::string_view name)
{
	auto cheatIt = find(cheatList, [&](auto& c){ return name == c.name; });
	if(cheatIt)
		return **cheatIt;
	return cheatList.emplace_back(Cheat{.name = StaticString<MAX_CHEAT_NAME_CHARS>{name}});
}

void MdSystem::readCheatFile()
{
	auto path = userFilePath(cheatsDir, ".pat");
	auto file = appContext().openFileUri(path, {.test = true, .accessHint = IOAccessHint::All});
	if(!file)
	{
		return;
	}
	log.info("reading cheats file:{}", path);
	char line[256];
	IG::FileStream<FileIO> fileStream{std::move(file), "r"};
	while(fgets(line, sizeof(line), fileStream.filePtr()))
	{
		log.info("got line:{}", line);
		std::array<char, MAX_CHEAT_NAME_CHARS + 1> tempName{};
		std::array<char, 12> tempCode{};
		auto items = sscanf(line, "%11s %" PP_STRINGIFY_EXP(MAX_CHEAT_NAME_CHARS) "[^\n]", tempCode.data(), tempName.data());

		if(items == 2) // code & name
		{
			auto& cheat = prepareCheat(cheatList, tempName.data());
			auto& code = cheat.codes.emplace_back(toUpperCase<CheatCodeString>(tempCode.data()));
			if(!decodeCheat(tempCode.data(), code.address, code.data, code.origData))
			{
				log.warn("Invalid code:{} from cheat file", tempCode.data());
				continue;
			}
			log.info("read cheat {}:{}", cheat.name, tempCode.data());
		}
		else if(items == 1) // ON/OFF string
		{
			if(std::string_view{tempCode.data()} == "ON" && !cheatList.empty())
			{
				auto &lastCheat = cheatList.back();
				lastCheat.on = true;
				log.info("turned on cheat:{} from file", lastCheat.name);
			}
		}
	}
}

void MdSystem::RAMCheatUpdate()
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

void MdSystem::ROMCheatUpdate()
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

Cheat* MdSystem::newCheat(EmuApp& app, const char* name, CheatCodeDesc desc)
{
	if(strlen(desc.str) > 11)
	{
		app.postMessage(true, "Code is too long");
		return {};
	}
	CheatCode code{toUpperCase<CheatCodeString>(desc.str)};
	if(!decodeCheat(code.text.data(), code.address, code.data, code.origData))
	{
		app.postMessage(true, "Invalid code");
		return {};
	}
	auto& c = cheatList.emplace_back(Cheat{.name = name});
	c.codes.emplace_back(std::move(code));
	log.info("added new cheat, {} total", cheatList.size());
	writeCheatFile();
	updateCheats();
	return &c;
}

bool MdSystem::setCheatName(Cheat& c, const char* name)
{
	c.name = name;
	writeCheatFile();
	return true;
}

std::string_view MdSystem::cheatName(const Cheat& c) const { return c.name; }

void MdSystem::setCheatEnabled(Cheat& c, bool on)
{
	c.on = on;
	writeCheatFile();
	updateCheats();
}

bool MdSystem::isCheatEnabled(const Cheat& c) const { return c.on; }

bool MdSystem::addCheatCode(EmuApp& app, Cheat*& cheatPtr, CheatCodeDesc desc)
{
	CheatCode code{toUpperCase<CheatCodeString>(desc.str)};
	if(!decodeCheat(code.text.data(), code.address, code.data, code.origData))
	{
		app.postMessage(true, "Invalid code");
		return {};
	}
	cheatPtr->codes.emplace_back(std::move(code));
	writeCheatFile();
	updateCheats();
	return true;
}

bool MdSystem::modifyCheatCode(EmuApp& app, Cheat&, CheatCode& c, CheatCodeDesc desc)
{
	auto codeStr = toUpperCase<CheatCodeString>(desc.str);
	if(!decodeCheat(codeStr.data(), c.address, c.data, c.origData))
	{
		app.postMessage(true, "Invalid code");
		return false;
	}
	c.text = codeStr;
	writeCheatFile();
	updateCheats();
	return true;
}

Cheat* MdSystem::removeCheatCode(Cheat& c, CheatCode& code)
{
	eraseFirst(c.codes, code);
	bool removedAllCodes = c.codes.empty();
	if(removedAllCodes)
		eraseFirst(cheatList, c);
	writeCheatFile();
	updateCheats();
	return removedAllCodes ? nullptr : &c;
}

bool MdSystem::removeCheat(Cheat& c)
{
	eraseFirst(cheatList, c);
	writeCheatFile();
	updateCheats();
	return true;
}

void MdSystem::forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)> del)
{
	for(auto& c: cheatList)
	{
		if(!del(c, std::string_view{c.name}))
			break;
	}
}

void MdSystem::forEachCheatCode(Cheat& cheat, DelegateFunc<bool(CheatCode&, std::string_view)> del)
{
	for(auto& c: cheat.codes)
	{
		if(!del(c, std::string_view{c.text}))
			break;
	}
}

EditCheatView::EditCheatView(ViewAttachParams attach, Cheat& cheat, BaseEditCheatsView& editCheatsView):
	BaseEditCheatView
	{
		"Edit Cheat",
		attach,
		cheat,
		editCheatsView,
		items
	},
	addCode
	{
		"Add Another Game Genie / Action Replay Code", attach,
		[this](const Input::Event& e) { addNewCheatCode(codePromptString(), e); }
	}
{
	loadItems();
}

void EditCheatView::loadItems()
{
	codes.clear();
	for(auto& c: cheatPtr->codes)
	{
		codes.emplace_back("Code", c.text, attachParams(), [this, &c](const Input::Event& e)
		{
			pushAndShowNewCollectValueInputView<const char*, ScanValueMode::AllowBlank>(attachParams(), e, editCodePromptString(), c.text,
				[this, &c](CollectTextInputView&, auto str) { return modifyCheatCode(c, {str}); });
		});
	};
	items.clear();
	items.emplace_back(&name);
	for(auto& c: codes)
	{
		items.emplace_back(&c);
	}
	items.emplace_back(&addCode);
	items.emplace_back(&remove);
}

EditCheatsView::EditCheatsView(ViewAttachParams attach, CheatsView& cheatsView):
	BaseEditCheatsView
	{
		attach,
		cheatsView,
		[this](ItemMessage msg) -> ItemReply
		{
			return msg.visit(overloaded
			{
				[&](const ItemsMessage &m) -> ItemReply { return 1 + cheats.size(); },
				[&](const GetItemMessage &m) -> ItemReply
				{
					switch(m.idx)
					{
						case 0: return &addCode;
						default: return &cheats[m.idx - 1];
					}
				},
			});
		}
	},
	addCode
	{
		"Add Game Genie / Action Replay Code", attach,
		[this](const Input::Event& e) { addNewCheat(codePromptString(), e); }
	} {}

}

void ROMCheatUpdate()
{
	static_cast<EmuEx::MdSystem&>(EmuEx::gSystem()).ROMCheatUpdate();
}
