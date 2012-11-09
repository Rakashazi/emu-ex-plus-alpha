#ifndef RTC_H
#define RTC_H

u16 rtcRead(GBASys &gba, u32 address);
bool rtcWrite(u32 address, u16 value);
void rtcEnable(bool);
bool rtcIsEnabled();
void rtcReset();

void rtcReadGame(gzFile gzFile);
void rtcSaveGame(gzFile gzFile);

#endif // RTC_H
