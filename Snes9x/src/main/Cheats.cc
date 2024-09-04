#include <imagine/gui/TextEntry.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <emuframework/Cheats.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/viewUtils.hh>
#include "EmuCheatViews.hh"
#include "MainSystem.hh"
#include <cheats.h>
#include <imagine/util/format.hh>

void S9xEnableCheat(SCheat&);
void S9xDisableCheat(SCheat&);
SCheat S9xTextToCheat(const std::string&);
std::string S9xCheatToText(const SCheat&);

namespace EmuEx
{

constexpr SystemLogger log{"Cheats"};

static unsigned parseHex(const char* str) { return strtoul(str, nullptr, 16); }

int numCheats()
{
	#ifndef SNES9X_VERSION_1_4
	return ::Cheat.group.size();
	#else
	return ::Cheat.num_cheats;
	#endif
}

static FS::PathString cheatsFilename(Snes9xSystem& sys)
{
	return sys.userFilePath(sys.cheatsDir, ".cht");
}

void Snes9xSystem::writeCheatFile()
{
	if(!numCheats())
		logMsg("no cheats present, removing .cht file if present");
	else
		logMsg("saving %u cheat(s)", numCheats());
	S9xSaveCheatFile(cheatsFilename(*this).data());
}

static void setCheatCodeEnabled(CheatCode& c, bool on)
{
	if(on)
		S9xEnableCheat(c);
	else
		S9xDisableCheat(c);
}

static bool tryDisableCheatCode(CheatCode& c)
{
	bool isEnabled = c.enabled;
	S9xDisableCheat(c);
	return isEnabled;
}

static void setCheatAddress(CheatCode& cheat, uint32_t a)
{
	auto isEnabled = tryDisableCheatCode(cheat);
	cheat.address = a;
	setCheatCodeEnabled(cheat, isEnabled);
	static_cast<Snes9xSystem&>(EmuEx::gSystem()).writeCheatFile();
}

static void setCheatValue(CheatCode& cheat, uint8 v)
{
	auto isEnabled = tryDisableCheatCode(cheat);
	cheat.byte = v;
	setCheatCodeEnabled(cheat, isEnabled);
	static_cast<Snes9xSystem&>(EmuEx::gSystem()).writeCheatFile();
}

static void setCheatConditionalValue(CheatCode& cheat, bool conditional, uint8 v)
{
	auto isEnabled = tryDisableCheatCode(cheat);
	#ifndef SNES9X_VERSION_1_4
	cheat.conditional = conditional;
	cheat.cond_byte = v;
	#else
	cheat.saved = conditional;
	cheat.saved_byte = v;
	#endif
	setCheatCodeEnabled(cheat, isEnabled);
	static_cast<Snes9xSystem&>(EmuEx::gSystem()).writeCheatFile();
}

static void setCheatConditionalValue(CheatCode& cheat, int v)
{
	if(v >= 0 && v <= 0xFF)
	{
		setCheatConditionalValue(cheat, true, uint8(v));
	}
	else
	{
		setCheatConditionalValue(cheat, false, 0u);
	}
}

static std::pair<bool, uint8> cheatConditionalValue(CheatCode& c)
{
	#ifndef SNES9X_VERSION_1_4
	return {c.conditional, c.cond_byte};
	#else
	return {c.saved, c.saved_byte};
	#endif
}

static std::string codeConditionalToString(CheatCode& c)
{
	auto [cond, byte] = cheatConditionalValue(c);
	return cond ? std::format("{:x}", byte) : std::string{};
}

Cheat* Snes9xSystem::newCheat(EmuApp& app, const char* name, CheatCodeDesc desc)
{
	#ifndef SNES9X_VERSION_1_4
	if(S9xAddCheatGroup(name, desc.str) == -1)
	{
		app.postMessage(true, "Invalid code");
		return {};
	}
	log.info("added new cheat, {} total", ::Cheat.group.size());
	writeCheatFile();
	return static_cast<Cheat*>(&::Cheat.group.back());
	#else
	uint8 byte;
	uint32 address;
	uint8 bytes[3];
	bool8 sram;
	uint8 numBytes;
	if(!S9xGameGenieToRaw(desc.str, address, byte))
	{
		S9xAddCheat(false, true, address, byte);
		return static_cast<Cheat*>(&::Cheat.c[numCheats() - 1]);
	}
	else if(!S9xProActionReplayToRaw (desc.str, address, byte))
	{
		S9xAddCheat(false, true, address, byte);
		return static_cast<Cheat*>(&::Cheat.c[numCheats() - 1]);
	}
	else if(!S9xGoldFingerToRaw(desc.str, address, sram, numBytes, bytes))
	{
		for(auto i : iotaCount(numBytes))
			S9xAddCheat(false, true, address + i, bytes[i]);
		// TODO: handle cheat names for multiple codes added at once
		return static_cast<Cheat*>(&::Cheat.c[numCheats() - 1]);
	}
	return {};
	#endif
}

bool Snes9xSystem::setCheatName(Cheat& c, const char* name)
{
	c.name = name;
	writeCheatFile();
	return true;
}

std::string_view Snes9xSystem::cheatName(const Cheat& c) const { return c.name; }

void Snes9xSystem::setCheatEnabled(Cheat& c, bool on)
{
	#ifndef SNES9X_VERSION_1_4
	c.enabled = on;
  for(auto& c :c.cheat)
  {
  	setCheatCodeEnabled(static_cast<CheatCode&>(c), on);
  }
	#else
  auto idx = std::distance(::Cheat.c, static_cast<SCheat*>(&c));
	if(on)
		S9xEnableCheat(idx);
	else
		S9xDisableCheat(idx);
	#endif
	writeCheatFile();
}

bool Snes9xSystem::isCheatEnabled(const Cheat& c) const { return c.enabled; }

bool Snes9xSystem::addCheatCode(EmuApp& app, Cheat*& cheatPtr, CheatCodeDesc desc)
{
	#ifndef SNES9X_VERSION_1_4
	SCheat newCheat = S9xTextToCheat(desc.str);
	if(!newCheat.address)
	{
		app.postMessage(true, "Invalid code");
		return {};
	}
	newCheat.enabled = cheatPtr->enabled;
	setCheatCodeEnabled(static_cast<CheatCode&>(newCheat), cheatPtr->enabled);
	cheatPtr->cheat.emplace_back(newCheat);
	writeCheatFile();
	return true;
	#else
	return false;
	#endif
}

Cheat* Snes9xSystem::removeCheatCode(Cheat& c, CheatCode& code)
{
	#ifndef SNES9X_VERSION_1_4
	S9xDisableCheat(code);
	c.cheat.erase(toIterator(c.cheat, static_cast<SCheat&>(code)));
	bool removedAllCodes = c.cheat.empty();
	if(removedAllCodes)
		::Cheat.group.erase(toIterator(::Cheat.group, static_cast<SCheatGroup&>(c)));
	writeCheatFile();
	return removedAllCodes ? nullptr : &c;
	#else
	return nullptr;
	#endif
}

bool Snes9xSystem::removeCheat(Cheat& c)
{
	#ifndef SNES9X_VERSION_1_4
	S9xDeleteCheatGroup(std::distance(::Cheat.group.data(), static_cast<SCheatGroup*>(&c)));
	#else
	S9xDeleteCheat(std::distance(::Cheat.c, static_cast<SCheat*>(&c)));
	#endif
	writeCheatFile();
	return true;
}

void Snes9xSystem::forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)> del)
{
	#ifndef SNES9X_VERSION_1_4
	for(auto& c: ::Cheat.group)
	#else
	for(auto& c: ::Cheat.c | std::views::take(::Cheat.num_cheats))
	#endif
	{
		if(!del(static_cast<Cheat&>(c), c.name))
			break;
	}
}

void Snes9xSystem::forEachCheatCode(Cheat& cheat, DelegateFunc<bool(CheatCode&, std::string_view)> del)
{
	#ifndef SNES9X_VERSION_1_4
	for(auto& c: cheat.cheat)
	{
		if(!del(static_cast<CheatCode&>(c), S9xCheatToText(c)))
			break;
	}
	#endif
}

EditRamCheatView::EditRamCheatView(ViewAttachParams attach, Cheat& cheat_, CheatCode& code_, EditCheatView& editCheatView_):
	TableView
	{
		"Edit RAM Patch",
		attach,
		[this](ItemMessage msg) -> ItemReply
		{
			return msg.visit(overloaded
			{
				[&](const ItemsMessage&) -> ItemReply { return 4u; },
				[&](const GetItemMessage& m) -> ItemReply
				{
					switch(m.idx)
					{
						case 0: return &addr;
						case 1: return &value;
						case 2: return &conditional;
						case 3: return &remove;
						default: std::unreachable();
					}
				},
			});
		}
	},
	cheat{cheat_},
	code{code_},
	editCheatView{editCheatView_},
	addr
	{
		"Address",
		std::format("{:x}", code_.address),
		attach,
		[this](const Input::Event& e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 6-digit hex", std::format("{:x}", code.address),
				[this](CollectTextInputView&, auto str)
				{
					unsigned a = parseHex(str);
					if(a > 0xFFFFFF)
					{
						app().postMessage(true, "value must be <= FFFFFF");
						return false;
					}
					setCheatAddress(code, a);
					addr.set2ndName(str);
					addr.place();
					editCheatView.loadItems();
					return true;
				});
		}
	},
	value
	{
		"Value",
		std::format("{:x}", code_.byte),
		attach,
		[this](const Input::Event& e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 2-digit hex", std::format("{:x}", code.byte),
				[this](CollectTextInputView&, auto str)
				{
					unsigned a = parseHex(str);
					if(a > 0xFF)
					{
						app().postMessage(true, "value must be <= FF");
						return false;
					}
					setCheatValue(code, a);
					value.set2ndName(str);
					value.place();
					editCheatView.loadItems();
					return true;
				});
		}
	},
	conditional
	{
		#ifndef SNES9X_VERSION_1_4
		"Conditional Value",
		#else
		"Saved Value",
		#endif
		codeConditionalToString(code_),
		attach,
		[this](const Input::Event& e)
		{
			pushAndShowNewCollectValueInputView<const char*, ScanValueMode::AllowBlank>(attachParams(), e, "Input 2-digit hex or blank", codeConditionalToString(code),
				[this](CollectTextInputView &, const char *str)
				{
					int a = -1;
					if(strlen(str))
					{
						a = parseHex(str);
						if(a > 0xFF)
						{
							app().postMessage(true, "value must be <= FF");
							return true;
						}
					}
					setCheatConditionalValue(code, a);
					conditional.set2ndName(str);
					conditional.place();
					editCheatView.loadItems();
					return true;
				});
		}
	},
	remove
	{
		"Delete", attach,
		[this](const Input::Event& e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really delete this patch?",
				YesNoAlertView::Delegates{.onYes = [this]{ editCheatView.removeCheatCode(code); dismiss(); }}), e);
		}
	} {}

EditCheatView::EditCheatView(ViewAttachParams attach, Cheat& cheat, BaseEditCheatsView& editCheatsView):
	BaseEditCheatView
	{
		"Edit Cheat",
		attach,
		cheat,
		editCheatsView,
		items
	}
	#ifndef SNES9X_VERSION_1_4
	,addCode
	{
		"Add Another Game Genie/Action Replay/Gold Finger Code", attach,
		[this](const Input::Event& e) { addNewCheatCode("Input xxxx-xxxx (GG), xxxxxxxx (AR), GF code, or blank", e); }
	}
	#endif
{
	loadItems();
}

void EditCheatView::loadItems()
{
	codes.clear();
	system().forEachCheatCode(*cheatPtr, [this](CheatCode& c, std::string_view code)
	{
		codes.emplace_back("Code", code, attachParams(), [this, &c](const Input::Event& e)
		{
			pushAndShow(makeView<EditRamCheatView>(*cheatPtr, c, *this), e);
		});
		return true;
	});
	items.clear();
	items.emplace_back(&name);
	for(auto& c: codes)
	{
		items.emplace_back(&c);
	}
	#ifndef SNES9X_VERSION_1_4
	items.emplace_back(&addCode);
	#endif
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
				[&](const ItemsMessage&) -> ItemReply { return 1 + cheats.size(); },
				[&](const GetItemMessage& m) -> ItemReply
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
		"Add Game Genie/Action Replay/Gold Finger Code", attach,
		[this](const Input::Event& e) { addNewCheat("Input xxxx-xxxx (GG), xxxxxxxx (AR), or GF code", e); }
	} {}

}
