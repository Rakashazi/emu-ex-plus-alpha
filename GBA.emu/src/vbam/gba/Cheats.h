#ifndef CHEATS_H
#define CHEATS_H

#include <gba/GBA.h>

struct CheatsData {
  int code;
  int size;
  int status;
  bool enabled;
  u32 rawaddress;
  u32 address;
  u32 value;
  u32 oldValue;
  char codestring[20];
  char desc[32];
};

void cheatsAdd(ARM7TDMI &cpu, const char *codeStr, const char *desc, u32 rawaddress, u32 address, u32 value, int code, int size);
void cheatsAddCheatCode(const char *code, const char *desc);
bool cheatsAddGSACode(ARM7TDMI &cpu, const char *code, const char *desc, bool v3);
bool cheatsAddCBACode(ARM7TDMI &cpu, const char *code, const char *desc);
bool cheatsImportGSACodeFile(ARM7TDMI &cpu, const char *name, int game, bool v3);
void cheatsDelete(ARM7TDMI &cpu, int number, bool restore);
void cheatsDeleteAll(ARM7TDMI &cpu, bool restore);
void cheatsEnable(int number);
void cheatsDisable(ARM7TDMI &cpu, int number);
void cheatsSaveGame(gzFile file);
void cheatsReadGame(gzFile file, int version);
void cheatsReadGameSkip(gzFile file, int version);
void cheatsSaveCheatList(const char *file);
bool cheatsLoadCheatList(const char *file);
void cheatsWriteMemory(u32 address, u32 value);
void cheatsWriteHalfWord(u32 address, u16 value);
void cheatsWriteByte(u32 address, u8 value);
int cheatsCheckKeys(ARM7TDMI &cpu, u32 keys, u32 extended);

extern int cheatsNumber;
extern CheatsData cheatsList[100];

#endif // CHEATS_H
