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
#include <string.h>
#include "snes9x.h"
#include "srtc.h"
#include "memmap.h"

/***   The format of the rtc_data structure is:

Index Description     Range (nibble)
----- --------------  ---------------------------------------

  0   Seconds low     0-9
  1   Seconds high    0-5

  2   Minutes low     0-9
  3   Minutes high    0-5

  4   Hour low        0-9
  5   Hour high       0-2

  6   Day low         0-9
  7   Day high        0-3

  8   Month           1-C (0xC is December, 12th month)

  9   Year ones       0-9
  A   Year tens       0-9
  B   Year High       9-B  (9=19xx, A=20xx, B=21xx)

  C   Day of week     0-6  (0=Sunday, 1=Monday,...,6=Saturday)

***/

SRTC_DATA           rtc;


static int month_keys[12] = { 1, 4, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6 };


/*********************************************************************************************
 *
 * Note, if you are doing a save state for this game:
 *
 * On save:
 *
 *	Call S9xUpdateSrtcTime and save the rtc data structure.
 *
 * On load:
 *
 *	restore the rtc data structure
 *      rtc.system_timestamp = time (NULL);
 *        
 *
 *********************************************************************************************/


void S9xResetSRTC ()
{
    rtc.index = -1;
    rtc.mode = MODE_READ;
}

void S9xHardResetSRTC ()
{
    ZeroMemory (&rtc, sizeof (rtc));
    rtc.index = -1;
    rtc.mode = MODE_READ;
    rtc.count_enable = FALSE;
    rtc.needs_init = TRUE;

    // Get system timestamp
    rtc.system_timestamp = time (NULL);
}

/**********************************************************************************************/
/* S9xSRTCComputeDayOfWeek()                                                                  */
/* Return 0-6 for Sunday-Saturday                                                             */
/**********************************************************************************************/
unsigned int    S9xSRTCComputeDayOfWeek ()
{
    unsigned    year = rtc.data[10]*10 + rtc.data[9];
    unsigned    month = rtc.data[8];
    unsigned    day = rtc.data[7]*10 + rtc.data[6];
    unsigned    day_of_week;

    year += (rtc.data[11] - 9) * 100;

    // Range check the month for valid array indicies
    if ( month > 12 )
        month = 1;

    day_of_week = year + (year / 4) + month_keys[month-1] + day - 1;

    if(( year % 4 == 0 ) && ( month <= 2 ) )
        day_of_week--;

    day_of_week %= 7;

    return day_of_week;
}


/**********************************************************************************************/
/* S9xSRTCDaysInMonth()                                                                       */
/* Return the number of days in a specific month for a certain year                           */
/**********************************************************************************************/
int	S9xSRTCDaysInMmonth( int month, int year )
{
    int		mdays;

    switch ( month )
    {
	case 2:
		if ( ( year % 4 == 0 ) )    // DKJM2 only uses 199x - 22xx
			mdays = 29;
		else
			mdays = 28;
		break;

	case 4:
	case 6:
	case 9:
	case 11:
		mdays = 30;
		break;

	default:	// months 1,3,5,7,8,10,12
		mdays = 31;
		break;
    }

    return mdays;
}


#define DAYTICKS (60*60*24)
#define HOURTICKS (60*60)
#define MINUTETICKS 60


/**********************************************************************************************/
/* S9xUpdateSrtcTime()                                                                        */
/* Advance the  S-RTC time if counting is enabled                                             */
/**********************************************************************************************/
void	S9xUpdateSrtcTime ()
{
	time_t	cur_systime;
	long    time_diff;

    // Keep track of game time by computing the number of seconds that pass on the system
    // clock and adding the same number of seconds to the S-RTC clock structure.
    // I originally tried using mktime and localtime library functions to keep track
    // of time but some of the GNU time functions fail when the year goes to 2099
    // (and maybe less) and this would have caused a bug with DKJM2 so I'm doing
    // it this way to get around that problem.

    // Note: Dai Kaijyu Monogatari II only allows dates in the range 1996-21xx.

    if (rtc.count_enable && !rtc.needs_init)
    {
        cur_systime = time (NULL);

        // This method assumes one time_t clock tick is one second
        //        which should work on PCs and GNU systems.
        //        If your tick interval is different adjust the
	//        DAYTICK, HOURTICK, and MINUTETICK defines

        time_diff = (long) (cur_systime - rtc.system_timestamp);
	rtc.system_timestamp = cur_systime;
        
        if ( time_diff > 0 )
        {
	   int		seconds;
	   int		minutes;
	   int		hours;
	   int		days;
	   int		month;
	   int		year;
	   int		temp_days;

	   int		year_hundreds;
	   int		year_tens;
	   int		year_ones;


	   if ( time_diff > DAYTICKS )
	   {
	       days = time_diff / DAYTICKS;
	       time_diff = time_diff - days * DAYTICKS;
	   }
	   else
	   {
	       days = 0;
	   }

	   if ( time_diff > HOURTICKS )
	   {
	       hours = time_diff / HOURTICKS;
	       time_diff = time_diff - hours * HOURTICKS;
	   }
	   else
	   {
	       hours = 0;
	   }

	   if ( time_diff > MINUTETICKS )
	   {
	       minutes = time_diff / MINUTETICKS;
	       time_diff = time_diff - minutes * MINUTETICKS;
	   }
	   else
	   {
	       minutes = 0;
	   }

	   if ( time_diff > 0 )
	   {
	       seconds = time_diff;
	   }
	   else
	   {
	       seconds = 0;
	   }


	   seconds += (rtc.data[1]*10 + rtc.data[0]);
           if ( seconds >= 60 )
	   {
	       seconds -= 60;
	       minutes += 1;
	   }

	   minutes += (rtc.data[3]*10 + rtc.data[2]);
           if ( minutes >= 60 )
	   {
	       minutes -= 60;
	       hours += 1;
	   }

	   hours += (rtc.data[5]*10 + rtc.data[4]);
           if ( hours >= 24 )
	   {
	       hours -= 24;
	       days += 1;
	   }

	   if ( days > 0 )
	   {
	       year =  rtc.data[10]*10 + rtc.data[9];
	       year += ( 1000 + rtc.data[11] * 100 );

	       month = rtc.data[8];
	       days += (rtc.data[7]*10 + rtc.data[6]);
	       while ( days > (temp_days = S9xSRTCDaysInMmonth( month, year )) )
               {
		    days -= temp_days;
		    month += 1;
		    if ( month > 12 )
		    {
		        year += 1;
		        month = 1;
		    }
	       }

               year_tens = year % 100;
               year_ones = year_tens % 10;
               year_tens /= 10;
               year_hundreds = (year - 1000) / 100;

	       rtc.data[6] = days % 10;
	       rtc.data[7] = days / 10;
	       rtc.data[8] = month;
	       rtc.data[9] = year_ones;
	       rtc.data[10] = year_tens;
	       rtc.data[11] = year_hundreds;
	       rtc.data[12] = S9xSRTCComputeDayOfWeek ();
	   }

	   rtc.data[0] = seconds % 10;
	   rtc.data[1] = seconds / 10;
	   rtc.data[2] = minutes % 10;
	   rtc.data[3] = minutes / 10;
	   rtc.data[4] = hours % 10;
	   rtc.data[5] = hours / 10;

	   return;
        }
    }
}


/**********************************************************************************************/
/* S9xSetSRTC()                                                                               */
/* This function sends data to the S-RTC used in Dai Kaijyu Monogatari II                     */
/**********************************************************************************************/
void S9xSetSRTC (uint8 data, uint16 Address)
{

    data &= 0x0F;	// Data is only 4-bits, mask out unused bits.

    if( data >= 0xD )
    {
        // It's an RTC command

        switch ( data )
        {
            case 0xD:
                rtc.mode = MODE_READ;
                rtc.index = -1;
                break;

            case 0xE:
                rtc.mode = MODE_COMMAND;
                break;

            default:
                // Ignore the write if it's an 0xF ???
	        // Probably should switch back to read mode -- but this
	        //  sequence never occurs in DKJM2
                break;
        }

        return;
    }

    if ( rtc.mode == MODE_LOAD_RTC )
    {
        if ( (rtc.index >= 0) || (rtc.index < MAX_RTC_INDEX) )
        {
            rtc.data[rtc.index++] = data;

            if ( rtc.index == MAX_RTC_INDEX )
            {
                // We have all the data for the RTC load

                rtc.system_timestamp = time (NULL);	// Get local system time

                // Get the day of the week
                rtc.data[rtc.index++] = S9xSRTCComputeDayOfWeek ();

                // Start RTC counting again
                rtc.count_enable = TRUE;
                rtc.needs_init = FALSE;
            }

            return;
        }
        else
        {
            // Attempting to write too much data
            // error(); // ignore??
        }
    }
    else if ( rtc.mode == MODE_COMMAND )
    {
        switch( data )
        {
            case COMMAND_CLEAR_RTC:
                // Disable RTC counter
                rtc.count_enable = FALSE;

                ZeroMemory (rtc.data, MAX_RTC_INDEX+1);
                rtc.index = -1;
                rtc.mode = MODE_COMMAND_DONE;
                break;

            case COMMAND_LOAD_RTC:
                // Disable RTC counter
                rtc.count_enable = FALSE;

                rtc.index = 0;  // Setup for writing
                rtc.mode = MODE_LOAD_RTC;
                break;

            default:
                rtc.mode = MODE_COMMAND_DONE;
                // unrecognized command - need to implement.
        }

        return;
    }
    else
    {
        if ( rtc.mode == MODE_READ )
        {
            // Attempting to write while in read mode. Ignore.
        }

        if ( rtc.mode == MODE_COMMAND_DONE )
        {
            // Maybe this isn't an error.  Maybe we should kick off
            // a new E command.  But is this valid?
        }
    }
}

/**********************************************************************************************/
/* S9xGetSRTC()                                                                               */
/* This function retrieves data from the S-RTC                                                */
/**********************************************************************************************/
uint8 S9xGetSRTC (uint16 Address)
{
    if ( rtc.mode == MODE_READ )
    {
        if ( rtc.index < 0 )
        {
            S9xUpdateSrtcTime ();	// Only update it if the game reads it
            rtc.index++;
            return ( 0x0f );        // Send start marker.
        }
        else if (rtc.index > MAX_RTC_INDEX)
        {
            rtc.index = -1;         // Setup for next set of reads
            return ( 0x0f );        // Data done marker.
        }
        else
        {
            // Feed out the data
            return rtc.data[rtc.index++];
        }
     }
     else
     {
         return 0x0;
     }
}

void S9xSRTCPreSaveState ()
{
    if (Settings.SRTC)
    {
	S9xUpdateSrtcTime ();

	int s = Memory.SRAMSize ?
		(1 << (Memory.SRAMSize + 3)) * 128 : 0;
	if (s > 0x20000)
	    s = 0x20000;

	SRAM [s + 0] = rtc.needs_init;
	SRAM [s + 1] = rtc.count_enable;
	memmove (&SRAM [s + 2], rtc.data, MAX_RTC_INDEX + 1);
	SRAM [s + 3 + MAX_RTC_INDEX] = rtc.index;
	SRAM [s + 4 + MAX_RTC_INDEX] = rtc.mode;

#ifdef LSB_FIRST
	memmove (&SRAM [s + 5 + MAX_RTC_INDEX], &rtc.system_timestamp, 8);
#else
	SRAM [s + 5  + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >>  0);
	SRAM [s + 6  + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >>  8);
	SRAM [s + 7  + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 16);
	SRAM [s + 8  + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 24);
	SRAM [s + 9  + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 32);
	SRAM [s + 10 + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 40);
	SRAM [s + 11 + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 48);
	SRAM [s + 12 + MAX_RTC_INDEX] = (uint8) (rtc.system_timestamp >> 56);
#endif
    }
}

void S9xSRTCPostLoadState ()
{
    if (Settings.SRTC)
    {
	int s = Memory.SRAMSize ?
		(1 << (Memory.SRAMSize + 3)) * 128 : 0;
	if (s > 0x20000)
	    s = 0x20000;

	rtc.needs_init = SRAM [s + 0];
	rtc.count_enable = SRAM [s + 1];
	memmove (rtc.data, &SRAM [s + 2], MAX_RTC_INDEX + 1);
	rtc.index = SRAM [s + 3 + MAX_RTC_INDEX];
	rtc.mode = SRAM [s + 4 + MAX_RTC_INDEX];

#ifdef LSB_FIRST
	memmove (&rtc.system_timestamp, &SRAM [s + 5 + MAX_RTC_INDEX], 8);
#else
	rtc.system_timestamp |= (SRAM [s +  5 + MAX_RTC_INDEX] <<  0);
	rtc.system_timestamp |= (SRAM [s +  6 + MAX_RTC_INDEX] <<  8);
	rtc.system_timestamp |= (SRAM [s +  7 + MAX_RTC_INDEX] << 16);
	rtc.system_timestamp |= (SRAM [s +  8 + MAX_RTC_INDEX] << 24);
	rtc.system_timestamp |= (SRAM [s +  9 + MAX_RTC_INDEX] << 32);
	rtc.system_timestamp |= (SRAM [s + 10 + MAX_RTC_INDEX] << 40);
	rtc.system_timestamp |= (SRAM [s + 11 + MAX_RTC_INDEX] << 48);
	rtc.system_timestamp |= (SRAM [s + 12 + MAX_RTC_INDEX] << 56);
#endif
	S9xUpdateSrtcTime ();
    }
}

