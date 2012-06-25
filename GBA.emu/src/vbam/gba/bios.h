#ifndef BIOS_H
#define BIOS_H

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
extern void BIOS_RegisterRamReset(ARM7TDMI &cpu, u32);
extern void BIOS_RLUnCompVram(ARM7TDMI &cpu);
extern void BIOS_RLUnCompWram(ARM7TDMI &cpu);
extern void BIOS_SoftReset(ARM7TDMI &cpu);
extern void BIOS_Sqrt(ARM7TDMI &cpu);
extern void BIOS_MidiKey2Freq(ARM7TDMI &cpu);
extern void BIOS_SndDriverJmpTableCopy(ARM7TDMI &cpu);

#endif // BIOS_H
