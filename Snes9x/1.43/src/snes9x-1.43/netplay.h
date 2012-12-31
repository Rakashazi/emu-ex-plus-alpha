/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#ifndef _NETPLAY_H_
#define _NETPLAY_H_

/*
 * Client to server joypad update
 *
 * magic        1
 * sequence_no  1
 * opcode       1
 * joypad data  4
 *
 * Server to client joypad update
 * magic        1
 * sequence_no  1
 * opcode       1 + num joypads (top 3 bits)
 * joypad data  4 * n
 */

//#define NP_DEBUG 1

#define NP_VERSION 10
#define NP_JOYPAD_HIST_SIZE 120
#define NP_DEFAULT_PORT 6096

#define NP_MAX_CLIENTS 5

#define NP_SERV_MAGIC 'S'
#define NP_CLNT_MAGIC 'C'

#define NP_CLNT_HELLO 0
#define NP_CLNT_JOYPAD 1
#define NP_CLNT_RESET 2
#define NP_CLNT_PAUSE 3
#define NP_CLNT_LOAD_ROM 4
#define NP_CLNT_ROM_IMAGE 5
#define NP_CLNT_FREEZE_FILE 6
#define NP_CLNT_SRAM_DATA 7
#define NP_CLNT_READY 8
#define NP_CLNT_LOADED_ROM 9
#define NP_CLNT_RECEIVED_ROM_IMAGE 10
#define NP_CLNT_WAITING_FOR_ROM_IMAGE 11

#define NP_SERV_HELLO 0
#define NP_SERV_JOYPAD 1
#define NP_SERV_RESET 2
#define NP_SERV_PAUSE 3
#define NP_SERV_LOAD_ROM 4
#define NP_SERV_ROM_IMAGE 5
#define NP_SERV_FREEZE_FILE 6
#define NP_SERV_SRAM_DATA 7
#define NP_SERV_READY 8

struct SNPClient
{
    volatile uint8 SendSequenceNum;
    volatile uint8 ReceiveSequenceNum;
    volatile bool8 Connected;
    volatile bool8 SaidHello;
    volatile bool8 Paused;
    volatile bool8 Ready;
    int Socket;
    char *ROMName;
    char *HostName;
    char *Who;
};

enum {
    NP_SERVER_SEND_ROM_IMAGE,
    NP_SERVER_SYNC_ALL,
    NP_SERVER_SYNC_CLIENT,
    NP_SERVER_SEND_FREEZE_FILE_ALL,
    NP_SERVER_SEND_ROM_LOAD_REQUEST_ALL,
    NP_SERVER_RESET_ALL,
    NP_SERVER_SEND_SRAM_ALL,
    NP_SERVER_SEND_SRAM
};

#define NP_MAX_TASKS 20

struct NPServerTask
{
    uint32 Task;
    void  *Data;
};

struct SNPServer
{
    struct SNPClient Clients [NP_MAX_CLIENTS];
    int    NumClients;
    volatile struct NPServerTask TaskQueue [NP_MAX_TASKS];
    volatile uint32 TaskHead;
    volatile uint32 TaskTail;
    int    Socket;
    uint32 FrameTime;
    uint32 FrameCount;
    char   ROMName [30];
    uint32 Joypads [5];
    bool8  ClientPaused;
    uint32 Paused;
    bool8  SendROMImageOnConnect;
    bool8  SyncByReset;
};

#define NP_MAX_ACTION_LEN 200

struct SNetPlay
{
    volatile uint8  MySequenceNum;
    volatile uint8  ServerSequenceNum;
    volatile bool8  Connected;
    volatile bool8  Abort;
    volatile uint8  Player;
    volatile bool8  ClientsReady [NP_MAX_CLIENTS];
    volatile bool8  ClientsPaused [NP_MAX_CLIENTS];
    volatile bool8  Paused;
    volatile bool8  PendingWait4Sync;
    volatile uint8  PercentageComplete;
    volatile bool8  Waiting4EmulationThread;
    volatile bool8  Answer;
#ifdef __WIN32__
    HANDLE          ReplyEvent;
#endif
    volatile int    Socket;
    char *ServerHostName;
    char *ROMName;
    int Port;
    volatile uint32 JoypadWriteInd;
    volatile uint32 JoypadReadInd;
    uint32 Joypads [NP_JOYPAD_HIST_SIZE][NP_MAX_CLIENTS];
    uint32 Frame [NP_JOYPAD_HIST_SIZE];
    uint32 FrameCount;
    uint32 MaxFrameSkip;
    uint32 MaxBehindFrameCount;
    char   ActionMsg [NP_MAX_ACTION_LEN];
    char   ErrorMsg [NP_MAX_ACTION_LEN];
    char   WarningMsg [NP_MAX_ACTION_LEN];
};

extern "C" struct SNetPlay NetPlay;

//
// NETPLAY_CLIENT_HELLO message format:
// header
// frame_time (4)
// ROMName (variable)

#define WRITE_LONG(p, v) { \
*((p) + 0) = (uint8) ((v) >> 24); \
*((p) + 1) = (uint8) ((v) >> 16); \
*((p) + 2) = (uint8) ((v) >> 8); \
*((p) + 3) = (uint8) ((v) >> 0); \
}

#define READ_LONG(p) \
((((uint8) *((p) + 0)) << 24) | \
 (((uint8) *((p) + 1)) << 16) | \
 (((uint8) *((p) + 2)) <<  8) | \
 (((uint8) *((p) + 3)) <<  0))

bool8 S9xNPConnectToServer (const char *server_name, int port,
                            const char *rom_name);
bool8 S9xNPWaitForHeartBeat ();
uint32 S9xNPGetJoypad (int which1);
bool8 S9xNPSendJoypadUpdate (uint32 joypad);
void S9xNPDisconnect ();
bool8 S9xNPInitialise ();
bool8 S9xNPSendData (int fd, const uint8 *data, int len);
bool8 S9xNPGetData (int fd, uint8 *data, int len);

void S9xNPSyncClients ();
void S9xNPStepJoypadHistory ();

void S9xNPResetJoypadReadPos ();
bool8 S9xNPSendReady (uint8 op = NP_CLNT_READY);
bool8 S9xNPSendPause (bool8 pause);
void S9xNPReset ();
void S9xNPSetAction (const char *action, bool8 force = FALSE);
void S9xNPSetError (const char *error);
void S9xNPSetWarning (const char *warning);
void S9xNPDiscardHeartbeats ();
void S9xNPServerQueueSendingFreezeFile (const char *filename);
void S9xNPServerQueueSyncAll ();
void S9xNPServerQueueSendingROMImage ();
void S9xNPServerQueueSendingLoadROMRequest (const char *filename);

void S9xNPServerAddTask (uint32 task, void *data);

bool8 S9xNPStartServer (int port);
void S9xNPStopServer ();
#ifdef __WIN32__
#define S9xGetMilliTime timeGetTime
#else
uint32 S9xGetMilliTime ();
#endif
#endif

