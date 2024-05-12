#include "core/gba/internal/gbaSram.h"

#include "core/gba/gbaFlash.h"
#include "core/gba/gba.h"

uint8_t sramRead(uint32_t address)
{
    return flashSaveMemory[address & 0xFFFF];
}
void sramDelayedWrite(uint32_t address, uint8_t byte)
{
    coreOptions.saveType = GBA_SAVE_SRAM;
    cpuSaveGameFunc = sramWrite;
    sramWrite(address, byte);
}

void sramWrite(uint32_t address, uint8_t byte)
{
    if((address & 0xFFFF) >= flashSaveMemory.size())
    {
    	log("out of bounds SRAM write @ 0x%X data:0x%X SRAM Size:0x%X", address, byte, flashSaveMemory.size());
      return;
    }
    flashSaveMemory[address & 0xFFFF] = byte;
    systemSaveUpdateCounter = SYSTEM_SAVE_UPDATED;
}
