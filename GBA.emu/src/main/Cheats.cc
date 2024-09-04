/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/viewUtils.hh>
#include "EmuCheatViews.hh"
#include "MainSystem.hh"
#include "GBASys.hh"
#include <imagine/fs/FS.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/util/string.h>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <core/gba/gbaCheats.h>
#include <core/gba/gba.h>

void cheatsEnable(CheatsData&);
void cheatsDisable(ARM7TDMI&, CheatsData&);

namespace EmuEx
{

constexpr SystemLogger log{"GBA.emu"};

static auto matchingCheats(std::string_view name)
{
	return cheatsList | std::views::filter([=](auto& c) { return c.desc == name; });
}

static bool deleteOneCheat(std::string_view name)
{
	for(auto i: iotaCount(cheatsList.size()))
	{
		if(name == cheatsList[i].desc)
		{
			cheatsDelete(gGba.cpu, i, true);
			return true;
		}
	}
	return false;
}

static auto cheatInputString(bool isGSv3)
{
	return isGSv3 ? "Input xxxxxxxx yyyyyyyy" : "Input xxxxxxxx yyyyyyyy (GS) or xxxxxxxx yyyy (AR)";
}

static CheatsData& sortCheat(auto& cheatsList, const CheatsData& cheat)
{
	auto insertIt = find(std::views::reverse(cheatsList), [&](auto& c){ return std::string_view{c.desc} == cheat.desc; });
	if(insertIt) // store cheats with same name consecutively
	{
		return *cheatsList.insert(insertIt->base(), cheat);
	}
	else
	{
		return cheatsList.emplace_back(cheat);
	}
}

static CheatsData& sortLastCheat()
{
	auto lastCheat = cheatsList.back();
	cheatsList.pop_back();
	return sortCheat(cheatsList, lastCheat);
}

static CheatsData* addCode(EmuApp& app, const char* code, const char* name, bool isGSv3)
{
	auto tempStr{IG::toUpperCase(code)};
	if(tempStr.size() == 17 && tempStr[8] == ' ')
	{
		log.info("removing middle space in text");
		tempStr.erase(tempStr.begin() + 8);
	}
	if(isGSv3 ?
		cheatsAddGSACode(gGba.cpu, tempStr.data(), name, true) :
		((tempStr.size() == 16 && cheatsAddGSACode(gGba.cpu, tempStr.data(), name, false))
		|| cheatsAddCBACode(gGba.cpu, tempStr.data(), name)))
	{
		log.info("added new cheat, {} total", cheatsList.size());
	}
	else
	{
		app.postMessage(true, "Invalid format");
		return {};
	}
	cheatsDisable(gGba.cpu, cheatsList.size() - 1);
	return &cheatsList.back();
}

static CheatsData* addCodeSorted(EmuApp& app, const char* code, const char* name, bool isGSv3)
{
	if(!addCode(app, code, name, isGSv3))
		return {};
	return &sortLastCheat();
}

static void enableCheat(std::string_view name)
{
	for(auto& c: matchingCheats(name)) { cheatsEnable(c); };
}

static void disableCheat(ARM7TDMI& cpu, std::string_view name)
{
	for(auto& c: matchingCheats(name)) { cheatsDisable(cpu, c);  };
}

static bool cheatExists(std::string_view name)
{
	for(auto& _: matchingCheats(name)) { return true; }
	return false;
}

static std::span<Cheat> cheats()
{
	return std::span<Cheat>{static_cast<Cheat*>(cheatsList.data()), cheatsList.size()};
}

Cheat* GbaSystem::newCheat(EmuApp& app, const char* name, CheatCodeDesc code)
{
	auto newCheat = addCode(app, code.str, name, code.flags);
	if(!newCheat)
		return {};
	if(strlen(name))
		newCheat = &sortLastCheat();
	writeCheatFile();
	return static_cast<Cheat*>(newCheat);
}

bool GbaSystem::setCheatName(Cheat& cheat, const char* newName)
{
	if(cheatExists(newName))
		return false;
	auto name = std::to_array(cheat.desc);
	for(auto& c: matchingCheats(std::string_view{name.data()}))
	{
		strncpy(c.desc, newName, sizeof(c.desc));
	}
	writeCheatFile();
	return true;
}

std::string_view GbaSystem::cheatName(const Cheat& c) const { return c.desc; }

void GbaSystem::setCheatEnabled(Cheat& c, bool on)
{
	if(on)
		enableCheat(c.desc);
	else
		disableCheat(gGba.cpu, c.desc);
	writeCheatFile();
}

bool GbaSystem::isCheatEnabled(const Cheat& c) const { return c.enabled; }

bool GbaSystem::addCheatCode(EmuApp& app, Cheat*& cheatPtr, CheatCodeDesc code)
{
	auto cheatIdx = std::distance(cheats().data(), cheatPtr);
	auto name = std::to_array(cheatPtr->desc);
	bool isEnabled = cheatPtr->enabled;
	auto codePtr = addCodeSorted(app, code.str, name.data(), code.flags);
	if(!codePtr)
		return false;
	cheatPtr = &cheats()[cheatIdx];
	if(isEnabled)
		cheatsEnable(*codePtr);
	writeCheatFile();
	return true;
}

Cheat* GbaSystem::removeCheatCode(Cheat& cheat, CheatCode& code)
{
	cheatsDisable(gGba.cpu, code);
	auto name = std::to_array(cheat.desc);
	cheatsDelete(gGba.cpu, std::distance(static_cast<CheatCode*>(cheatsList.data()), &code), true);
	writeCheatFile();
	for(auto& c: matchingCheats(std::string_view{name.data()})) { return static_cast<Cheat*>(&c); }
	return {};
}

bool GbaSystem::removeCheat(Cheat& cheat)
{
	forEachCheatCode(cheat, [&](CheatCode& code, std::string_view){ cheatsDisable(gGba.cpu, code); return true; });
	bool didDelete{};
	auto name = std::to_array(cheat.desc);
	while(deleteOneCheat(std::string_view{name.data()})) { didDelete = true; }
	writeCheatFile();
	return didDelete;
}

void GbaSystem::forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)> del)
{
	std::string_view lastName{};
	for(auto& c: cheatsList)
	{
		if(lastName == c.desc)
			continue;
		lastName = c.desc;
		if(!del(static_cast<Cheat&>(c), std::string_view{c.desc}))
			break;
	}
}

void GbaSystem::forEachCheatCode(Cheat& c, DelegateFunc<bool(CheatCode&, std::string_view)> del)
{
	for(auto& c: matchingCheats(c.desc))
	{
		if(!del(static_cast<CheatCode&>(c), std::string_view{c.desc}))
			break;
	}
}

void GbaSystem::readCheatFile()
{
	auto filename = userFilePath(cheatsDir, ".clt");
	if(cheatsLoadCheatList(appContext(), filename.data()))
	{
		log.info("loaded cheat file:{}", filename);
		std::vector<CheatsData> groupedCheatsList;
		groupedCheatsList.reserve(cheatsList.size());
		for(auto& c: cheatsList) { sortCheat(groupedCheatsList, c); }
		cheatsList = std::move(groupedCheatsList);
		//for(auto& c: cheatsList) { log.debug("cheat:{} {}", c.desc, c.codestring); }
	}
}

void GbaSystem::writeCheatFile()
{
	auto ctx = appContext();
	auto filename = userFilePath(cheatsDir, ".clt");
	if(cheatsList.empty())
	{
		log.info("deleting cheats file:{}", filename);
		ctx.removeFileUri(filename);
		return;
	}
	cheatsSaveCheatList(ctx, filename.data());
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
	addGS12CBCode
	{
		"Add Another Game Shark v1-2/Code Breaker Code", attach,
		[this](const Input::Event& e) { addNewCheatCode(cheatInputString(false), e, 0); }
	},
	addGS3Code
	{
		"Add Another Game Shark v3 Code", attach,
		[this](const Input::Event& e) { addNewCheatCode(cheatInputString(true), e, 1); }
	}
{
	loadItems();
}

void EditCheatView::loadItems()
{
	codes.clear();
	system().forEachCheatCode(*cheatPtr, [this](CheatCode& c, std::string_view code)
	{
		codes.emplace_back("Code", c.codestring, attachParams(), [this, &c](const Input::Event& e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really delete this code?",
				YesNoAlertView::Delegates{.onYes = [this, &c]{ removeCheatCode(c); }}), e);
		});
		return true;
	});
	items.clear();
	items.emplace_back(&name);
	for(auto& c: codes)
	{
		items.emplace_back(&c);
	}
	items.emplace_back(&addGS12CBCode);
	items.emplace_back(&addGS3Code);
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
				[&](const ItemsMessage &m) -> ItemReply { return 2 + cheats.size(); },
				[&](const GetItemMessage &m) -> ItemReply
				{
					switch(m.idx)
					{
						case 0: return &addGS12CBCode;
						case 1: return &addGS3Code;
						default: return &cheats[m.idx - 2];
					}
				},
			});
		}
	},
	addGS12CBCode
	{
		"Add Game Shark v1-2/Code Breaker Code", attach,
		[this](const Input::Event& e) { addNewCheat(cheatInputString(false), e, 0); }
	},
	addGS3Code
	{
		"Add Game Shark v3 Code", attach,
		[this](const Input::Event& e) { addNewCheat(cheatInputString(true), e, 1); }
	} {}

}
