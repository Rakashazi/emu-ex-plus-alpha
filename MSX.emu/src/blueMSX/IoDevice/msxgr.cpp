// -----------------------------------------------------------------------
// file:        msxgr.cpp
// version:     0.5d (October 22, 2005)
//
// description: This is an unofficial API to the MSX Game Reader. Most
//              of the interface is based on assumptions and tests.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2, or (at your option)
//  any later version. See COPYING for more details.
//
// Copyright 2005 Vincent van Dam (vincentd@erg.verweg.com)
// -----------------------------------------------------------------------

#include "msxgr.h"

#ifdef WII
// Not supported on WII
#else
int CMSXGr::Init()
{
	// Load DLL.
	hLib = LoadLibrary("MSXGr.dll");

	if (!hLib)	{
		// DLL could not be found.
		return -1;
	}

	_MSXGR_Init MSXGR_Init =
		(_MSXGR_Init)GetProcAddress(hLib, "MSXGR_Init");

	if (MSXGR_Init == NULL) {
		// MSXGR_Init method not found, assume corrupt DLL.
		return -1;
	}
	
	nLastError = MSXGR_Init();

	if (nLastError) {
		// Error during initialisation
		FreeLibrary(hLib);   
		return nLastError;
	}

	// initialiase funtion pointers
	MSXGR_Err2Str = (_MSXGR_Err2Str)GetProcAddress(hLib, "MSXGR_Err2Str");
	MSXGR_GetVersion = (_MSXGR_GetVersion)GetProcAddress(hLib, "MSXGR_GetVersion");
	MSXGR_SetDebugMode = (_MSXGR_SetDebugMode)GetProcAddress(hLib, "MSXGR_SetDebugMode");
	MSXGR_IsSlotEnable = (_MSXGR_IsSlotEnable)GetProcAddress(hLib, "MSXGR_IsSlotEnable");
	MSXGR_GetSlotStatus = (_MSXGR_GetSlotStatus)GetProcAddress(hLib, "MSXGR_GetSlotStatus");
	MSXGR_ReadMemory = (_MSXGR_ReadMemory)GetProcAddress(hLib, "MSXGR_ReadMemory");
	MSXGR_WriteMemory =	(_MSXGR_WriteMemory)GetProcAddress(hLib, "MSXGR_WriteMemory");
	MSXGR_WriteIO =	(_MSXGR_WriteIO)GetProcAddress(hLib, "MSXGR_WriteIO");
	MSXGR_ReadIO = (_MSXGR_ReadIO)GetProcAddress(hLib, "MSXGR_ReadIO");

	// Wait for the driver to attach the game reader(s)
	int nSlot;
	int nTry=0;
	while (nTry<5) {
		Sleep(300);
		for (nSlot=0;nSlot<16;nSlot++)
			if (IsSlotEnable(nSlot)) nTry = 5;
	}

	return 0;
}


void CMSXGr::Uninit()
{
	if (!hLib)	{
		// DLL not loaded
		return;
	}

	_MSXGR_Uninit MSXGR_Uninit =
		(_MSXGR_Uninit)GetProcAddress(hLib, "MSXGR_Uninit");

	// uninit msxgr
	if (MSXGR_Uninit) {
		MSXGR_Uninit();
	}

	// unload dll
	FreeLibrary(hLib);
	hLib = NULL;

	return;
}

char* CMSXGr::Err2Str(int nError)
{
	return MSXGR_Err2Str(nError);
}


char* CMSXGr::GetLastErrorStr()
{
	return Err2Str(nLastError);
}

int CMSXGr::GetVersion()
{
	return MSXGR_GetVersion();
}

void CMSXGr::SetDebugMode(int nLevel)
{
	MSXGR_SetDebugMode(nLevel);
	return;
}

bool CMSXGr::IsSlotEnable(int nSlot)
{
	return MSXGR_IsSlotEnable(nSlot);
}

int CMSXGr::GetSlotStatus(int nSlot,int *pBuffer)
{
	// pBuffer[0] = 01 slot enable
	// pBuffer[1] = ff cartridge inserted
	// pBuffer[2] = ?
	nLastError = MSXGR_GetSlotStatus(nSlot,pBuffer);
	return nLastError;
}

bool CMSXGr::IsCartridgeInserted(int nSlot)
{
	int Status[3];
	int nError = GetSlotStatus(nSlot,Status);
	return !nError && Status[1];
}

int CMSXGr::ReadMemory(int nSlot,char* pBuffer,int nAddress,int nLength)
{
	nLastError = MSXGR_ReadMemory(nSlot,pBuffer,nAddress,nLength);
	return nLastError;
}

int CMSXGr::WriteMemory(int nSlot,char* pBuffer,int nAddress,int nLength)
{
	nLastError = MSXGR_WriteMemory(nSlot,pBuffer,nAddress,nLength);
	return nLastError;
}

int CMSXGr::WriteIO(int nSlot,char* pBuffer,int nAddress,int nLength)
{
	nLastError = MSXGR_WriteIO(nSlot,pBuffer,nAddress,nLength);
	return nLastError;
}


int CMSXGr::ReadIO(int nSlot,char* pBuffer,int nAddress,int nLength)
{
	nLastError = MSXGR_ReadIO(nSlot,pBuffer,nAddress,nLength);
	return nLastError;
}
#endif

