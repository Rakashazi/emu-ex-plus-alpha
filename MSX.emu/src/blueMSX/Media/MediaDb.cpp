/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Media/MediaDb.cpp,v $
**
** $Revision: 1.91 $
**
** $Date: 2009-04-30 03:53:28 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
extern "C" {
#include "MsxTypes.h"
#include "MediaDb.h"
#include "Crc32Calc.h"
#include "TokenExtract.h"
#include "StrcmpNoCase.h"
#include "ArchGlob.h"
#include "Board.h"
#include "Language.h"
}

#include "tinyxml.h"
#include "Sha1.h"
#include <string>
#include <map>

using namespace std;

typedef map<UInt32, MediaType*> CrcMap;
typedef map<string, MediaType*> Sha1Map;


struct MediaDb {
    Sha1Map sha1Map;
    CrcMap crcMap;
};

struct MediaType {
    MediaType(RomType rt, const string t, const string c = "", const string y = "", const string ct = "", const string r = "", string s = "") :
        romType(rt), title(t), company(c), year(y), country(ct), remark(r), start(s) {}

    MediaType(const MediaType& mt) :
        romType(mt.romType), title(mt.title), company(mt.company), year(mt.year), 
        country(mt.country), remark(mt.remark), start(mt.start) {}

    string title;
    string company;
    string year;
    string country;
    string remark;
    RomType romType;
    string start;
};

static MediaDb* romdb;
static MediaDb* diskdb;
static MediaDb* casdb;
static RomType  romdbDefaultType = ROM_UNKNOWN;


static string parseCountryCode(const string& code)
{
    if (code == "SE" || code == "se") return "Sweden";
    if (code == "JP" || code == "jp") return "Japan";
    if (code == "KR" || code == "kr") return "Korea";
    if (code == "NL" || code == "nl") return "The Netherlands";
    if (code == "GB" || code == "gb") return "England";
    if (code == "FR" || code == "fr") return "France";
    if (code == "ES" || code == "es") return "Spain";
    if (code == "BR" || code == "br") return "Brazil";
    if (code == "MA" || code == "ma") return "Arabic";

    return code;
}

RomType mediaDbStringToType(const char* romName)
{
    const std::string name = romName;

    // Megaroms
    if (name == "ASCII16")          return ROM_ASCII16;
    if (name == "ASCII16SRAM2")     return ROM_ASCII16SRAM;
    if (name == "ASCII8")           return ROM_ASCII8;
    if (name == "ASCII8SRAM8")      return ROM_ASCII8SRAM;
    if (name == "KoeiSRAM8")        return ROM_KOEI;
    if (name == "KoeiSRAM32")       return ROM_KOEI;
    if (name == "Konami")           return ROM_KONAMI4;
    if (name == "KonamiSCC")        return ROM_KONAMI5;
    if (name == "Manbow2")          return ROM_MANBOW2;
    if (name == "MegaFlashRomScc")  return ROM_MEGAFLSHSCC;
    if (name == "Halnote")          return ROM_HALNOTE;
    if (name == "HarryFox")         return ROM_HARRYFOX;
    if (name == "Playball")         return ROM_PLAYBALL;
    if (name == "HolyQuran")        return ROM_HOLYQURAN;
    if (name == "CrossBlaim")       return ROM_CROSSBLAIM;
    if (name == "Zemina80in1")      return ROM_KOREAN80;
    if (name == "Zemina90in1")      return ROM_KOREAN90;
    if (name == "Zemina126in1")     return ROM_KOREAN126;
    if (name == "Wizardry")         return ROM_ASCII8SRAM;
    if (name == "GameMaster2")      return ROM_GAMEMASTER2;
    if (name == "SuperLodeRunner")  return ROM_LODERUNNER;
    if (name == "R-Type")           return ROM_RTYPE;
    if (name == "Majutsushi")       return ROM_MAJUTSUSHI;
    if (name == "Synthesizer")      return ROM_KONAMISYNTH;
    if (name == "KeyboardMaster")   return ROM_KONAMKBDMAS;
    if (name == "GenericKonami")    return ROM_KONAMI4NF;
    if (name == "SuperPierrot")     return ROM_ASCII16NF;
    if (name == "WordPro")          return ROM_KONWORDPRO;
    if (name == "Normal")           return ROM_STANDARD;
    if (name == "MatraInk")         return ROM_MATRAINK;
    if (name == "NettouYakyuu")     return ROM_NETTOUYAKYUU;

    // System roms
    if (name == "Bunsetsu")     return ROM_BUNSETU;
    if (name == "CasPatch")     return ROM_CASPATCH;
    if (name == "Coleco")       return ROM_COLECO;
    if (name == "MegaCart")     return ROM_CVMEGACART;
    if (name == "SG1000")       return ROM_SG1000;
    if (name == "SC3000")       return ROM_SC3000;
    if (name == "SG1000Castle") return ROM_SG1000CASTLE;
    if (name == "SegaBasic")    return ROM_SEGABASIC;
    if (name == "FMPAC")        return ROM_FMPAC;
    if (name == "FMPAK")        return ROM_FMPAK;
    if (name == "DiskPatch")    return ROM_DISKPATCH;
    if (name == "Jisyo")        return ROM_JISYO;
    if (name == "Kanji1")       return ROM_KANJI;
    if (name == "Kanji12")      return ROM_KANJI12;
    if (name == "MB8877A")      return ROM_NATIONALFDC;
    if (name == "SVI738FDC")    return ROM_SVI738FDC;
    if (name == "TC8566AF")     return ROM_TC8566AF;
    if (name == "TC8566AFTR")   return ROM_TC8566AF_TR;
    if (name == "WD2793")       return ROM_PHILIPSFDC;
    if (name == "Microsol")     return ROM_MICROSOL;
    if (name == "ARC")          return ROM_ARC;
    if (name == "MoonSound")    return ROM_MOONSOUND;

    if (name == "SunriseIDE")   return ROM_SUNRISEIDE;
    if (name == "BeerIDE")      return ROM_BEERIDE;
    if (name == "GIDE")         return ROM_GIDE;
    if (name == "GoudaSCSI")    return ROM_GOUDASCSI;

    if (name == "NMS1210")      return ROM_NMS1210;

    if (name == "fsa1fm2")      return ROM_PANASONIC8;
    if (name == "FSA1FM2")      return ROM_PANASONIC8;
    if (name == "Panasonic8")   return ROM_PANASONIC8;
    if (name == "PanasonicDram")return ROM_DRAM;
    if (name == "PanasonicWx16")return ROM_PANASONICWX16;
    if (name == "Panasonic16")  return ROM_PANASONIC16;
    if (name == "Panasonic32")  return ROM_PANASONIC32;
    if (name == "A1FMModem")    return ROM_FSA1FMMODEM;
    if (name == "fsa1fm1")      return ROM_FSA1FMMODEM;
    if (name == "FSA1FM1")      return ROM_FSA1FMMODEM;
    if (name == "Standard16K")  return ROM_MSXDOS2;
    if (name == "SVI328CART")   return ROM_SVI328;
    if (name == "SVI80COL")     return ROM_SVI80COL;
    if (name == "SVI727")       return ROM_SVI727;
    if (name == "SVI738FDC")    return ROM_SVI738FDC;
    if (name == "MSX-AUDIO")    return ROM_MSXAUDIO;
    if (name == "MSX-MUSIC")    return ROM_MSXMUSIC;
    if (name == "National")     return ROM_NATIONAL;
    if (name == "CX5M-MUSIC")   return ROM_YAMAHASFG05;
    if (name == "VMX80")        return ROM_MICROSOL80;
    if (name == "HBI-V1")       return ROM_SONYHBIV1;
    if (name == "SFG-01")       return ROM_YAMAHASFG01;
    if (name == "SFG-05")       return ROM_YAMAHASFG05;
    if (name == "NET")          return ROM_YAMAHANET;
    if (name == "SF-7000IPL")   return ROM_SF7000IPL;
    if (name == "FMDAS")        return ROM_FMDAS;
    if (name == "Obsonet")      return ROM_OBSONET;
    if (name == "Dumas")        return ROM_DUMAS;
    if (name == "NoWind")       return ROM_NOWIND;

    // Roms not supproted in this format in the db
    if (name == "0x4000")       return ROM_0x4000;
    if (name == "0xC000")       return ROM_0xC000;
    if (name == "auto")         return ROM_PLAIN;
    if (name == "basic")        return ROM_BASIC;

    if (name == "mirrored")     return ROM_PLAIN;
    if (name == "forteII")      return ROM_FORTEII;
    if (name == "msxdos2")      return ROM_MSXDOS2;
    if (name == "konami5")      return ROM_KONAMI5;
    if (name == "konami4")      return ROM_KONAMI4;
    if (name == "ascii8")       return ROM_ASCII8;
    if (name == "halnote")      return ROM_HALNOTE;
    if (name == "konamisynth")  return ROM_KONAMISYNTH;
    if (name == "kbdmaster")    return ROM_KONAMKBDMAS;
    if (name == "majutsushi")   return ROM_MAJUTSUSHI;
    if (name == "ascii16")      return ROM_ASCII16;
    if (name == "gamemaster2")  return ROM_GAMEMASTER2;
    if (name == "ascii8sram")   return ROM_ASCII8SRAM;
    if (name == "koei")         return ROM_KOEI;
    if (name == "ascii16sram")  return ROM_ASCII16SRAM;
    if (name == "konami4nf")    return ROM_KONAMI4NF;
    if (name == "ascii16nf")    return ROM_ASCII16NF;
    if (name == "snatcher")     return ROM_SNATCHER;
    if (name == "sdsnatcher")   return ROM_SDSNATCHER;
    if (name == "sccmirrored")  return ROM_SCCMIRRORED;
    if (name == "sccexpanded")  return ROM_SCCEXTENDED;
    if (name == "scc")          return ROM_SCC;
    if (name == "sccplus")      return ROM_SCCPLUS;
    if (name == "scc-i")        return ROM_SCCPLUS;
    if (name == "scc+")         return ROM_SCCPLUS;
    if (name == "pac")          return ROM_PAC;
    if (name == "fmpac")        return ROM_FMPAC;
    if (name == "fmpak")        return ROM_FMPAK;
    if (name == "rtype")        return ROM_RTYPE;
    if (name == "crossblaim")   return ROM_CROSSBLAIM;
    if (name == "harryfox")     return ROM_HARRYFOX;
    if (name == "loderunner")   return ROM_LODERUNNER;
    if (name == "korean80")     return ROM_KOREAN80;
    if (name == "korean90")     return ROM_KOREAN90;
    if (name == "korean126")    return ROM_KOREAN126;
    if (name == "holyquran")    return ROM_HOLYQURAN;  
    if (name == "opcodesave")   return ROM_OPCODESAVE;
    if (name == "opcodebios")   return ROM_OPCODEBIOS;
    if (name == "opcodeslot")   return ROM_OPCODESLOT;
    if (name == "opcodeega")    return ROM_OPCODEMEGA;
    if (name == "coleco")       return ROM_COLECO;
    if (name == "sg1000")       return ROM_SG1000;
    if (name == "castle")       return ROM_SG1000CASTLE;


    // SG-1000 roms
    if (name == "sg1000castle") return ROM_SG1000CASTLE;


    return ROM_UNKNOWN;
}


static string mediaDbGetRemarks(TiXmlElement* dmp)
{
    string remark;

    for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
        if (strcmp(it->Value(), "remark") == 0) {
            for (TiXmlElement* i = it->FirstChildElement(); i != NULL; i = i->NextSiblingElement()) {
                if (strcmp(i->Value(), "text") == 0) {
                    TiXmlNode* name = i->FirstChild();
                    if (name != NULL) {
                        if (remark.length()) {
                            remark += "\n";
                        }
                        remark += name->Value();
                    }
                }
            }
        }
    }

    return remark;
}

static string mediaDbGetStart(TiXmlElement* dmp)
{
    string start;

    for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
        if (strcmp(it->Value(), "start") == 0) {
            for (TiXmlElement* i = it->FirstChildElement(); i != NULL; i = i->NextSiblingElement()) {
                if (strcmp(i->Value(), "text") == 0) {
                    TiXmlNode* name = i->FirstChild();
                    if (name != NULL) {
                        if (start.length()) {
                            start += "\n";
                        }
                        start += name->Value();
                    }
                }
            }
        }
    }
    return start;
}

static const bool genEmbeddedRomDB = 1;

static void mediaDbAddItem(MediaDb* mediaDb, TiXmlElement* dmp, const MediaType& mediaType)
{
    for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
        if (strcmp(it->Value(), "hash") == 0) {
            const char* type = it->Attribute("algo");
            if (type != NULL) {
                if (strcmp(type, "sha1") == 0) {
                    TiXmlNode* hash = it->FirstChild();
                    string sha1(hash->Value());
                    mediaDb->sha1Map[sha1] = new MediaType(mediaType);
                    if(genEmbeddedRomDB && mediaType.romType != ROM_UNKNOWN)
                    {
                    printf("added sha1 %s type %d\n", sha1.c_str(), mediaType.romType);
                    unsigned int digest[5];
                    static FILE *dbOut = 0;
                    if(!dbOut) dbOut = fopen("EmbeddedRomDBData.h", "wb");
                    fprintf(dbOut, "\t{ {");
                    for(int i = 0; i < 5; i++)
                    {
                    	fprintf(dbOut, "0x");
                    	for(int c = 0; c < 8; c++)
                    	{
                    		int idx = (i*8)+c;
                    		char val = '0';
                    		if(idx < sha1.length())
                    			val = sha1.c_str()[idx];
                    		fprintf(dbOut, "%c", val);
                    	}
                    	if(i != 4) fprintf(dbOut, ", ");
                    }
                    fprintf(dbOut, " }, %d },\n", mediaType.romType);
                    fflush(dbOut);
                    }
                    //if (mediaDb == casdb) printf("Adding: %s: %s\n", mediaType.title.c_str(), sha1.c_str());
                }
                if (strcmp(type, "crc32") == 0) {
                    UInt32 crc32;
                    TiXmlNode* hash = it->FirstChild();
                    if (sscanf(hash->Value(), "%x", &crc32) == 1) {
                        mediaDb->crcMap[crc32] = new MediaType(mediaType);
                        //printf("added crc32 0x%X type %d\n", crc32, mediaType.romType);
                    }
                }
            }
        }
    }
}

static void mediaDbAddDump(TiXmlElement* dmp, 
                           string& title,
                           string& company,
                           string& country,
                           string& year,
                           string& system)
{
    if (strcmp(dmp->Value(), "megarom") == 0 || strcmp(dmp->Value(), "systemrom") == 0 || strcmp(dmp->Value(), "rom") == 0) {
        RomType romType = strcmp(dmp->Value(), "rom") == 0 ? ROM_PLAIN : ROM_UNKNOWN;

        for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
            if (strcmp(it->Value(), "type") == 0) {
                TiXmlNode* name = it->FirstChild();
                if (name != NULL) {
                    romType = mediaDbStringToType(name->Value());
                }
            }
        }

        if (romType != ROM_CVMEGACART) {
            if (strcmpnocase(system.c_str(), "coleco") == 0) {
                romType = ROM_COLECO;
            }
        }

        if (strcmpnocase(system.c_str(), "svi") == 0) {
            if (romType != ROM_SVI80COL) {
                romType = ROM_SVI328;
            }
        }

        if (romType != ROM_SG1000CASTLE && romType != ROM_SEGABASIC) {
            if (strcmpnocase(system.c_str(), "sg1000") == 0) {
                romType = ROM_SG1000;
            }

            if (strcmpnocase(system.c_str(), "sc3000") == 0 ||
                strcmpnocase(system.c_str(), "sf7000") == 0)
            {
                romType = ROM_SC3000;
            }
        }

        // For standard roms, a start tag is used to specify start address
        if (romType == ROM_STANDARD) {
            for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
                if (strcmp(it->Value(), "start") == 0) {
                    TiXmlNode* name = it->FirstChild();
                    if (name != NULL) {
                        if (strcmp(name->Value(), "0x0000") == 0) {
                            romType = ROM_STANDARD;
                        }
                        if (strcmp(name->Value(), "0x4000") == 0) {
                            romType = ROM_0x4000;
                        }
                        if (strcmp(name->Value(), "0x8000") == 0) {
                            romType = ROM_BASIC;
                        }
                        if (strcmp(name->Value(), "0xC000") == 0) {
                            romType = ROM_0xC000;
                        }
                    }
                }
            }
        }

        string remark = mediaDbGetRemarks(dmp);

        mediaDbAddItem(romdb, dmp, MediaType(romType, title, company, year, country, remark));
    }

    if (strcmp(dmp->Value(), "sccpluscart") == 0) {
        RomType romType = ROM_SCC;
        

        for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
            if (strcmp(it->Value(), "boot") == 0) {
                TiXmlNode* name = it->FirstChild();
                if (name != NULL && strcmp(name->Value(), "scc+") == 0) {
                    romType = ROM_SCCPLUS;
                }
            }
        }

        string remark = mediaDbGetRemarks(dmp);

        mediaDbAddItem(romdb, dmp, MediaType(romType, title, company, year, country, remark));
    }

    if (strcmp(dmp->Value(), "cas") == 0) {
        string start = mediaDbGetStart(dmp);
        string remark = mediaDbGetRemarks(dmp);

        for (TiXmlElement* itt = dmp->FirstChildElement(); itt != NULL; itt = itt->NextSiblingElement()) {
            if (strcmp(itt->Value(), "format") == 0) {
                const char* type = itt->Attribute("type");
                if (type != NULL && strcmp(type, "cas") == 0) {
                    mediaDbAddItem(casdb, itt, MediaType(ROM_UNKNOWN, title, company, year, country, remark, start));
                }
            }
        }
    }

    if (strcmp(dmp->Value(), "dsk") == 0) {
        string start = mediaDbGetStart(dmp);
        string remark = mediaDbGetRemarks(dmp);

        for (TiXmlElement* itt = dmp->FirstChildElement(); itt != NULL; itt = itt->NextSiblingElement()) {
            if (strcmp(itt->Value(), "format") == 0) {
                const char* type = itt->Attribute("type");
                if (type != NULL && strcmp(type, "dsk") == 0) {
                    mediaDbAddItem(diskdb, itt, MediaType(ROM_UNKNOWN, title, company, year, country, remark, start));
                }
            }
        }
    }
}

void mediaDbAddFromXmlFile(const char* fileName)
{
    static const char* rootTag = "softwaredb";

    if (fileName == NULL) {
        return;
    }

    TiXmlDocument doc(fileName);

    doc.LoadFile();
    if (doc.Error()) {
        return;
    }
    
    TiXmlElement* root = doc.RootElement();
    if (root == NULL || strcmp(root->Value(), rootTag) != 0) {
        return;
    }
    
    for (TiXmlElement* sw = root->FirstChildElement(); sw != NULL; sw = sw->NextSiblingElement()) {
        if (strcmp(sw->Value(), "software") != 0) {
            continue;
        }

        string  title;
        string  company;
        string country;
        string  year;
        string  system;
        
        TiXmlElement* item;

        for (item = sw->FirstChildElement(); item != NULL; item = item->NextSiblingElement()) {
            if (strcmp(item->Value(), "system") == 0) {
                TiXmlNode* name = item->FirstChild();
                if (name != NULL) {
                    system = name->Value();
                }
            }
            if (strcmp(item->Value(), "title") == 0) {
                TiXmlNode* name = item->FirstChild();
                if (name != NULL) {
                    title = name->Value();
                }
            }
            if (strcmp(item->Value(), "company") == 0) {
                TiXmlNode* name = item->FirstChild();
                if (name != NULL) {
                    company = name->Value();
                }
            }
            if (strcmp(item->Value(), "country") == 0) {
                TiXmlNode* name = item->FirstChild();
                if (name != NULL) {
                    country = parseCountryCode(name->Value());
                }
            }
            if (strcmp(item->Value(), "year") == 0) {
                TiXmlNode* name = item->FirstChild();
                if (name != NULL) {
                    year = name->Value();
                }
            }
        }

        for (item = sw->FirstChildElement(); item != NULL; item = item->NextSiblingElement()) {
            if (strcmp(item->Value(), "dump") != 0) {
                continue;
            }
            
            string start;

            for (TiXmlElement* dmp = item->FirstChildElement(); dmp != NULL; dmp = dmp->NextSiblingElement()) {
                if (strcmp(dmp->Value(), "group") == 0) {
                    for (TiXmlElement* it = dmp->FirstChildElement(); it != NULL; it = it->NextSiblingElement()) {
                        mediaDbAddDump(it, title, company, country, year, system);
                    }
                    continue;
                }
                mediaDbAddDump(dmp, title, company, country, year, system);
            }
        }
    }
}

extern MediaType* mediaDbLookup(MediaDb* mediaDb, const void *buffer, int size)
{
    if (size > 2 * 1024 * 1024) {
        return NULL;
    }

	SHA1 sha1;
	sha1.update((const UInt8*)buffer, size);
    
//    printf("SHA1: %s\n", sha1.hex_digest().c_str());

    Sha1Map::iterator iterSha1 = mediaDb->sha1Map.find(sha1.hex_digest());
    if (iterSha1 != mediaDb->sha1Map.end()) {
        return iterSha1->second;
    }

    UInt32 crc = calcCrc32(buffer, size);

    CrcMap::iterator iterCrc = mediaDb->crcMap.find(crc);
    if (iterCrc != mediaDb->crcMap.end()) {
        return iterCrc->second;
    }
    
    return NULL;
}

extern "C" const char* romTypeToString(RomType romType)
{
    switch (romType) {    
    case ROM_STANDARD:    return langRomTypeStandard();
    case ROM_MSXDOS2:     return langRomTypeMsxdos2();
    case ROM_KONAMI5:     return langRomTypeKonamiScc();
    case ROM_MANBOW2:     return langRomTypeManbow2();
    case ROM_MEGAFLSHSCC: return langRomTypeMegaFlashRomScc();
    case ROM_OBSONET:     return langRomTypeObsonet();
    case ROM_DUMAS:       return langRomTypeDumas();
    case ROM_NOWIND:      return langRomTypeNoWind();
    case ROM_KONAMI4:     return langRomTypeKonami();
    case ROM_ASCII8:      return langRomTypeAscii8();
    case ROM_ASCII16:     return langRomTypeAscii16();
    case ROM_GAMEMASTER2: return langRomTypeGameMaster2();
    case ROM_ASCII8SRAM:  return langRomTypeAscii8Sram();
    case ROM_ASCII16SRAM: return langRomTypeAscii16Sram();
    case ROM_RTYPE:       return langRomTypeRtype();
    case ROM_CROSSBLAIM:  return langRomTypeCrossblaim();
    case ROM_HARRYFOX:    return langRomTypeHarryFox();
    case ROM_MAJUTSUSHI:  return langRomTypeMajutsushi();
    case ROM_KOREAN80:    return langRomTypeZenima80();
    case ROM_KOREAN90:    return langRomTypeZenima90();
    case ROM_KOREAN126:   return langRomTypeZenima126();
    case ROM_SCC:         return langRomTypeScc();
    case ROM_SCCPLUS:     return langRomTypeSccPlus();
    case ROM_SNATCHER:    return langRomTypeSnatcher();
    case ROM_SDSNATCHER:  return langRomTypeSdSnatcher();
    case ROM_SCCMIRRORED: return langRomTypeSccMirrored();
    case ROM_SCCEXTENDED: return langRomTypeSccExtended();
    case ROM_FMPAC:       return langRomTypeFmpac();
    case ROM_FMPAK:       return langRomTypeFmpak();
    case ROM_KONAMI4NF:   return langRomTypeKonamiGeneric();
    case ROM_ASCII16NF:   return langRomTypeSuperPierrot();
    case ROM_PLAIN:       return langRomTypeMirrored();
    case ROM_NETTOUYAKYUU:return "Jaleco Moero!! Nettou Yakyuu '88";
    case ROM_MATRAINK:    return "Matra INK";
    case ROM_FORTEII:     return "Forte II";
    case ROM_NORMAL:      return langRomTypeNormal();
    case ROM_DISKPATCH:   return langRomTypeDiskPatch();
    case ROM_CASPATCH:    return langRomTypeCasPatch();
    case ROM_TC8566AF:    return langRomTypeTc8566afFdc();
    case ROM_TC8566AF_TR: return langRomTypeTc8566afTrFdc();
    case ROM_MICROSOL:    return langRomTypeMicrosolFdc();
    case ROM_ARC:         return "Parallax ARC";
    case ROM_NATIONALFDC: return langRomTypeNationalFdc();
    case ROM_PHILIPSFDC:  return langRomTypePhilipsFdc();
    case ROM_SVI738FDC:   return langRomTypeSvi738Fdc();
    case RAM_MAPPER:      return langRomTypeMappedRam();
    case RAM_1KB_MIRRORED:return langRomTypeMirroredRam1k();
    case RAM_2KB_MIRRORED:return langRomTypeMirroredRam2k();
    case RAM_NORMAL:      return langRomTypeNormalRam();
    case ROM_KANJI:       return langRomTypeKanji();
    case ROM_HOLYQURAN:   return langRomTypeHolyQuran();
    case SRAM_MATSUCHITA: return langRomTypeMatsushitaSram();
    case SRAM_MATSUCHITA_INV: return langRomTypeMasushitaSramInv();
    case ROM_PANASONIC8:  return langRomTypePanasonic8();
    case ROM_PANASONICWX16:return langRomTypePanasonicWx16();
    case ROM_PANASONIC16: return langRomTypePanasonic16();
    case ROM_PANASONIC32: return langRomTypePanasonic32();
    case ROM_FSA1FMMODEM: return langRomTypePanasonicModem();
    case ROM_DRAM:        return langRomTypeDram();
    case ROM_BUNSETU:     return langRomTypeBunsetsu();
    case ROM_JISYO:       return langRomTypeJisyo();
    case ROM_KANJI12:     return langRomTypeKanji12();
    case ROM_NATIONAL:    return langRomTypeNationalSram();
    case SRAM_S1985:      return langRomTypeS1985();
    case ROM_S1990:       return langRomTypeS1990();
    case ROM_TURBORIO:    return langRomTypeTurborPause();
    case ROM_F4DEVICE:    return langRomTypeF4deviceNormal();
    case ROM_F4INVERTED:  return langRomTypeF4deviceInvert();
    case ROM_MSXMIDI:     return langRomTypeMsxMidi();
    case ROM_TURBORTIMER: return langRomTypeTurborTimer();
    case ROM_KOEI:        return langRomTypeKoei();
    case ROM_BASIC:       return langRomTypeBasic();
    case ROM_HALNOTE:     return langRomTypeHalnote();
    case ROM_LODERUNNER:  return langRomTypeLodeRunner();
    case ROM_0x4000:      return langRomTypeNormal4000();
    case ROM_0xC000:      return langRomTypeNormalC000();
    case ROM_KONAMISYNTH: return langRomTypeKonamiSynth();
    case ROM_KONAMKBDMAS: return langRomTypeKonamiKbdMast();
    case ROM_KONWORDPRO:  return langRomTypeKonamiWordPro();
    case ROM_PAC:         return langRomTypePac();
    case ROM_MEGARAM:     return langRomTypeMegaRam();
    case ROM_MEGARAM128:  return langRomTypeMegaRam128();
    case ROM_MEGARAM256:  return langRomTypeMegaRam256();
    case ROM_MEGARAM512:  return langRomTypeMegaRam512();
    case ROM_MEGARAM768:  return langRomTypeMegaRam768();
    case ROM_MEGARAM2M:   return langRomTypeMegaRam2mb();
    case ROM_EXTRAM:      return langRomTypeExtRam();
    case ROM_EXTRAM16KB:  return langRomTypeExtRam16();
    case ROM_EXTRAM32KB:  return langRomTypeExtRam32();
    case ROM_EXTRAM48KB:  return langRomTypeExtRam48();
    case ROM_EXTRAM64KB:  return langRomTypeExtRam64();
    case ROM_EXTRAM512KB: return langRomTypeExtRam512();
    case ROM_EXTRAM1MB:   return langRomTypeExtRam1mb();
    case ROM_EXTRAM2MB:   return langRomTypeExtRam2mb();
    case ROM_EXTRAM4MB:   return langRomTypeExtRam4mb();
    case ROM_MSXMUSIC:    return langRomTypeMsxMusic();
    case ROM_MSXAUDIO:    return langRomTypeMsxAudio();
    case ROM_MOONSOUND:   return langRomTypeMoonsound();
    case ROM_SVI328:      return langRomTypeSvi328Cart();
    case ROM_SVI328FDC:   return langRomTypeSvi328Fdc();
    case ROM_SVI328PRN:   return langRomTypeSvi328Prn();
    case ROM_SVI328RS232: return langRomTypeSvi328Uart();
    case ROM_SVI80COL:    return langRomTypeSvi328col80();
    case ROM_SVI328RSIDE: return langRomTypeSvi328RsIde();
    case ROM_SVI727:      return langRomTypeSvi727col80();
    case ROM_COLECO:      return langRomTypeColecoCart();
    case ROM_SG1000:      return langRomTypeSg1000Cart();
    case ROM_SC3000:      return langRomTypeSc3000Cart();
    case ROM_SG1000CASTLE:return langRomTypeTheCastle();
    case ROM_SEGABASIC:   return langRomTypeSegaBasic();
    case ROM_SONYHBI55:   return langRomTypeSonyHbi55();
    case ROM_MSXAUDIODEV: return langRomTypeY8950();
    case ROM_MSXPRN:      return langRomTypeMsxPrinter();
    case ROM_TURBORPCM:   return langRomTypeTurborPcm();
    case ROM_JOYREXPSG:   return "Joyrex PSG";
    case ROM_OPCODEPSG:   return "Opcode PSG";
    case ROM_GAMEREADER:  return langRomTypeGameReader();
    case ROM_SUNRISEIDE:  return langRomTypeSunriseIde();
    case ROM_BEERIDE:     return langRomTypeBeerIde();
    case ROM_NMS1210:     return "Philips NMS1210 Serial Interface";
    case ROM_GIDE:        return langRomTypeGide();
    case ROM_MICROSOL80:  return langRomTypeVmx80();
    case ROM_NMS8280DIGI: return langRomTypeNms8280Digitiz();
    case ROM_SONYHBIV1:   return langRomTypeHbiV1Digitiz();
    case ROM_PLAYBALL:    return langRomTypePlayBall();
    case ROM_FMDAS:       return langRomTypeFmdas();
    case ROM_YAMAHASFG01: return langRomTypeSfg01();
    case ROM_YAMAHASFG05: return langRomTypeSfg05();
    case ROM_YAMAHANET:   return "Yamaha Net";
    case ROM_SF7000IPL:   return "SF-7000 IPL";
    case ROM_OPCODEBIOS:  return "ColecoVision Opcode Bios";
    case ROM_OPCODEMEGA:  return "ColecoVision Opcode MegaRam";
    case ROM_OPCODESAVE:  return "ColecoVision Opcode SaveRam";
    case ROM_OPCODESLOT:  return "ColecoVision Opcode Slot Manager";
    case ROM_CVMEGACART:  return "ColecoVision MegaCart(R)";
    case SRAM_MEGASCSI:   return langRomTypeMegaSCSI();
    case SRAM_MEGASCSI128:return langRomTypeMegaSCSI128();
    case SRAM_MEGASCSI256:return langRomTypeMegaSCSI256();
    case SRAM_MEGASCSI512:return langRomTypeMegaSCSI512();
    case SRAM_MEGASCSI1MB:return langRomTypeMegaSCSI1mb();
    case SRAM_ESERAM:     return langRomTypeEseRam();
    case SRAM_ESERAM128:  return langRomTypeEseRam128();
    case SRAM_ESERAM256:  return langRomTypeEseRam256();
    case SRAM_ESERAM512:  return langRomTypeEseRam512();
    case SRAM_ESERAM1MB:  return langRomTypeEseRam1mb();
    case SRAM_WAVESCSI:   return langRomTypeWaveSCSI();
    case SRAM_WAVESCSI128:return langRomTypeWaveSCSI128();
    case SRAM_WAVESCSI256:return langRomTypeWaveSCSI256();
    case SRAM_WAVESCSI512:return langRomTypeWaveSCSI512();
    case SRAM_WAVESCSI1MB:return langRomTypeWaveSCSI1mb();
    case SRAM_ESESCC:     return langRomTypeEseSCC();
    case SRAM_ESESCC128:  return langRomTypeEseSCC128();
    case SRAM_ESESCC256:  return langRomTypeEseSCC256();
    case SRAM_ESESCC512:  return langRomTypeEseSCC512();
    case ROM_GOUDASCSI:   return langRomTypeGoudaSCSI();

    case ROM_UNKNOWN:     return langTextUnknown();
    }

    return langTextUnknown();
}

extern "C" const char* romTypeToShortString(RomType romType) 
{
    switch (romType) {
    case ROM_STANDARD:    return "STANDARD";
    case ROM_MSXDOS2:     return "MSXDOS2";
    case ROM_KONAMI5:     return "KONAMI SCC";
    case ROM_MANBOW2:     return "MANBOW 2";
    case ROM_MEGAFLSHSCC: return "MEGAFLSHSCC";
    case ROM_OBSONET:     return "OBSONET";
    case ROM_DUMAS:       return "DUMAS";
    case ROM_NOWIND:      return "NOWIND";
    case ROM_KONAMI4:     return "KONAMI";
    case ROM_ASCII8:      return "ASCII8";
    case ROM_ASCII16:     return "ASCII16";
    case ROM_GAMEMASTER2: return "GMASTER2";
    case ROM_ASCII8SRAM:  return "ASCII8SRAM";
    case ROM_ASCII16SRAM: return "ASCII16SRAM";
    case ROM_RTYPE:       return "R-TYPE";
    case ROM_CROSSBLAIM:  return "CROSSBLAIM";
    case ROM_HARRYFOX:    return "HARRYFOX";
    case ROM_KOREAN80:    return "ZEM 80IN1";
    case ROM_KOREAN126:   return "ZEM 126IN1";
    case ROM_KOREAN90:    return "ZEM 90IN1";
    case ROM_SCC:         return "SCC";
    case ROM_SCCPLUS:     return "SCC-I";
    case ROM_SNATCHER:    return "SNATCHER";
    case ROM_SDSNATCHER:  return "SDSNATCHER";
    case ROM_SCCMIRRORED: return "SCCMIRRORED";
    case ROM_SCCEXTENDED: return "SCCEXTENDED";
    case ROM_FMPAC:       return "FMPAC";
    case ROM_FMPAK:       return "FMPAK";
    case ROM_KONAMI4NF:   return "KONAMI GEN";
    case ROM_ASCII16NF:   return "SUPERPIERR";
    case ROM_PLAIN:       return "MIRRORED";
    case ROM_NETTOUYAKYUU:return "NETTOU YAKYUU";
    case ROM_MATRAINK:    return "MATRA INK";
    case ROM_FORTEII:     return "FORTE II";
    case ROM_NORMAL:      return "NORMAL";
    case ROM_DRAM:        return "DRAM";
    case ROM_DISKPATCH:   return "DISKPATCH";
    case ROM_CASPATCH:    return "CASPATCH";
    case ROM_TC8566AF:    return "TC8566AF";
    case ROM_TC8566AF_TR: return "TC8566AF";
    case ROM_MICROSOL:    return "MICROSOL";
    case ROM_ARC:         return "ARC";
    case ROM_NATIONALFDC: return "NATNL FDC";
    case ROM_PHILIPSFDC:  return "PHILIPSFDC";
    case ROM_SVI738FDC:   return "SVI738 FDC";
    case RAM_MAPPER:      return "MAPPED RAM";
    case RAM_1KB_MIRRORED:return "1K MIR RAM";
    case RAM_2KB_MIRRORED:return "2K MIR RAM";
    case RAM_NORMAL:      return "NORMAL RAM";
    case ROM_KANJI:       return "KANJI";
    case ROM_HOLYQURAN:   return "HOLYQURAN";
    case SRAM_MATSUCHITA:     return "MATSUSHITA";
    case SRAM_MATSUCHITA_INV: return "MATSUS INV";
    case ROM_PANASONICWX16:   return "PANASON 16";
    case ROM_PANASONIC16: return "PANASON 16";
    case ROM_PANASONIC32: return "PANASON 32";
    case ROM_BUNSETU:     return "BUNSETSU";
    case ROM_JISYO:       return "JISYO";
    case ROM_KANJI12:     return "KANJI12";
    case ROM_NATIONAL:    return "NATIONAL";
    case SRAM_S1985:      return "S1985";
    case ROM_S1990:       return "S1990";
    case ROM_TURBORIO:    return "TR PAUSE";
    case ROM_F4DEVICE:    return "F4NORMAL";
    case ROM_F4INVERTED:  return "F4INV";
    case ROM_MSXMIDI:     return "MSX-MIDI";
    case ROM_TURBORTIMER: return "TURBORTMR";
    case ROM_KOEI:        return "KOEI";
    case ROM_BASIC:       return "BASIC";
    case ROM_HALNOTE:     return "HALNOTE";
    case ROM_LODERUNNER:  return "LODERUNNER";
    case ROM_0x4000:      return "4000h";
    case ROM_0xC000:      return "C000h";
    case ROM_KONAMISYNTH: return "KONSYNTH";
    case ROM_KONAMKBDMAS: return "KBDMASTER";
    case ROM_KONWORDPRO:  return "KONWORDPRO";
    case ROM_MAJUTSUSHI:  return "MAJUTSUSHI";
    case ROM_PAC:         return "PAC";
    case ROM_MEGARAM:     return "MEGARAM";
    case ROM_MEGARAM128:  return "MEGARAM128";
    case ROM_MEGARAM256:  return "MEGARAM256";
    case ROM_MEGARAM512:  return "MEGARAM512";
    case ROM_MEGARAM768:  return "MEGARAM768";
    case ROM_MEGARAM2M:   return "MEGARAM2MB";
    case ROM_EXTRAM:      return "EXTERN RAM";
    case ROM_EXTRAM16KB:  return "EXTRAM 16";
    case ROM_EXTRAM32KB:  return "EXTRAM 32";
    case ROM_EXTRAM48KB:  return "EXTRAM 48";
    case ROM_EXTRAM64KB:  return "EXTRAM 64";
    case ROM_EXTRAM512KB: return "EXTRAM 512";
    case ROM_EXTRAM1MB:   return "EXTRAM 2MB";
    case ROM_EXTRAM2MB:   return "EXTRAM 1MB";
    case ROM_EXTRAM4MB:   return "EXTRAM 4MB";
    case ROM_MSXMUSIC:    return "MSXMUSIC";
    case ROM_MSXAUDIO:    return "MSXAUDIO";
    case ROM_MOONSOUND:   return "MOONSOUND";
    case ROM_SVI328:      return "SVI328";
    case ROM_SVI328FDC:   return "SVI328FDC";
    case ROM_SVI328PRN:   return "SVI328PRN";
    case ROM_SVI328RS232: return "SVI328RS232";
    case ROM_SVI80COL:    return "SVI80COL";
    case ROM_SVI328RSIDE: return "SVI328RSIDE";
    case ROM_SVI727:      return "SVI727";
    case ROM_COLECO:      return "COLECO";
    case ROM_SG1000:      return "SG-1000";
    case ROM_SC3000:      return "SC-3000";
    case ROM_SG1000CASTLE:return "THECASTLE";
    case ROM_SEGABASIC:   return "SEGABASIC";
    case ROM_SONYHBI55:   return "HBI-55";
    case ROM_MSXAUDIODEV: return "MSXAUDIO";
    case ROM_MSXPRN:      return "MSXPRN";
    case ROM_TURBORPCM:   return "TURBOR PCM";
    case ROM_JOYREXPSG:   return "JOYREX PSG";
    case ROM_OPCODEPSG:   return "OPCODE PSG";
    case ROM_OPCODEBIOS:  return "OPCODE BIOS";
    case ROM_OPCODEMEGA:  return "OPCODE MEGA";
    case ROM_OPCODESAVE:  return "OPCODE SAVE";
    case ROM_OPCODESLOT:  return "OPCODE SLOT";
    case ROM_GAMEREADER:  return "GAMEREADER";
    case ROM_SUNRISEIDE:  return "SUNRISEIDE";
    case ROM_BEERIDE:     return "BEER IDE";
    case ROM_NMS1210:     return "NMS1210";
    case ROM_GIDE:        return "GIDE";
    case ROM_MICROSOL80:  return "MICROSOL80";
    case ROM_NMS8280DIGI: return "8280 DIGI";
    case ROM_SONYHBIV1:   return "SONY HBI-V1";
    case ROM_PLAYBALL:    return "PLAYBALL";
    case ROM_FMDAS:       return "FM-DAS";
    case ROM_YAMAHASFG01: return "SFG-01";
    case ROM_YAMAHASFG05: return "SFG-05";
    case ROM_YAMAHANET:   return "YAMAHA NET";
    case ROM_SF7000IPL:   return "SF-7000 IPL";
    case ROM_CVMEGACART:  return "MEGACART";
    case SRAM_MEGASCSI:   return "MEGASCSI";
    case SRAM_MEGASCSI128:return "MEGASCSI128";
    case SRAM_MEGASCSI256:return "MEGASCSI256";
    case SRAM_MEGASCSI512:return "MEGASCSI512";
    case SRAM_MEGASCSI1MB:return "MEGASCSI1MB";
    case SRAM_ESERAM:     return "ESE-RAM";
    case SRAM_ESERAM128:  return "ESE-RAM128";
    case SRAM_ESERAM256:  return "ESE-RAM256";
    case SRAM_ESERAM512:  return "ESE-RAM512";
    case SRAM_ESERAM1MB:  return "ESE-RAM1MB";
    case SRAM_WAVESCSI:   return "WAVESCSI";
    case SRAM_WAVESCSI128:return "WAVESCSI128";
    case SRAM_WAVESCSI256:return "WAVESCSI256";
    case SRAM_WAVESCSI512:return "WAVESCSI512";
    case SRAM_WAVESCSI1MB:return "WAVESCSI1MB";
    case SRAM_ESESCC:     return "ESE-SCC";
    case SRAM_ESESCC128:  return "ESE-SCC128";
    case SRAM_ESESCC256:  return "ESE-SCC256";
    case SRAM_ESESCC512:  return "ESE-SCC512";
    case ROM_GOUDASCSI:   return "GOUDA SCSI";

    case ROM_UNKNOWN:     return "UNKNOWN";
    }

    return "UNKNOWN";
}

int romTypeIsRom(RomType romType) {
    switch (romType) {
    case ROM_SCC:         return 1;
    case ROM_SCCPLUS:     return 1;
    case ROM_MOONSOUND:   return 1;
    case ROM_SNATCHER:    return 1;
    case ROM_SDSNATCHER:  return 1;
    case ROM_SCCMIRRORED: return 1;
    case ROM_SCCEXTENDED: return 1;
    case ROM_PLAIN:       return 1;
    case ROM_NETTOUYAKYUU:return 1;
    case ROM_MATRAINK:    return 1;
    case ROM_FORTEII:     return 1;
    case ROM_FMPAK:       return 1;
    case ROM_NORMAL:      return 1;
    case ROM_DRAM:        return 1;
    case ROM_DISKPATCH:   return 1;
    case ROM_CASPATCH:    return 1;
    case ROM_MICROSOL:    return 1;
    case ROM_ARC:         return 1;
    case ROM_NATIONALFDC: return 1;
    case ROM_PHILIPSFDC:  return 1;
    case ROM_SVI738FDC:   return 1;
    case ROM_HOLYQURAN:   return 1;
    case SRAM_MATSUCHITA: return 1;
    case SRAM_MATSUCHITA_INV: return 1;
    case ROM_BASIC:       return 1;
    case ROM_0x4000:      return 1;
    case ROM_0xC000:      return 1;
    case ROM_KONAMISYNTH: return 1;
    case ROM_KONAMKBDMAS: return 1;
    case ROM_KONWORDPRO:  return 1;
    case ROM_MICROSOL80:  return 1;
    case ROM_SONYHBIV1:   return 1;
    case ROM_PLAYBALL:    return 1;
    case ROM_FMDAS:       return 1;
    case ROM_YAMAHASFG01: return 1;
    case ROM_YAMAHASFG05: return 1;
    case ROM_SF7000IPL:   return 1;
    case ROM_YAMAHANET:   return 1;
    case ROM_EXTRAM16KB:  return 1;
    case ROM_EXTRAM32KB:  return 1;
    case ROM_EXTRAM48KB:  return 1;
    case ROM_EXTRAM64KB:  return 1;
    }
    return 0;
}

int romTypeIsMegaRom(RomType romType) {
    switch (romType) {
    case ROM_STANDARD:    return 1;
    case ROM_MSXDOS2:     return 1;
    case ROM_KONAMI5:     return 1;
    case ROM_MANBOW2:     return 1;
    case ROM_MEGAFLSHSCC: return 1;
    case ROM_OBSONET:     return 1;
    case ROM_DUMAS:       return 1;
    case ROM_NOWIND:      return 1;
    case ROM_KONAMI4:     return 1;
    case ROM_ASCII8:      return 1;
    case ROM_ASCII16:     return 1;
    case ROM_GAMEMASTER2: return 1;
    case ROM_ASCII8SRAM:  return 1;
    case ROM_TC8566AF:    return 1;
    case ROM_TC8566AF_TR: return 1;
    case ROM_ASCII16SRAM: return 1;
    case ROM_RTYPE:       return 1;
    case ROM_CROSSBLAIM:  return 1;
    case ROM_HARRYFOX:    return 1;
    case ROM_KOREAN80:    return 1;
    case ROM_KOREAN126:   return 1;
    case ROM_KONAMI4NF:   return 1;
    case ROM_ASCII16NF:   return 1;
    case ROM_HOLYQURAN:   return 1;
    case ROM_MAJUTSUSHI:  return 1;
    case ROM_KOEI:        return 1;
    case ROM_HALNOTE:     return 1;
    case ROM_LODERUNNER:  return 1;
    case ROM_MSXAUDIO:    return 1;
    case ROM_KOREAN90:    return 1;
    case ROM_SONYHBI55:   return 1;
    case ROM_EXTRAM512KB: return 1;
    case ROM_EXTRAM1MB:   return 1;
    case ROM_EXTRAM2MB:   return 1;
    case ROM_EXTRAM4MB:   return 1;
    case ROM_GAMEREADER:  return 1;
    case ROM_SUNRISEIDE:  return 1;
    case ROM_BEERIDE:     return 1;
    case SRAM_MEGASCSI:   return 1;
    case SRAM_MEGASCSI128:return 1;
    case SRAM_MEGASCSI256:return 1;
    case SRAM_MEGASCSI512:return 1;
    case SRAM_MEGASCSI1MB:return 1;
    case SRAM_ESERAM:     return 1;
    case SRAM_ESERAM128:  return 1;
    case SRAM_ESERAM256:  return 1;
    case SRAM_ESERAM512:  return 1;
    case SRAM_ESERAM1MB:  return 1;
    case SRAM_WAVESCSI:   return 1;
    case SRAM_WAVESCSI128:return 1;
    case SRAM_WAVESCSI256:return 1;
    case SRAM_WAVESCSI512:return 1;
    case SRAM_WAVESCSI1MB:return 1;
    case SRAM_ESESCC:     return 1;
    case SRAM_ESESCC128:  return 1;
    case SRAM_ESESCC256:  return 1;
    case SRAM_ESESCC512:  return 1;
    }
    return 0;
}

int romTypeIsMegaRam(RomType romType) {
    switch (romType) {
    case ROM_MEGARAM:     return 1;
    case ROM_MEGARAM128:  return 1;
    case ROM_MEGARAM256:  return 1;
    case ROM_MEGARAM512:  return 1;
    case ROM_MEGARAM768:  return 1;
    case ROM_MEGARAM2M:   return 1;
    }
    return 0;
}

int romTypeIsFmPac(RomType romType) {
    switch (romType) {
    case ROM_FMPAC:       return 1;
    }
    return 0;
}

extern "C" void mediaDbLoad(const char* directory)
{
    if (romdb == NULL) {
        romdb = new MediaDb;
    }
    if (diskdb == NULL) {
        diskdb = new MediaDb;
    }
    if (casdb == NULL) {
        casdb = new MediaDb;
    }

    string path = directory;
    path += "/";

    string searchPath = path + "*.xml";

    ArchGlob* glob = archGlob(searchPath.c_str(), ARCH_GLOB_FILES);

    if (glob != NULL) {
        for (int i = 0; i < glob->count; i++) {
            mediaDbAddFromXmlFile(glob->pathVector[i]);
        }
        archGlobFree(glob);
    }
}

extern "C" MediaType* mediaDbLookupRom(const void *buffer, int size) 
{
    const char* romData = (const char*)buffer;
    static MediaType defaultColeco(ROM_COLECO, "Unknown Coleco rom");
    static MediaType defaultSvi(ROM_SVI328, "Unknown SVI rom");
    static MediaType defaultSg1000(ROM_SG1000, "Unknown SG-1000 rom");
    static MediaType defaultSc3000(ROM_SC3000, "Unknown SC-3000 rom");

    if (romdb == NULL) {
        return NULL;
    }
    MediaType* mediaType = mediaDbLookup(romdb, buffer, size);

    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0xF3 && romData[1] == 0x31)
    {
        mediaType = &defaultSvi;
    }

    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0x55 && (unsigned char)romData[1] == 0xAA) 
    {
        mediaType = &defaultColeco;
    }
#if 0
    if (mediaType == NULL &&
        size <= 0x8000 && (unsigned char)romData[0] == 0x55 && (unsigned char)romData[1] == 0xAA) 
    {
        mediaType = &defaultSg1000;
    }
#endif
    return mediaType;
}

extern "C" MediaType* mediaDbLookupDisk(const void *buffer, int size)
{
    if (diskdb == NULL) {
        return NULL;
    }
    return mediaDbLookup(diskdb, buffer, size);
}

extern "C" MediaType* mediaDbLookupCas(const void *buffer, int size)
{
    if (casdb == NULL) {
        return NULL;
    }
    return mediaDbLookup(casdb, buffer, size);
}

RomType mediaDbGetRomType(MediaType* mediaType)
{
    return mediaType->romType;
}

extern "C" const char* mediaDbGetTitle(MediaType* mediaType)
{
    return mediaType->title.c_str();
}

extern "C" const char* mediaDbGetYear(MediaType* mediaType)
{
    return mediaType->year.c_str();
}

extern "C" const char* mediaDbGetCompany(MediaType* mediaType)
{
    return mediaType->company.c_str();
}

extern "C" const char* mediaDbGetRemark(MediaType* mediaType)
{
    return mediaType->remark.c_str();
}

extern "C" const char* mediaDbGetPrettyString(MediaType* mediaType)
{
    static char prettyString[512];

    prettyString[0] = 0;

    if (mediaType != NULL) {
        strcat(prettyString, mediaType->title.c_str());
        if (mediaType->company.length() || mediaType->year.length() || mediaType->country.length()) {
            strcat(prettyString, " -");
        }
            
        if (mediaType->company.length()) {
            strcat(prettyString, " ");
            strcat(prettyString, mediaType->company.c_str());
        }

        if (mediaType->year.length()) {
            strcat(prettyString, " ");
            strcat(prettyString, mediaType->year.c_str());
        }

        if (mediaType->country.length()) {
            strcat(prettyString, " ");
            strcat(prettyString, mediaType->country.c_str());
        }

        if (mediaType->remark.length()) {
            std::string remark = " : ";
            for (int i = 0; mediaType->remark[i] != '\r' && mediaType->remark[i] != '\n' && mediaType->remark[i] != '\0'; i++) {
                remark += mediaType->remark[i];
            }
            int remarkLength = 35 - mediaType->start.length();
            if (remarkLength > 0) {
                if (remark.length() > 35) {
                    remark = remark.substr(0, 35) + "...";
                }
                strcat(prettyString, remark.c_str());
            }
        }
        
        if (mediaType->start.length()) {
            strcat(prettyString, " [ ");
            strcat(prettyString, mediaType->start.c_str());
            strcat(prettyString, " ]");
        }
    }

    return prettyString;
}


extern "C" void mediaDbSetDefaultRomType(RomType romType)
{
    romdbDefaultType = romType;
}

extern "C" MediaType* mediaDbGuessRom(const void *buffer, int size) 
{
    static MediaType staticMediaType(ROM_UNKNOWN, "Unknown MSX rom");

    const UInt8* romData = (const UInt8*)buffer;
    int i;
    int mapper;
    UInt32 counters[6] = { 0, 0, 0, 0, 0, 0 };

    staticMediaType.romType = romdbDefaultType;

    if (size < 128) {
        return &staticMediaType;
    }

    MediaType* mediaType = mediaDbLookupRom(buffer, size);
    if (mediaType == NULL) {
        mediaType = &staticMediaType;
//        printf("xx %d\n", romdbDefaultType);
    }

    if (mediaType->romType != ROM_UNKNOWN) {
    	printf("rom is in DB\n");
        return mediaType;
    }
    else
    	printf("rom not in DB\n");

    BoardType boardType = boardGetType();

    switch (boardType) {
    case BOARD_SVI:
        staticMediaType.romType = ROM_SVI328;
        return &staticMediaType;
    case BOARD_COLECO:
    case BOARD_COLECOADAM:
        staticMediaType.romType = ROM_COLECO;
        return &staticMediaType;
    case BOARD_SG1000:
        staticMediaType.romType = ROM_SG1000;
        return &staticMediaType;
    case BOARD_SC3000:
    case BOARD_SF7000:
        staticMediaType.romType = ROM_SC3000;
        return &staticMediaType;
    case BOARD_MSX_FORTE_II:
        break;
    case BOARD_MSX:
        break;
    }


	if (size <= 0x10000) {
		if (size == 0x10000) {
            if (romData[0x4000] == 'A' && romData[0x4001] == 'B') mediaType->romType = ROM_PLAIN;
            else mediaType->romType = ROM_ASCII16;
			return mediaType;
		} 
        
        if (size <= 0x4000 && romData[0] == 'A' && romData[1] == 'B') {
			UInt16 init = romData[2] + 256 * romData[3];
			UInt16 text = romData[8] + 256 * romData[9];
//			if (init == 0 && (text & 0xc000) == 0x8000) {
			if ((text & 0xc000) == 0x8000) {
                mediaType->romType = ROM_BASIC;
			    return mediaType;
			}
		}
        mediaType->romType = ROM_PLAIN;
		return mediaType;
	}
    
    const char ManbowTag[] = "Mapper: Manbow 2";
    UInt32 tagLength = strlen(ManbowTag);
    for (i = 0; i < (int)(size - tagLength); i++) {
        if (romData[i] == ManbowTag[0]) {
            if (memcmp(romData + i, ManbowTag, tagLength) == 0) {
                mediaType->romType = ROM_MANBOW2;
			    return mediaType;
            }
        }
    }

    /* Count occurences of characteristic addresses */
    for (i = 0; i < size - 3; i++) {
        if (romData[i] == 0x32) {
            UInt32 value = romData[i + 1] + ((UInt32)romData[i + 2] << 8);

            switch(value) {
            case 0x4000: 
            case 0x8000: 
            case 0xa000: 
                counters[3]++;
                break;

            case 0x5000: 
            case 0x9000: 
            case 0xb000: 
                counters[2]++;
                break;

            case 0x6000: 
                counters[3]++;
                counters[4]++;
                counters[5]++;
                break;

            case 0x6800: 
            case 0x7800: 
                counters[4]++;
                break;

            case 0x7000: 
                counters[2]++;
                counters[4]++;
                counters[5]++;
                break;

            case 0x77ff: 
                counters[5]++;
                break;
            }
        }
    }

    /* Find which mapper type got more hits */
    mapper = 0;

    counters[4] -= counters[4] ? 1 : 0;

	for (i = 0; i <= 5; i++) {
		if (counters[i] > 0 && counters[i] >= counters[mapper]) {
			mapper = i;
		}
	}

    if (mapper == 5 && counters[0] == counters[5]) {
		mapper = 0;
	}

    switch (mapper) {
    default:
    case 0: mediaType->romType = ROM_STANDARD; break;
    case 1: mediaType->romType = ROM_MSXDOS2; break;
    case 2: mediaType->romType = ROM_KONAMI5; break;
    case 3: mediaType->romType = ROM_KONAMI4; break;
    case 4: mediaType->romType = ROM_ASCII8; break;
    case 5: mediaType->romType = ROM_ASCII16; break;
    }
    
    return mediaType;
}


extern "C" void mediaDbCreateRomdb()
{
    if (romdb == NULL) {
        romdb = new MediaDb;
    }
}

extern "C" void mediaDbCreateDiskdb()
{
    if (diskdb == NULL) {
        diskdb = new MediaDb;
    }
}

extern "C" void mediaDbCreateCasdb()
{
    if (casdb == NULL) {
        casdb = new MediaDb;
    }
}

