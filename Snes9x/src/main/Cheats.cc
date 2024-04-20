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

namespace EmuEx
{

int numCheats()
{
	#ifndef SNES9X_VERSION_1_4
	return Cheat.group.size();
	#else
	return Cheat.num_cheats;
	#endif
}

static void setCheatName(int idx, std::string_view name)
{
	#ifndef SNES9X_VERSION_1_4
	if(idx >= numCheats())
		return;
	Cheat.group[idx].name = name;
	#else
	strncpy(Cheat.c[idx].name, name.data(), sizeof(SCheat::name));
	#endif
}

static const char *cheatName(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	return Cheat.group[idx].name.c_str();
	#else
	return Cheat.c[idx].name;
	#endif
}

static void deleteCheat(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	S9xDeleteCheatGroup(idx);
	#else
	S9xDeleteCheat(idx);
	#endif
}

static bool cheatIsEnabled(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	return Cheat.group[idx].enabled;
	#else
	return Cheat.c[idx].enabled;
	#endif
}

static void enableCheat(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	S9xEnableCheatGroup(idx);
	#else
	S9xEnableCheat(idx);
	#endif
}

static void disableCheat(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	S9xDisableCheatGroup(idx);
	#else
	S9xDisableCheat(idx);
	#endif
}

static void setCheatAddress(int idx, uint32_t a)
{
	#ifndef SNES9X_VERSION_1_4
	Cheat.group[idx].cheat[0].address = a;
	#else
	Cheat.c[idx].address = a;
	#endif
}

static uint32_t cheatAddress(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	return Cheat.group[idx].cheat[0].address;
	#else
	return Cheat.c[idx].address;
	#endif
}

static void setCheatValue(int idx, uint8 v)
{
	#ifndef SNES9X_VERSION_1_4
	Cheat.group[idx].cheat[0].byte = v;
	#else
	Cheat.c[idx].byte = v;
	#endif
}

static uint8 cheatValue(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	return Cheat.group[idx].cheat[0].byte;
	#else
	return Cheat.c[idx].byte;
	#endif
}

static void setCheatConditionalValue(int idx, bool conditional, uint8 v)
{
	#ifndef SNES9X_VERSION_1_4
	Cheat.group[idx].cheat[0].conditional = conditional;
	Cheat.group[idx].cheat[0].cond_byte = v;
	#else
	Cheat.c[idx].saved = conditional;
	Cheat.c[idx].saved_byte = v;
	#endif
}

static std::pair<bool, uint8> cheatConditionalValue(int idx)
{
	#ifndef SNES9X_VERSION_1_4
	return {Cheat.group[idx].cheat[0].conditional, Cheat.group[idx].cheat[0].cond_byte};
	#else
	return {Cheat.c[idx].saved, Cheat.c[idx].saved_byte};
	#endif
}

static bool addCheat(const char *cheatStr)
{
	#ifndef SNES9X_VERSION_1_4
	if(S9xAddCheatGroup("", cheatStr) == -1)
	{
		return false;
	}
	return true;
	#else
	uint8 byte;
	uint32 address;
	uint8 bytes[3];
	bool8 sram;
	uint8 numBytes;
	if(!S9xGameGenieToRaw(cheatStr, address, byte))
	{
		S9xAddCheat(false, true, address, byte);
		return true;
	}
	else if(!S9xProActionReplayToRaw (cheatStr, address, byte))
	{
		S9xAddCheat(false, true, address, byte);
		return true;
	}
	else if(!S9xGoldFingerToRaw(cheatStr, address, sram, numBytes, bytes))
	{
		for(auto i : iotaCount(numBytes))
			S9xAddCheat(false, true, address + i, bytes[i]);
		// TODO: handle cheat names for multiple codes added at once
		return true;
	}
	return false;
	#endif
}

static FS::PathString cheatsFilename(EmuSystem &sys_)
{
	auto &sys = static_cast<Snes9xSystem&>(sys_);
	return sys.userFilePath(sys.cheatsDir, ".cht");
}

static void writeCheatsFile(EmuSystem &sys)
{
	if(!numCheats())
		logMsg("no cheats present, removing .cht file if present");
	else
		logMsg("saving %u cheat(s)", numCheats());
	S9xSaveCheatFile(cheatsFilename(sys).data());
}

EmuEditCheatView::EmuEditCheatView(ViewAttachParams attach, int cheatIdx, RefreshCheatsDelegate onCheatListChanged_):
	BaseEditCheatView
	{
		"Edit Address/Values",
		attach,
		cheatName(cheatIdx),
		items,
		[this](TextMenuItem &, View &, Input::Event)
		{
			deleteCheat(idx);
			onCheatListChanged();
			writeCheatsFile(system());
			dismiss();
			return true;
		},
		onCheatListChanged_
	},
	items{&name, &addr, &value, &saved, &remove},
	addr
	{
		"Address",
		u"",
		attach,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 6-digit hex", addrStr.data(),
				[this](CollectTextInputView&, auto str)
				{
					unsigned a = strtoul(str, nullptr, 16);
					if(a > 0xFFFFFF)
					{
						logMsg("addr 0x%X too large", a);
						app().postMessage(true, "Invalid input");
						return false;
					}
					addrStr = a ? str : "0";
					auto wasEnabled = cheatIsEnabled(idx);
					if(wasEnabled)
					{
						disableCheat(idx);
					}
					setCheatAddress(idx, a);
					if(wasEnabled)
					{
						enableCheat(idx);
					}
					writeCheatsFile(system());
					addr.set2ndName(addrStr);
					addr.compile();
					postDraw();
					return true;
				});
		}
	},
	value
	{
		"Value",
		u"",
		attach,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Input 2-digit hex", valueStr.data(),
				[this](CollectTextInputView&, const char *str)
				{
					unsigned a = strtoul(str, nullptr, 16);
					if(a > 0xFF)
					{
						app().postMessage(true, "value must be <= FF");
						return false;
					}
					valueStr = a ? str : "0";
					auto wasEnabled = cheatIsEnabled(idx);
					if(wasEnabled)
					{
						disableCheat(idx);
					}
					setCheatValue(idx, a);
					if(wasEnabled)
					{
						enableCheat(idx);
					}
					writeCheatsFile(system());
					value.set2ndName(valueStr);
					value.compile();
					postDraw();
					return true;
				});
		}
	},
	saved
	{
		#ifndef SNES9X_VERSION_1_4
		"Conditional Value",
		#else
		"Saved Value",
		#endif
		u"",
		attach,
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			pushAndShowNewCollectTextInputView(attachParams(), e, "Input 2-digit hex or blank", savedStr.data(),
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						unsigned a = 0x100;
						if(strlen(str))
						{
							unsigned a = strtoul(str, nullptr, 16);
							if(a > 0xFF)
							{
								app().postMessage(true, "value must be <= FF");
								return true;
							}
							savedStr = str;
						}
						else
						{
							savedStr.clear();
						}
						auto wasEnabled = cheatIsEnabled(idx);
						if(wasEnabled)
						{
							disableCheat(idx);
						}
						if(a <= 0xFF)
						{
							setCheatConditionalValue(idx, true, a);
						}
						else
						{
							setCheatConditionalValue(idx, false, 0);
						}
						if(wasEnabled)
						{
							enableCheat(idx);
						}
						writeCheatsFile(system());
						saved.set2ndName(savedStr);
						saved.compile();
						postDraw();
					}
					view.dismiss();
					return false;
				});
		}
	},
	idx{cheatIdx}
{
	auto address = cheatAddress(idx);
	auto value = cheatValue(idx);
	auto [saved, savedVal] = cheatConditionalValue(idx);
	logMsg("got cheat with addr 0x%.6x val 0x%.2x saved val 0x%.2x", address, value, savedVal);
	IG::formatTo(addrStr, "{:x}", address);
	addr.set2ndName(addrStr);
	IG::formatTo(valueStr, "{:x}", value);
	this->value.set2ndName(valueStr);
	if(!saved)
		savedStr.clear();
	else
	{
		IG::formatTo(savedStr, "{:x}", savedVal);
		this->saved.set2ndName(savedStr);
	}
}

std::string_view EmuEditCheatView::cheatNameString() const
{
	return cheatName(idx);
}

void EmuEditCheatView::renamed(std::string_view str)
{
	setCheatName(idx, str);
	writeCheatsFile(system());
}

void EmuEditCheatListView::loadCheatItems()
{
	auto cheats = numCheats();
	cheat.clear();
	cheat.reserve(cheats);
	for(auto c : iotaCount(cheats))
	{
		cheat.emplace_back(cheatName(c), attachParams(),
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				pushAndShow(makeView<EmuEditCheatView>(c, [this](){ onCheatListChanged(); }), e);
			});
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
		"Add Game Genie/Action Replay/Gold Finger Code", attach,
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(numCheats() == EmuCheats::MAX)
			{
				app().postMessage(true, "Too many cheats, delete some first");
				return;
			}
			pushAndShowNewCollectTextInputView(attachParams(), e,
				"Input xxxx-xxxx (GG), xxxxxxxx (AR), or GF code", "",
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!addCheat(str))
						{
							app().postMessage(true, "Invalid format");
							return true;
						}
						auto idx = numCheats() - 1;
						setCheatName(idx, "Unnamed Cheat");
						logMsg("added new cheat, %d total", numCheats());
						onCheatListChanged();
						writeCheatsFile(system());
						view.dismiss();
						pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
							[this, idx](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									setCheatName(idx, str);
									onCheatListChanged();
									writeCheatsFile(system());
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
	auto cheats = numCheats();
	cheat.clear();
	cheat.reserve(cheats);
	for(auto c : iotaCount(cheats))
	{
		cheat.emplace_back(cheatName(c), attachParams(), cheatIsEnabled(c),
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				bool on = item.flipBoolValue(*this);
				if(on)
					enableCheat(c);
				else
					disableCheat(c);
				writeCheatsFile(system());
			});
	}
}

}
