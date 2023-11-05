/*
 *	Emulation for the NEC PD4990A.
 *
 *	The PD4990A is a serial I/O Calendar & Clock IC used in the
 *      NEO GEO and probably a couple of other machines.
 */
#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

//#include "driver.h"
//#include "SDL.h"
#include <time.h>
#include <string.h>
#include "pd4990a.h"
#include "state.h"


/* Set the data in the chip to Monday 09/09/73 00:00:00     */
/* If you ever read this Leejanne, you know what I mean :-) */
struct pd4990a_s pd4990a =
{
  0x00,	/* seconds BCD */
  0x00,	/* minutes BCD */
  0x00,	/* hours   BCD */
  0x09,	/* days    BCD */
  9,		/* month   Hexadecimal form */
  0x73,	/* year    BCD */
  1		/* weekday BCD */
};

#define DATA_BIT	0x1
#define CLOCK_BIT	0x2
#define END_BIT		0x4

void pd4990a_addretrace()
{
  ++pd4990a.testwaits;
  if(pd4990a.testwaits>=pd4990a.maxwaits)
  {
	pd4990a.testbit ^= 1;
	pd4990a.testwaits=0;
  }
  pd4990a.retraces++;
  if (pd4990a.retraces < 60)
	return;
  pd4990a.retraces = 0;
  pd4990a.seconds++;
  if ( (pd4990a.seconds & 0x0f) < 10 )
	return;
  pd4990a.seconds &= 0xf0;
  pd4990a.seconds += 0x10;
  if (pd4990a.seconds < 0x60)
	return;
  pd4990a.seconds = 0;
  pd4990a.minutes++;
  if ( (pd4990a.minutes & 0x0f) < 10 )
	return;
  pd4990a.minutes &= 0xf0;
  pd4990a.minutes += 0x10;
  if (pd4990a.minutes < 0x60)
	return;
  pd4990a.minutes = 0;
  pd4990a.hours++;
  if ( (pd4990a.hours & 0x0f) < 10 )
	return;
  pd4990a.hours &= 0xf0;
  pd4990a.hours += 0x10;
  if (pd4990a.hours < 0x24)
	return;
  pd4990a.hours = 0;
  pd4990a_increment_day();
}

#if 0
void pd4990a_init_save_state(void) {
    create_state_register(ST_PD4990A,"seconds",1,&pd4990a.seconds,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"minutes",1,&pd4990a.minutes,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"hours",1,&pd4990a.hours,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"days",1,&pd4990a.days,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"month",1,&pd4990a.month,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"year",1,&pd4990a.year,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"weekday",1,&pd4990a.weekday,sizeof(Sint32),REG_INT32);

    create_state_register(ST_PD4990A,"pd4990a.retraces",1,&pd4990a.retraces,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.testbit",1,&pd4990a.testbit,sizeof(Sint32),REG_INT32);
    
    create_state_register(ST_PD4990A,"pd4990a.testwaits",1,&pd4990a.testwaits,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.maxwaits",1,&pd4990a.maxwaits,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.shiftlo",1,&pd4990a.shiftlo,sizeof(Uint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.shifthi",1,&pd4990a.shifthi,sizeof(Uint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.reading",1,&pd4990a.reading,sizeof(Sint8),REG_INT8);
    create_state_register(ST_PD4990A,"pd4990a.writing",1,&pd4990a.writing,sizeof(Sint8),REG_INT8);

    create_state_register(ST_PD4990A,"pd4990a.outputbit",1,&pd4990a.outputbit,sizeof(Sint32),REG_INT32);
    create_state_register(ST_PD4990A,"pd4990a.bitno",1,&pd4990a.bitno,sizeof(Sint32),REG_INT32);

}
#endif

void pd4990a_mkstate(Stream *gzf,int mode)
{
	mkstate_data(gzf, &pd4990a, sizeof(pd4990a), mode);
}

void pd4990a_init(void) {
    time_t ltime;
    struct tm *today;

    memset(&pd4990a, 0, sizeof(pd4990a));

//    printf("pd4990a_init\n");

    pd4990a.seconds = 0x00;		/* BCD */
    pd4990a.minutes = 0x00;		/* BCD */
    pd4990a.hours = 0x00;		/* BCD */
    pd4990a.days = 0x09;		/* BCD */
    pd4990a.month = 9;			/* Hexadecimal form */
    pd4990a.year = 0x73;		/* BCD */
    pd4990a.weekday = 1;		/* BCD */

    pd4990a.maxwaits = 1;

    time(&ltime);
    today = localtime(&ltime);

    pd4990a.seconds = ((today->tm_sec / 10) << 4) + (today->tm_sec % 10);
    pd4990a.minutes = ((today->tm_min / 10) << 4) + (today->tm_min % 10);
    pd4990a.hours = ((today->tm_hour / 10) << 4) + (today->tm_hour % 10);
    pd4990a.days = ((today->tm_mday / 10) << 4) + (today->tm_mday % 10);
    pd4990a.month = (today->tm_mon + 1);
    pd4990a.year = ((today->tm_year / 10) << 4) + (today->tm_year % 10);
    pd4990a.weekday = today->tm_wday;
    //printf("%d %d %d %d %d %d %d\n",pd4990a.seconds,pd4990a.minutes,pd4990a.hours,pd4990a.days,pd4990a.month,pd4990a.year,pd4990a.weekday);
    //pd4990a_init_save_state();
}

void pd4990a_increment_day(void)
{
  int real_year;

  pd4990a.days++;
  if ((pd4990a.days & 0x0f) >= 10)
  {
    pd4990a.days &= 0xf0;
    pd4990a.days += 0x10;
  }

  pd4990a.weekday++;
  if (pd4990a.weekday == 7)
    pd4990a.weekday=0;

  switch(pd4990a.month)
  {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      if (pd4990a.days == 0x32)
      {
	pd4990a.days = 1;
	pd4990a_increment_month();
      }
      break;
    case 2:
      real_year = (pd4990a.year>>4)*10 + (pd4990a.year&0xf);
      if ((real_year % 4) && (!(real_year % 100) || (real_year % 400)))
      {
	if (pd4990a.days == 0x29)
	{
	  pd4990a.days = 1;
	  pd4990a_increment_month();
	}
      }
      else
      {
	if (pd4990a.days == 0x30)
	{
	  pd4990a.days = 1;
	  pd4990a_increment_month();
	}
      }
      break;
    case 4: case 6: case 9: case 11:
      if (pd4990a.days == 0x31)
      {
	pd4990a.days = 1;
	pd4990a_increment_month();
      }
      break;
  }
}

void pd4990a_increment_month(void)
{
  pd4990a.month++;
  if (pd4990a.month == 13)
  {
    pd4990a.month = 1;
    pd4990a.year++;
    if ( (pd4990a.year & 0x0f) >= 10 )
    {
      pd4990a.year &= 0xf0;
      pd4990a.year += 0x10;
    }
    if (pd4990a.year == 0xA0)
      pd4990a.year = 0;
  }
}

int read_4990_testbit(void)
{
    return (pd4990a.testbit);
}

int read_4990_databit(void)
{
    return (pd4990a.outputbit);
}

static void pd4990a_readbit(void)
{
  switch(pd4990a.bitno)
  {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x04: case 0x05: case 0x06: case 0x07:
      pd4990a.outputbit=(pd4990a.seconds >> pd4990a.bitno) & 0x01;
      break;
    case 0x08: case 0x09: case 0x0a: case 0x0b:
    case 0x0c: case 0x0d: case 0x0e: case 0x0f:
      pd4990a.outputbit=(pd4990a.minutes >> (pd4990a.bitno-0x08)) & 0x01;
      break;
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x14: case 0x15: case 0x16: case 0x17:
      pd4990a.outputbit=(pd4990a.hours >> (pd4990a.bitno-0x10)) & 0x01;
      break;
    case 0x18: case 0x19: case 0x1a: case 0x1b:
    case 0x1c: case 0x1d: case 0x1e: case 0x1f:
      pd4990a.outputbit=(pd4990a.days >> (pd4990a.bitno-0x18)) & 0x01;
      break;
    case 0x20: case 0x21: case 0x22: case 0x23:
      pd4990a.outputbit=(pd4990a.weekday >> (pd4990a.bitno-0x20)) & 0x01;
      break;
    case 0x24: case 0x25: case 0x26: case 0x27:
      pd4990a.outputbit=(pd4990a.month >> (pd4990a.bitno-0x24)) & 0x01;
      break;
    case 0x28: case 0x29: case 0x2a: case 0x2b:
    case 0x2c: case 0x2d: case 0x2e: case 0x2f:
      pd4990a.outputbit=(pd4990a.year >> (pd4990a.bitno-0x28)) & 0x01;
      break;
    case 0x30: case 0x31: case 0x32: case 0x33:
      /*unknown */
      break;
  }
}




static void pd4990a_resetbitstream(void)
{
  pd4990a.shiftlo=0;
  pd4990a.shifthi=0;
  pd4990a.bitno=0;
}

static void pd4990a_writebit(Uint8 bit)
{
  if(pd4990a.bitno<=31)	/*low part */
    pd4990a.shiftlo|=bit<<pd4990a.bitno;
  else	/*high part */
    pd4990a.shifthi|=bit<<(pd4990a.bitno-32);
}

static void pd4990a_nextbit(void)
{
  ++pd4990a.bitno;
  if(pd4990a.reading)
    pd4990a_readbit();
  if(pd4990a.reading && pd4990a.bitno==0x34)
  {
    pd4990a.reading=0;
    pd4990a_resetbitstream();
  }

}

static Uint8 pd4990a_getcommand(void)
{
  /*Warning: problems if the 4 bits are in different */
  /*parts, It's very strange that this case could happen. */
  if(pd4990a.bitno<=31)
    return pd4990a.shiftlo>>(pd4990a.bitno-4);
  else
    return pd4990a.shifthi>>(pd4990a.bitno-32-4);
}

static void pd4990a_update_date(void)
{
  pd4990a.seconds=(pd4990a.shiftlo>>0 )&0xff;
  pd4990a.minutes=(pd4990a.shiftlo>>8 )&0xff;
  pd4990a.hours  =(pd4990a.shiftlo>>16)&0xff;
  pd4990a.days   =(pd4990a.shiftlo>>24)&0xff;
  pd4990a.weekday=(pd4990a.shifthi>>0 )&0x0f;
  pd4990a.month  =(pd4990a.shifthi>>4 )&0x0f;
  pd4990a.year   =(pd4990a.shifthi>>8 )&0xff;
}

static void pd4990a_process_command(void)
{
  switch(pd4990a_getcommand())
  {
    case 0x1:	/*load output register */
      pd4990a.bitno=0;
      if(pd4990a.reading)
	pd4990a_readbit();	/*prepare first bit */
      pd4990a.shiftlo=0;
      pd4990a.shifthi=0;
      break;
    case 0x2:
      pd4990a.writing=0;	/*store register to current date */
      pd4990a_update_date();
      break;
      case 0x3:	/*start reading */
	pd4990a.reading=1;
	break;
	case 0x7:	/*switch testbit every frame */
	  pd4990a.maxwaits=1;
	  break;
	  case 0x8:	/*switch testbit every half-second */
	    pd4990a.maxwaits=30;
	    break;
  }
}


void pd4990a_serial_control(unsigned char data)
{
  /*Check for command end */
  if(pd4990a.command_line && !(data&END_BIT)) /*end of command */
  {
    pd4990a_process_command();
    pd4990a_resetbitstream();
  }
  pd4990a.command_line=data&END_BIT;

  if(pd4990a.clock_line && !(data&CLOCK_BIT))	/*clock lower edge */
  {
    pd4990a_writebit(data&DATA_BIT);
    pd4990a_nextbit();
  }
  pd4990a.clock_line=data&CLOCK_BIT;
}

void write_4990_control_w(Uint32 address, Uint32 data) {
  pd4990a_serial_control(data&0x7);
}
