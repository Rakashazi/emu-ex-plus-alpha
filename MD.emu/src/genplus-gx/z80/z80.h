#pragma once

#include "osd_cpu.h"
#include <imagine/logger/logger.h>

enum
{
  /* line states */
  CLEAR_LINE = 0, /* clear (a fired, held or pulsed) line */
  ASSERT_LINE,    /* assert an interrupt immediately */
  HOLD_LINE,      /* hold interrupt line until acknowledged */
  PULSE_LINE     /* pulse interrupt line for one instruction */
};

enum {
  Z80_PC, Z80_SP,
  Z80_A, Z80_B, Z80_C, Z80_D, Z80_E, Z80_H, Z80_L,
  Z80_AF, Z80_BC, Z80_DE, Z80_HL,
  Z80_IX, Z80_IY,  Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
  Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
  Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3, Z80_WZ
};

enum {
  Z80_TABLE_op,
  Z80_TABLE_cb,
  Z80_TABLE_ed,
  Z80_TABLE_xy,
  Z80_TABLE_xycb,
  Z80_TABLE_ex  /* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Z80.r&127)|(Z80.r2&128)      */
/****************************************************************************/
typedef struct
{
  PAIR  pc,sp,af,bc,de,hl,ix,iy,wz;
  PAIR  af2,bc2,de2,hl2;
  UINT8  r,r2,iff1,iff2,halt,im,i;
  UINT8  nmi_state;      /* nmi line state */
  UINT8  nmi_pending;    /* nmi pending */
  UINT8  irq_state;      /* irq line state */
  UINT8  after_ei;      /* are we in the EI shadow? */
}  Z80_Regs;

class Z80CPU : public Z80_Regs
{
public:
	unsigned cycleCount;
	unsigned char *readmap[64];
	unsigned char *writemap[64];

	void (*writemem)(unsigned int address, unsigned char data);
	unsigned char (*readmem)(unsigned int address);
	void (*writeport)(unsigned int port, unsigned char data);
	unsigned char (*readport)(unsigned int port);

	void init();
	void reset();
	void exit();
	void run(unsigned cycles) __attribute__((hot));
	void burn(unsigned cycles);
	void setNmiLine(unsigned state);

	void setIRQ(unsigned state)
	{
		logMsg("set Z80 IRQ state 0x%X", state);
		irq_state = state;
	}
private:
	void staticInit();
};

extern Z80CPU Z80;

#define z80_readmap Z80.readmap
#define z80_writemap Z80.writemap
#define z80_readmem Z80.readmem
#define z80_writemem Z80.writemem
#define z80_readport Z80.readport
#define z80_writeport Z80.writeport

static void z80_init() { Z80.init(); }
static void z80_reset(void) { Z80.reset(); }
static void z80_exit (void) { Z80.exit(); }
static void z80_run(unsigned int cycles) { Z80.run(cycles); }
static void z80_burn(unsigned int cycles) { Z80.burn(cycles); }
static void z80_set_nmi_line(unsigned int state) { Z80.setNmiLine(state); }
