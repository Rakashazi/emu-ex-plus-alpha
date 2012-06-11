/*
 *	Header file for the PD4990A Serial I/O calendar & clock.
 */
#include "memory.h"
struct pd4990a_s
{
  int seconds;
  int minutes;
  int hours;
  int days;
  int month;
  int year;
  int weekday;

  Uint32 shiftlo,shifthi;
  int retraces;		/* Assumes 60 retraces a second */
  int testwaits;
  int maxwaits;
  int testbit;		/* Pulses a bit in order to simulate */
  /* test output */
  int outputbit;
  int bitno;

  char reading;
  char writing;

  // for pd4990a_serial_control
  int clock_line;
  int command_line;	/*?? */
};

void pd4990a_init(void);
//void pd4990a_init_save_state(void);
void pd4990a_addretrace(void);
int read_4990_testbit(void);
int read_4990_databit(void);
void write_4990_control_w(Uint32 address, Uint32 data);
void pd4990a_increment_day(void);
void pd4990a_increment_month(void);
