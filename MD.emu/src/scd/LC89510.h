/***********************************************************
 *                                                         *
 * This source was taken from the Gens project             *
 * Written by Stéphane Dallongeville                       *
 * Copyright (c) 2002 by Stéphane Dallongeville            *
 * Modified/adapted for PicoDrive by notaz, 2007           *
 *                                                         *
 ***********************************************************/

#pragma once

#ifdef B0
#undef B0
#endif

struct CDC
{
	constexpr CDC(): Buffer CXX11_INIT_LIST({0}), COMIN(0), IFSTAT(0),
			SBOUT(0), IFCTRL(0), Decode_Reg_Read(0) { }
	unsigned char Buffer[(32 * 1024 * 2) + 2352];
//	unsigned int Host_Data;		// unused
//	unsigned int DMA_Adr;		// 0A
//	unsigned int Stop_Watch;	// 0C
	unsigned int COMIN;
	unsigned int IFSTAT;
	union DBC_T
	{
		constexpr DBC_T(): N(0) { }
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} DBC;
	union DAC_T
	{
		constexpr DAC_T(): N(0) { }
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} DAC;
	union HEAD_T
	{
		constexpr HEAD_T(): N(0) { }
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} HEAD;
	union PT_T
	{
		constexpr PT_T(): N(0) { }
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} PT;
	union WA_T
	{
		constexpr WA_T(): N(0) { }
		struct
		{
			unsigned char L;
			unsigned char H;
			unsigned short unused;
		} B;
		int N;
	} WA;
	union STAT_T
	{
		constexpr STAT_T(): N(0) { }
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} STAT;
	unsigned int SBOUT;
	unsigned int IFCTRL;
	union CTRL_T
	{
		constexpr CTRL_T(): N(0) { }
		struct
		{
			unsigned char B0;
			unsigned char B1;
			unsigned char B2;
			unsigned char B3;
		} B;
		unsigned int N;
	} CTRL;
	unsigned int Decode_Reg_Read;
};

struct CDD
{
	constexpr CDD(): status(0), Minute(0), Seconde(0), Frame(0),
			Ext(0) { }
//	unsigned short Fader;	// 34
//	unsigned short Control;	// 36
//	unsigned short Cur_Comm;// unused

	// "Receive status"
	unsigned short status;
	unsigned short Minute;
	unsigned short Seconde;
	unsigned short Frame;
	unsigned char  Ext;
};


unsigned short Read_CDC_Host(int is_sub);
void LC89510_Reset(void);
void Update_CDC_TRansfer(int which);
void CDC_Update_Header(void);

unsigned char CDC_Read_Reg(void);
void CDC_Write_Reg(unsigned char Data);

void CDD_Export_Status(void);
void CDD_Import_Command(void);
