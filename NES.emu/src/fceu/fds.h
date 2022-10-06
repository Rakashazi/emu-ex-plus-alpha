namespace EmuEx
{
class NesSystem;
}

extern bool isFDS;
void FDSSoundReset(void);

void FCEU_FDSInsert(void);
//void FCEU_FDSEject(void);
void FCEU_FDSSelect(void);

void FCEU_FDSSetDisk(uint8 side, EmuEx::NesSystem &);
bool FCEU_FDSInserted();
uint8 FCEU_FDSCurrentSide();
uint8 FCEU_FDSSides();
void FCEU_FDSReadModifiedDisk();
void FCEU_FDSWriteModifiedDisk();
