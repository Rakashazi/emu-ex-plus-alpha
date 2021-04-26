#pragma once

// Enables WIP poll-detection code, not yet working
static const bool extraCpuSync = 0;

// Special memory handler funcs

// Gate Array (and PCM for sub-CPU)

unsigned mainGateRead8(unsigned address);
unsigned mainGateRead16(unsigned address);
void mainGateWrite8(unsigned address, unsigned data);
void mainGateWrite16(unsigned address, unsigned data);
unsigned subGateRead8(unsigned address);
unsigned subGateRead16(unsigned address);
void subGateWrite8(unsigned address, unsigned data);
void subGateWrite16(unsigned address, unsigned data);

// PRG

void subPrgWriteProtectCheck8(unsigned address, unsigned data);
void subPrgWriteProtectCheck16(unsigned address, unsigned data);

// WORD

unsigned mainReadWordDecoded8(unsigned address);
unsigned mainReadWordDecoded16(unsigned address);
void mainWriteWordDecoded8(unsigned address, unsigned data);
void mainWriteWordDecoded16(unsigned address, unsigned data);
unsigned subReadWordDecoded8(unsigned address);
unsigned subReadWordDecoded16(unsigned address);
void subWriteWordDecoded8(unsigned address, unsigned data);
void subWriteWordDecoded16(unsigned address, unsigned data);

// SRAM cart

unsigned sramCartRead8(unsigned address);
unsigned sramCartRead16(unsigned address);
void sramCartWrite8(unsigned address, unsigned data);
void sramCartWrite16(unsigned address, unsigned data);

// SRAM write protect register

unsigned bcramRegRead8(unsigned address);
void bcramRegWrite8(unsigned address, unsigned data);

// SRAM cart size register handlers

unsigned sramCartRegRead8(unsigned address);
unsigned sramCartRegRead16(unsigned address);

// BRAM

unsigned bramRead8(unsigned address);
unsigned bramRead16(unsigned address);
void bramWrite8(unsigned address, unsigned data);
void bramWrite16(unsigned address, unsigned data);

// Unused/Undefined

unsigned nullRead8(unsigned address);
unsigned nullRead16(unsigned address);
unsigned subUndefRead8(unsigned address);
unsigned subUndefRead16(unsigned address);
void subUndefWrite8(unsigned address, unsigned data);
void subUndefWrite16(unsigned address, unsigned data);

// Memory map update funcs

void updateMainCpuPrgMap(SegaCD &sCD); // use bank from gate register
void updateMainCpuPrgMap(SegaCD &sCD, unsigned newBank);

void updateCpuWordMap(SegaCD &sCD); // use mode from gate register
void updateCpuWordMap(SegaCD &sCD, unsigned modeReg);

void updateMainCpuSramMap(SegaCD &sCD);
void updateMainCpuSramMap(SegaCD &sCD, unsigned bcramReg);
