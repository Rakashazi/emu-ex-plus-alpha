#pragma once

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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/Option.hh>
#include "Cheats.hh"
#include "genplus-config.h"
#include "system.h"
#include "state.h"

extern t_config config;

namespace EmuEx
{

class EmuApp;

enum
{
	CFGKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_SMS_FM = 279,
	CFGKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283,
	CFGKEY_MD_REGION = 284, CFGKEY_VIDEO_SYSTEM = 285,
	CFGKEY_INPUT_PORT_1 = 286, CFGKEY_INPUT_PORT_2 = 287,
	CFGKEY_MULTITAP = 288, CFGKEY_CHEATS_PATH = 289,
};

bool hasMDExtension(std::string_view name);

class MdSystem final: public EmuSystem
{
public:
	std::string cheatsDir;
	int playerIdxMap[4]{};
	int gunDevIdx{};
	int8_t mdInputPortDev[2]{-1, -1};
	int8_t autoDetectedVidSysPAL{};
	int8_t savedVControllerPlayer = -1;

	Property<bool, CFGKEY_BIG_ENDIAN_SRAM> optionBigEndianSram;
	Property<bool, CFGKEY_SMS_FM, PropertyDesc<bool>{.defaultValue = true}> optionSmsFM;
	Property<bool, CFGKEY_6_BTN_PAD> option6BtnPad;
	Property<bool, CFGKEY_MULTITAP> optionMultiTap;
	Property<int8_t, CFGKEY_INPUT_PORT_1, PropertyDesc<int8_t>{.defaultValue = -1, .isValid = isValidWithMinMax<-1, 4>}> optionInputPort1;
	Property<int8_t, CFGKEY_INPUT_PORT_2, PropertyDesc<int8_t>{.defaultValue = -1, .isValid = isValidWithMinMax<-1, 4>}> optionInputPort2;
	Property<uint8_t, CFGKEY_MD_REGION, PropertyDesc<uint8_t>{.isValid = isValidWithMax<4>}> optionRegion;
	Property<uint8_t, CFGKEY_VIDEO_SYSTEM, PropertyDesc<uint8_t>{.isValid = isValidWithMax<2>}> optionVideoSystem;
	#ifndef NO_SCD
	FS::PathString cdBiosUSAPath{}, cdBiosJpnPath{}, cdBiosEurPath{};
	#endif
	static constexpr size_t maxSaveStateSize = STATE_SIZE + 4;
	static constexpr auto ntscFrameTime{fromSeconds<FrameTime>(262. * MCYCLES_PER_LINE / 53693175.)}; // ~59.92Hz
	static constexpr auto palFrameTime{fromSeconds<FrameTime>(313. * MCYCLES_PER_LINE / 53203424.)}; // ~49.70Hz
	std::vector<Cheat> cheatList;
	std::vector<CheatCode*> romCheatList;
	std::vector<CheatCode*> ramCheatList;

	MdSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}
	void setupInput(EmuApp &);
	void writeCheatFile();
	void readCheatFile();
	void applyCheats();
	void clearCheats();
	void clearCheatList();
	void updateCheats();
	void RAMCheatUpdate();
	void ROMCheatUpdate();

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".gp"; }
	size_t stateSize() { return maxSaveStateSize; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return videoSystem() == VideoSystem::PAL ? palFrameTime : ntscFrameTime; }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	WallClockTimePoint backupMemoryLastWriteTime(const EmuApp &) const;
	void closeSystem();
	bool resetSessionOptions(EmuApp &);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	void onSessionOptionsLoaded(EmuApp &);
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState dragState,
		Input::DragTrackerState prevDragState, IG::WindowRect gameRect);
	bool onPointerInputEnd(const Input::MotionEvent &, Input::DragTrackerState, IG::WindowRect gameRect);
	VideoSystem videoSystem() const;
	Cheat* newCheat(EmuApp&, const char* name, CheatCodeDesc);
	bool setCheatName(Cheat&, const char* name);
	std::string_view cheatName(const Cheat&) const;
	void setCheatEnabled(Cheat&, bool on);
	bool isCheatEnabled(const Cheat&) const;
	bool addCheatCode(EmuApp&, Cheat*&, CheatCodeDesc);
	bool modifyCheatCode(EmuApp&, Cheat&, CheatCode&, CheatCodeDesc);
	Cheat* removeCheatCode(Cheat&, CheatCode&);
	bool removeCheat(Cheat&);
	void forEachCheat(DelegateFunc<bool(Cheat&, std::string_view)>);
	void forEachCheatCode(Cheat&, DelegateFunc<bool(CheatCode&, std::string_view)>);

private:
	void setupSmsInput(EmuApp &);
	void setupMdInput(EmuApp &);
};

using MainSystem = MdSystem;

}
