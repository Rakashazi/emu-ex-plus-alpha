#ifndef VBAM_CORE_GBA_INTERNAL_GBABIOS_H_
#define VBAM_CORE_GBA_INTERNAL_GBABIOS_H_

#include <cstdint>

struct ARM7TDMI;

extern void BIOS_ArcTan(ARM7TDMI &cpu);
extern void BIOS_ArcTan2(ARM7TDMI &cpu);
extern void BIOS_BitUnPack(ARM7TDMI &cpu);
extern void BIOS_GetBiosChecksum(ARM7TDMI &cpu);
extern void BIOS_BgAffineSet(ARM7TDMI &cpu);
extern void BIOS_CpuSet(ARM7TDMI &cpu);
extern void BIOS_CpuFastSet(ARM7TDMI &cpu);
extern void BIOS_Diff8bitUnFilterWram(ARM7TDMI &cpu);
extern void BIOS_Diff8bitUnFilterVram(ARM7TDMI &cpu);
extern void BIOS_Diff16bitUnFilter(ARM7TDMI &cpu);
extern void BIOS_Div(ARM7TDMI &cpu);
extern void BIOS_DivARM(ARM7TDMI &cpu);
extern void BIOS_HuffUnComp(ARM7TDMI &cpu);
extern void BIOS_LZ77UnCompVram(ARM7TDMI &cpu);
extern void BIOS_LZ77UnCompWram(ARM7TDMI &cpu);
extern void BIOS_ObjAffineSet(ARM7TDMI &cpu);
extern void BIOS_RegisterRamReset(ARM7TDMI &cpu);
extern void BIOS_RegisterRamReset(ARM7TDMI &cpu, uint32_t);
extern void BIOS_RLUnCompVram(ARM7TDMI &cpu);
extern void BIOS_RLUnCompWram(ARM7TDMI &cpu);
extern void BIOS_SoftReset(ARM7TDMI &cpu);
extern void BIOS_Sqrt(ARM7TDMI &cpu);
extern void BIOS_MidiKey2Freq(ARM7TDMI &cpu);
extern void BIOS_SndDriverJmpTableCopy(ARM7TDMI &cpu);
extern void BIOS_SndDriverInit(ARM7TDMI &cpu);
extern void BIOS_SndDriverMode(ARM7TDMI &cpu);
extern void BIOS_SndDriverMain(ARM7TDMI &cpu);
extern void BIOS_SndDriverVSync(ARM7TDMI &cpu);
extern void BIOS_SndDriverVSyncOff(ARM7TDMI &cpu);
extern void BIOS_SndDriverVSyncOn(ARM7TDMI &cpu);
extern void BIOS_SndChannelClear(ARM7TDMI &cpu);

#endif // VBAM_CORE_GBA_INTERNAL_GBABIOS_H_
