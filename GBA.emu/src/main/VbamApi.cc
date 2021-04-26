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

#include <vbam/gba/GBA.h>
#include <vbam/gba/Sound.h>
#include <vbam/gba/RTC.h>
#include <vbam/common/SoundDriver.h>
#include <vbam/Util.h>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "internal.hh"

int systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
SystemColorMap systemColorMap;

static void debuggerOutput(const char *s, u32 addr)
{
	logMsg("called dbgOutput");
}
void (*dbgOutput)(const char *, u32) = debuggerOutput;

#ifndef NDEBUG
void systemMessage(int num, const char *msg, ...)
{
	if(!logger_isEnabled())
		return;
	va_list args;
	va_start( args, msg );
	logger_vprintf(LOG_M, msg, args);
	va_end( args );
	logger_printf(LOG_M, "\n");
}
#endif

void systemUpdateMotionSensor()
{
}

int systemGetSensorX()
{
	return 0;
}

int systemGetSensorY()
{
	return 0;
}

bool systemCanChangeSoundQuality()
{
	return false;
}

struct GameSettings
{
	const char *gameName;
	const char *gameID;
	int saveType;
	int rtcEnabled;
	int flashSize;
	int mirroringEnabled;
};

static void resetGameSettings()
{
	//agbPrintEnable(0);
	rtcEnable(0);
	cpuSaveType = 0;
	flashSetSize(0x10000);
}

void setGameSpecificSettings(GBASys &gba)
{
	resetGameSettings();
	bool mirroringEnable = 0;
	static const GameSettings setting[] = {
	{       "Dragon Ball Z - The Legacy of Goku II (Europe)(En,Fr,De,Es,It)",
	        "ALFP",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Dragon Ball Z - The Legacy of Goku (Europe)(En,Fr,De,Es,It)",
	        "ALGP",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Rocky (Europe)(En,Fr,De,Es,It)",
	        "AROP",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Rocky (USA)(En,Fr,De,Es,It)",
	        "AR8e",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Pokemon - Ruby Version (USA, Europe)",
	        "AXVE",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Sapphire Version (USA, Europe)",
	        "AXPE",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Super Mario Advance 4 - Super Mario Bros. 3 (Europe)(En,Fr,De,Es,It)",
	        "AX4P",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Top Gun - Combat Zones (USA)(En,Fr,De,Es,It)",
	        "A2YE",
	        5,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Dragon Ball Z - Taiketsu (Europe)(En,Fr,De,Es,It)",
	        "BDBP",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Mario vs. Donkey Kong (Europe)",
	        "BM5P",
	        3,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Pokemon - Emerald Version (USA, Europe)",
	        "BPEE",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Yu-Gi-Oh! - Ultimate Masters - World Championship Tournament 2006 (Europe)(En,Jp,Fr,De,Es,It)",
	        "BY6P",
	        2,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Pokemon Mystery Dungeon - Red Rescue Team (USA, Australia)",
	        "B24E",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon Mystery Dungeon - Red Rescue Team (Europe)",
	        "B24P",
	        3,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Classic NES Series - Castlevania (USA, Europe)",
	        "FADE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Bomberman (USA, Europe)",
	        "FBME",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Donkey Kong (USA, Europe)",
	        "FDKE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Dr. Mario (USA, Europe)",
	        "FDME",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Excitebike (USA, Europe)",
	        "FEBE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Ice Climber (USA, Europe)",
	        "FICE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Zelda II - The Adventure of Link (USA, Europe)",
	        "FLBE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Metroid (USA, Europe)",
	        "FMRE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Pac-Man (USA, Europe)",
	        "FP7E",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Super Mario Bros. (USA, Europe)",
	        "FSME",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Xevious (USA, Europe)",
	        "FXVE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Classic NES Series - Legend of Zelda (USA, Europe)",
	        "FZLE",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Yoshi's Universal Gravitation (Europe)(En,Fr,De,Es,It)",
	        "KYGP",
	        4,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Boktai - The Sun Is in Your Hand (Europe)(En,Fr,De,Es,It)",
	        "U3IP",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Boktai 2 - Solar Boy Django (Europe)(En,Fr,De,Es,It)",
	        "U32P",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Golden Sun - The Lost Age (USA)",
	        "AGFE",
	        -1,
	        1,
	        0x10000,
	        -1
	        },
	        {
	        "Golden Sun (USA)",
	        "AGSE",
	        -1,
	        1,
	        0x10000,
	        -1
	        },
	        {
	        "Dragon Ball Z - The Legacy of Goku II (USA)",
	        "ALFE",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Dragon Ball Z - The Legacy of Goku (USA)",
	        "ALGE",
	        1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Super Mario Advance 4 - Super Mario Bros 3 - Super Mario Advance 4 v1.1 (USA)",
	        "AX4E",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Dragon Ball Z - Taiketsu (USA)",
	        "BDBE",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Dragon Ball Z - Buu's Fury (USA)",
	        "BG3E",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "2 Games in 1 - Dragon Ball Z - The Legacy of Goku I & II (USA)",
	        "BLFE",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Pokemon - Fire Red Version (USA, Europe)",
	        "BPRE",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Leaf Green Version (USA, Europe)",
	        "BPGE",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Dragon Ball GT - Transformation (USA)",
	        "BT4E",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "2 Games in 1 - Dragon Ball Z - Buu's Fury + Dragon Ball GT - Transformation (USA)",
	        "BUFE",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "useBios=1",
	        "BYGE",
	        2,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Yoshi - Topsy-Turvy (USA)",
	        "KYGE",
	        4,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "e-Reader (USA)",
	        "PSAE",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Boktai - The Sun Is in Your Hand (USA)",
	        "U3IE",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Boktai 2 - Solar Boy Django (USA)",
	        "U32E",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Dragon Ball Z - The Legacy of Goku II International (Japan)",
	        "ALFJ",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Pocket Monsters - Sapphire (Japan)",
	        "AXPJ",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pocket Monsters - Ruby (Japan)",
	        "AXVJ",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Super Mario Advance 4 (Japan)",
	        "AX4J",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "F-Zero - Climax (Japan)",
	        "BFTJ",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Game Boy Wars Advance 1+2 (Japan)",
	        "BGWJ",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Sennen Kazoku (Japan)",
	        "BKAJ",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pocket Monsters - Emerald (Japan)",
	        "BPEJ",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pocket Monsters - Leaf Green (Japan)",
	        "BPGJ",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pocket Monsters - Fire Red (Japan)",
	        "BPRJ",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Digi Communication 2 - Datou! Black Gemagema Dan (Japan)",
	        "BDKJ",
	        1,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Rockman EXE 4.5 - Real Operation (Japan)",
	        "BR4J",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Famicom Mini Vol. 01 - Super Mario Bros. (Japan)",
	        "FMBJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 12 - Clu Clu Land (Japan)",
	        "FCLJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 13 - Balloon Fight (Japan)",
	        "FBFJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 14 - Wrecking Crew (Japan)",
	        "FWCJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 15 - Dr. Mario (Japan)",
	        "FDMJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 16 - Dig Dug (Japan)",
	        "FDDJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 17 - Takahashi Meijin no Boukenjima (Japan)",
	        "FTBJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 18 - Makaimura (Japan)",
	        "FMKJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 19 - Twin Bee (Japan)",
	        "FTWJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 20 - Ganbare Goemon! Karakuri Douchuu (Japan)",
	        "FGGJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 21 - Super Mario Bros. 2 (Japan)",
	        "FM2J",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 22 - Nazo no Murasame Jou (Japan)",
	        "FNMJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 23 - Metroid (Japan)",
	        "FMRJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 24 - Hikari Shinwa - Palthena no Kagami (Japan)",
	        "FPTJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 25 - The Legend of Zelda 2 - Link no Bouken (Japan)",
	        "FLBJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 26 - Famicom Mukashi Banashi - Shin Onigashima - Zen Kou Hen (Japan)",
	        "FFMJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 27 - Famicom Tantei Club - Kieta Koukeisha - Zen Kou Hen (Japan)",
	        "FTKJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 28 - Famicom Tantei Club Part II - Ushiro ni Tatsu Shoujo - Zen Kou Hen (Japan)",
	        "FTUJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 29 - Akumajou Dracula (Japan)",
	        "FADJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Famicom Mini Vol. 30 - SD Gundam World - Gachapon Senshi Scramble Wars (Japan)",
	        "FSDJ",
	        1,
	        -1,
	        -1,
	        1
	        },
	        {
	        "Koro Koro Puzzle - Happy Panechu! (Japan)",
	        "KHPJ",
	        4,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Yoshi no Banyuuinryoku (Japan)",
	        "KYGJ",
	        4,
	        -1,
	        -1,
	        -1
	        },
	        {
	        "Card e-Reader+ (Japan)",
	        "PSAJ",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Bokura no Taiyou - Taiyou Action RPG (Japan)",
	        "U3IJ",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Zoku Bokura no Taiyou - Taiyou Shounen Django (Japan)",
	        "U32J",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Shin Bokura no Taiyou - Gyakushuu no Sabata (Japan)",
	        "U33J",
	        -1,
	        1,
	        -1,
	        -1
	        },
	        {
	        "Mother 3 (Japan)",
	        "A3UJ",
	        -1,
	        -1,
	        65536,
	        -1
	        },
	        {
	        "Pokemon - Version Saphir (France)",
	        "AXPF",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Version Rubis (France)",
	        "AXVF",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Version Emeraude (France)",
	        "BPEF",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Version Vert Feuille (France)",
	        "BPGF",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Version Rouge Feu (France)",
	        "BPRF",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Versione Zaffiro (Italy)",
	        "AXPI",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Versione Rubino (Italy)",
	        "AXVI",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Versione Smeraldo (Italy)",
	        "BPEI",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Versione Verde Foglia (Italy)",
	        "BPGI",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Versione Rosso Fuoco (Italy)",
	        "BPRI",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Saphir-Edition (Germany)",
	        "AXPD",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Rubin-Edition (Germany)",
	        "AXVD",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Smaragd-Edition (Germany)",
	        "BPED",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Blattgruene Edition (Germany)",
	        "BPGD",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Feuerrote Edition (Germany)",
	        "BPRD",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Edicion Zafiro (Spain)",
	        "AXPS",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Edicion Rubi (Spain)",
	        "AXVS",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Edicion Esmeralda (Spain)",
	        "BPES",
	        -1,
	        1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Edicion Verde Hoja (Spain)",
	        "BPGS",
	        -1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "Pokemon - Edicion Rojo Fuego (Spain)",
	        "BPRS",
	        1,
	        -1,
	        131072,
	        -1
	        },
	        {
	        "WarioWare - Twisted! (USA)",
	        "RZWE",
	        -1,
	        1, // needs "RealTimeClock" (actually motion sensor and rumble)
	        -1,
	        -1
	        },
	        {
	        "Mawaru Made in Wario (Japan)",
	        "RZWJ",
	        -1,
	        1, // needs "RealTimeClock" (actually motion sensor and rumble)
	        -1,
	        -1
	        },
	};

	resetGameSettings();
	logMsg("game id: %c%c%c%c", gba.mem.rom[0xac], gba.mem.rom[0xad], gba.mem.rom[0xae], gba.mem.rom[0xaf]);
	for(auto e : setting)
	{
		if(IG::equal_n(e.gameID, 4, &gba.mem.rom[0xac]))
		{
			logMsg("loading settings for: %s", e.gameName);
			if(e.rtcEnabled >= 0)
			{
				logMsg("using RTC");
				detectedRtcGame = 1;
			}
			if(e.flashSize > 0)
			{
				logMsg("using flash size %d", e.flashSize);
				flashSetSize(e.flashSize);
			}
			if(e.saveType >= 0)
			{
				logMsg("using save type %d", e.saveType);
				cpuSaveType = e.saveType;
			}
			if(e.mirroringEnabled >= 0)
			{
				logMsg("using mirroring");
				mirroringEnable = e.mirroringEnabled;
			}
			break;
		}
	}

	switch(gba.mem.rom[0xac])
	{
		bcase 'F': // Classic NES
			logMsg("using classic NES series settings");
			cpuSaveType = 1; // EEPROM
			mirroringEnable = 1;
		bcase 'K': // Accelerometers
			cpuSaveType = 4; // EEPROM + sensor
		bcase 'R': // WarioWare Twisted style sensors
		case 'V': // Drill Dozer
			//rtcEnableWarioRumble(true);
		bcase 'U': // Boktai solar sensor and clock
		detectedRtcGame = 1;
	}
	doMirroring(gba, mirroringEnable);

	if(detectedRtcGame && (unsigned)optionRtcEmulation == RTC_EMU_AUTO)
	{
		logMsg("automatically enabling RTC");
		rtcEnable(true);
	}
	else
	{
		rtcEnable((unsigned)optionRtcEmulation == RTC_EMU_ON);
	}
}
