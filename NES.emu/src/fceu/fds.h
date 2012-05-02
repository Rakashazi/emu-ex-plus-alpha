#pragma once

extern bool isFDS;
void FDSSoundReset(void);

void FCEU_FDSInsert(void);
//void FCEU_FDSEject(void);
void FCEU_FDSSelect(void);

void FCEU_FDSSetDisk(uint8 side);
bool FCEU_FDSInserted();
uint8 FCEU_FDSCurrentSide();
uint8 FCEU_FDSSides();
void FCEU_FDSWriteModifiedDisk();
