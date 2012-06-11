#pragma once

// Enables WIP poll-detection code, not yet working
static const bool extraCpuSync = 0;

// Special memory handler funcs

// Gate Array (and PCM for sub-CPU)

uint mainGateRead8(uint address);
uint mainGateRead16(uint address);
void mainGateWrite8(uint address, uint data);
void mainGateWrite16(uint address, uint data);
uint subGateRead8(uint address);
uint subGateRead16(uint address);
void subGateWrite8(uint address, uint data);
void subGateWrite16(uint address, uint data);

// PRG

void subPrgWriteProtectCheck8(uint address, uint data);
void subPrgWriteProtectCheck16(uint address, uint data);

// WORD

uint mainReadWordDecoded8(uint address);
uint mainReadWordDecoded16(uint address);
void mainWriteWordDecoded8(uint address, uint data);
void mainWriteWordDecoded16(uint address, uint data);
uint subReadWordDecoded8(uint address);
uint subReadWordDecoded16(uint address);
void subWriteWordDecoded8(uint address, uint data);
void subWriteWordDecoded16(uint address, uint data);

// SRAM cart

uint sramCartRead8(uint address);
uint sramCartRead16(uint address);
void sramCartWrite8(uint address, uint data);
void sramCartWrite16(uint address, uint data);

// SRAM write protect register

uint bcramRegRead8(uint address);
void bcramRegWrite8(uint address, uint data);

// SRAM cart size register handlers

uint sramCartRegRead8(uint address);
uint sramCartRegRead16(uint address);

// BRAM

uint bramRead8(uint address);
uint bramRead16(uint address);
void bramWrite8(uint address, uint data);
void bramWrite16(uint address, uint data);

// Unused/Undefined

uint nullRead8(uint address);
uint nullRead16(uint address);
uint subUndefRead8(uint address);
uint subUndefRead16(uint address);
void subUndefWrite8(uint address, uint data);
void subUndefWrite16(uint address, uint data);

// Memory map update funcs

void updateMainCpuPrgMap(SegaCD &sCD); // use bank from gate register
void updateMainCpuPrgMap(SegaCD &sCD, uint newBank);

void updateCpuWordMap(SegaCD &sCD); // use mode from gate register
void updateCpuWordMap(SegaCD &sCD, uint modeReg);

void updateMainCpuSramMap(SegaCD &sCD);
void updateMainCpuSramMap(SegaCD &sCD, uint bcramReg);
