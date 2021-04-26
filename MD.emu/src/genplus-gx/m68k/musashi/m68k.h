#ifndef M68K__HEADER
#define M68K__HEADER

#define CONFIG_M68KEMU_MUSASHI

/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 3.32
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright Karl Stenerud.  All rights reserved.
 *
 * This code may be freely used for non-commercial purposes as long as this
 * copyright notice remains unaltered in the source code and any binary files
 * containing this code in compiled form.
 *
 * All other licensing terms must be negotiated with the author
 * (Karl Stenerud).
 *
 * The latest version of this code can be obtained at:
 * http://kstenerud.cjb.net
 */

/* ======================================================================== */
/* ============================= CONFIGURATION ============================ */
/* ======================================================================== */

/* Import the configuration for this build */
#include <assert.h>
#include <imagine/logger/logger.h>
#include <stdlib.h>
#include "m68kconf.h"

/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */

/* ======================================================================== */

/* There are 7 levels of interrupt to the 68K.
 * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
 */
#define M68K_IRQ_NONE 0
#define M68K_IRQ_1    1
#define M68K_IRQ_2    2
#define M68K_IRQ_3    3
#define M68K_IRQ_4    4
#define M68K_IRQ_5    5
#define M68K_IRQ_6    6
#define M68K_IRQ_7    7


/* Special interrupt acknowledge values.
 * Use these as special returns from the interrupt acknowledge callback
 * (specified later in this header).
 */

/* Causes an interrupt autovector (0x18 + interrupt level) to be taken.
 * This happens in a real 68K if VPA or AVEC is asserted during an interrupt
 * acknowledge cycle instead of DTACK.
 */
#define M68K_INT_ACK_AUTOVECTOR    0xffffffff

/* Causes the spurious interrupt vector (0x18) to be taken
 * This happens in a real 68K if BERR is asserted during the interrupt
 * acknowledge cycle (i.e. no devices responded to the acknowledge).
 */
#define M68K_INT_ACK_SPURIOUS      0xfffffffe


/* CPU types for use in m68k_set_cpu_type() */
enum
{
  M68K_CPU_TYPE_INVALID,
  M68K_CPU_TYPE_68000,
  M68K_CPU_TYPE_68008,
  M68K_CPU_TYPE_68010,
  M68K_CPU_TYPE_68EC020,
  M68K_CPU_TYPE_68020,
  M68K_CPU_TYPE_68030,  /* Supported by disassembler ONLY */
  M68K_CPU_TYPE_68040    /* Supported by disassembler ONLY */
};

/* Registers used by m68k_get_reg() and m68k_set_reg() */
typedef enum
{
  /* Real registers */
  M68K_REG_D0,    /* Data registers */
  M68K_REG_D1,
  M68K_REG_D2,
  M68K_REG_D3,
  M68K_REG_D4,
  M68K_REG_D5,
  M68K_REG_D6,
  M68K_REG_D7,
  M68K_REG_A0,    /* Address registers */
  M68K_REG_A1,
  M68K_REG_A2,
  M68K_REG_A3,
  M68K_REG_A4,
  M68K_REG_A5,
  M68K_REG_A6,
  M68K_REG_A7,
  M68K_REG_PC,    /* Program Counter */
  M68K_REG_SR,    /* Status Register */
  M68K_REG_SP,    /* The current Stack Pointer (located in A7) */
  M68K_REG_USP,    /* User Stack Pointer */
  M68K_REG_ISP,    /* Interrupt Stack Pointer */
  M68K_REG_MSP,    /* Master Stack Pointer */
  M68K_REG_SFC,    /* Source Function Code */
  M68K_REG_DFC,    /* Destination Function Code */
  M68K_REG_VBR,    /* Vector Base Register */
  M68K_REG_CACR,    /* Cache Control Register */
  M68K_REG_CAAR,    /* Cache Address Register */

  /* Assumed registers */
  /* These are cheat registers which emulate the 1-longword prefetch
     * present in the 68000 and 68010.
     */
  M68K_REG_PREF_ADDR,  /* Last prefetch address */
  M68K_REG_PREF_DATA,  /* Last prefetch data */

  /* Convenience registers */
  M68K_REG_PPC,    /* Previous value in the program counter */
  M68K_REG_IR,    /* Instruction register */
  M68K_REG_CPU_TYPE  /* Type of CPU being run */
} m68k_register_t;

/* ======================================================================== */
/* ====================== FUNCTIONS CALLED BY THE CPU ===================== */
/* ======================================================================== */

/* You will have to implement these functions */

/* read/write functions called by the CPU to access memory.
 * while values used are 32 bits, only the appropriate number
 * of bits are relevant (i.e. in write_memory_8, only the lower 8 bits
 * of value should be written to memory).
 *
 * NOTE: I have separated the immediate and PC-relative memory fetches
 *       from the other memory fetches because some systems require
 *       differentiation between PROGRAM and DATA fetches (usually
 *       for security setups such as encryption).
 *       This separation can either be achieved by setting
 *       M68K_SEPARATE_READS in m68kconf.h and defining
 *       the read functions, or by setting M68K_EMULATE_FC and
 *       making a function code callback function.
 *       Using the callback offers better emulation coverage
 *       because you can also monitor whether the CPU is in SYSTEM or
 *       USER mode, but it is also slower.
 */

/* Read from anywhere */
unsigned int  m68k_read_memory_8(unsigned int address);
unsigned int  m68k_read_memory_16(unsigned int address);
unsigned int  m68k_read_memory_32(unsigned int address);

/* Memory access for the disassembler */
unsigned int m68k_read_disassembler_8  (unsigned int address);
unsigned int m68k_read_disassembler_16 (unsigned int address);
unsigned int m68k_read_disassembler_32 (unsigned int address);

/* Write to anywhere */
void m68k_write_memory_8(unsigned int address, unsigned int value);
void m68k_write_memory_16(unsigned int address, unsigned int value);
void m68k_write_memory_32(unsigned int address, unsigned int value);


#include "macros.h"

struct _m68k_memory_map
{
	constexpr _m68k_memory_map() {}
  unsigned char *base{};
  unsigned int (*read8)(unsigned int address){};
  unsigned int (*read16)(unsigned int address){};
  void (*write8)(unsigned int address, unsigned int data){};
  void (*write16)(unsigned int address, unsigned int data){};
};

/* Special call to simulate undocumented 68k behavior when move.l with a
 * predecrement destination mode is executed.
 * To simulate real 68k behavior, first write the high word to
 * [address+2], and then write the low word to [address].
 *
 * Enable this functionality with M68K_SIMULATE_PD_WRITES in m68kconf.h.
 */
void m68k_write_memory_32_pd(unsigned int address, unsigned int value);

typedef union
{
  uint64_t i;
  double f;
} fp_reg;

struct M68KCPU
{
	constexpr M68KCPU(const unsigned char (&cycles)[0x10000], bool hasWorkingTas):
		cycles(cycles), hasWorkingTas(hasWorkingTas) {}

  static const unsigned cpu_type = 1;     /* CPU Type: 68000, 68008, 68010, 68EC020, or 68020 */
  const unsigned char (&cycles)[0x10000];
  unsigned dar[16]{};      /* Data and Address Registers */
#ifdef M68K_USE_PPC
  unsigned ppc = 0;       /* Previous program counter */
#endif
  unsigned pc = 0;           /* Program Counter */
  unsigned sp[7]{};        /* User, Interrupt, and Master Stack Pointers */
  unsigned ir = 0;           /* Instruction Register */
#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  unsigned vbr = 0;          /* Vector Base Register (m68010+) */
  unsigned sfc = 0;          /* Source Function Code Register (m68010+) */
  unsigned dfc = 0;          /* Destination Function Code Register (m68010+) */
  unsigned cacr = 0;         /* Cache Control Register (m68020, unemulated) */
  unsigned caar = 0;         /* Cache Address Register (m68020, unemulated) */
  fp_reg fpr[8]{};     /* FPU Data Register (m68040) */
  unsigned fpiar = 0;        /* FPU Instruction Address Register (m68040) */
  unsigned fpsr = 0;         /* FPU Status Register (m68040) */
  unsigned fpcr = 0;         /* FPU Control Register (m68040) */
#endif
  unsigned t1_flag = 0;      /* Trace 1 */
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  unsigned t0_flag = 0;      /* Trace 0 */
#endif
  unsigned s_flag = 0;       /* Supervisor */
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  unsigned m_flag = 0;       /* Master/Interrupt state */
#endif
  unsigned x_flag = 0;       /* Extend */
  unsigned n_flag = 0;       /* Negative */
  unsigned not_z_flag = 0;   /* Zero, inverted for speedups */
  unsigned v_flag = 0;       /* Overflow */
  unsigned c_flag = 0;       /* Carry */
  unsigned int_mask = 0;     /* I0-I2 */
  unsigned int_level = 0;    /* State of interrupt pins IPL0-IPL2 -- ASG: changed from ints_pending */
  unsigned stopped = 0;      /* Stopped state */
#if M68K_EMULATE_PREFETCH
  unsigned pref_addr = 0;    /* Last prefetch address */
  unsigned pref_data = 0;    /* Data in the prefetch queue */
#endif
  static const unsigned address_mask = 0x00ffffff; /* Available address pins */
  static const unsigned sr_mask = 0xa71f;      /* Implemented status register bits */
#if M68K_EMULATE_ADDRESS_ERROR
  unsigned instr_mode = 0;   /* Stores whether we are in instruction mode or group 0/1 exception mode */
  unsigned run_mode = 0;     /* Stores whether we are processing a reset, bus error, address error, or something else */
#endif
  const bool hasWorkingTas;

  /* Clocks required for instructions / exceptions */
  static const unsigned cyc_bcc_notake_b = -2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_bcc_notake_w = 2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_dbcc_f_noexp = -2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_dbcc_f_exp = 2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_scc_r_true = 2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_movem_w = 4 * M68K_CYCLE_SCALER;
  static const unsigned cyc_movem_l = 8 * M68K_CYCLE_SCALER;
  static const unsigned cyc_shift = 2 * M68K_CYCLE_SCALER;
  static const unsigned cyc_reset = 132 * M68K_CYCLE_SCALER;
  /*static const uint8* cyc_instruction = m68ki_cycles;
  static const uint16* cyc_exception = m68ki_exception_cycle_table;*/

  /* Callbacks to host */
#if M68K_EMULATE_INT_ACK == OPT_ON
  int  (*int_ack_callback)(M68KCPU &m68ki_cpu, int int_line) = nullptr;           /* Interrupt Acknowledge */
#endif
#if 0
  void (*bkpt_ack_callback)(M68KCPU &m68ki_cpu, unsigned int data);     /* Breakpoint Acknowledge */
  void (*reset_instr_callback)(M68KCPU &m68ki_cpu);               /* Called when a RESET instruction is encountered */
  void (*cmpild_instr_callback)(M68KCPU &m68ki_cpu, unsigned int, int); /* Called when a CMPI.L #v, Dn instruction is encountered */
  void (*rte_instr_callback)(M68KCPU &m68ki_cpu);                 /* Called when a RTE instruction is encountered */
  int  (*tas_instr_callback)(M68KCPU &m68ki_cpu);                 /* Called when a TAS instruction is encountered, allows / disallows writeback */
#endif
#if M68K_MONITOR_PC == OPT_ON
  void (*pc_changed_callback)(M68KCPU &m68ki_cpu, unsigned oldPC, unsigned PC) = nullptr; /* Called when the PC changes by a large amount */
#endif
#if 0
  void (*set_fc_callback)(M68KCPU &m68ki_cpu, unsigned int new_fc);     /* Called when the CPU function code changes */
  void (*instr_hook_callback)(M68KCPU &m68ki_cpu, unsigned int pc);     /* Called every instruction cycle prior to execution */
#endif

  int irqLatency = 0;
  int32_t cycleCount = 0;
  int32_t endCycles = 0;
  _m68k_memory_map memory_map[256]{};

  /* Set the IPL0-IPL2 pins on the CPU (IRQ).
   * A transition from < 7 to 7 will cause a non-maskable interrupt (NMI).
   * Setting IRQ to 0 will clear an interrupt request.
   */

  void updateIRQ(unsigned mask);
  void setIRQ(unsigned mask);
  void setIRQDelay(unsigned mask);

  static const bool callMemHooks =
	#ifndef NDEBUG
  	1
	#else
  	0
	#endif
  ;

#ifndef NDEBUG
  unsigned id_ = 0;
  void setID(unsigned id) { id_ = id; }
  unsigned id() const { return id_; }
#else
  void setID(unsigned id) { }
  unsigned id() const { return 0; }
#endif
};

void m68k_read_immediate_16_hook(M68KCPU &cpu, unsigned address);
void m68k_read_immediate_32_hook(M68KCPU &cpu, unsigned address);
void m68k_read_pcrelative_8_hook(M68KCPU &cpu, unsigned address);

/* Read data immediately following the PC */
static inline uint16_t m68k_read_immediate_16(M68KCPU &cpu, unsigned address)
{
	if(cpu.callMemHooks)
		m68k_read_immediate_16_hook(cpu, address);
	uint32_t mapIdx = ((address)>>16)&0xff;
	const _m68k_memory_map *temp = &cpu.memory_map[mapIdx];
	if(!M68K_DIRECT_IM_READS && temp->read16)
		return (*temp->read16)(address & cpu.address_mask);
	else
		return *(uint16_t *)(cpu.memory_map[mapIdx].base + ((address) & 0xffff));
}

static inline uint32_t m68k_read_immediate_32(M68KCPU &cpu, unsigned address)
{
	if(cpu.callMemHooks)
		m68k_read_immediate_32_hook(cpu, address);
	return (m68k_read_immediate_16(cpu, address) << 16) | (m68k_read_immediate_16(cpu, address+2));
}

/* Read data relative to the PC */
static inline uint8_t m68k_read_pcrelative_8(M68KCPU &cpu, unsigned address)
{
	if(cpu.callMemHooks)
		m68k_read_pcrelative_8_hook(cpu, address);
	uint32_t mapIdx = ((address)>>16)&0xff;
	const _m68k_memory_map *temp = &cpu.memory_map[mapIdx];
	if(!M68K_DIRECT_IM_READS && temp->read8)
		return (*temp->read8)(address & cpu.address_mask);
	else
		return READ_BYTE(cpu.memory_map[mapIdx].base, (address) & 0xffff);
}

#define m68k_read_pcrelative_16(m68ki_cpu, address) m68k_read_immediate_16(m68ki_cpu, address)
#define m68k_read_pcrelative_32(m68ki_cpu, address) m68k_read_immediate_32(m68ki_cpu, address)


/* ======================================================================== */
/* ============================== CALLBACKS =============================== */
/* ======================================================================== */

/* These functions allow you to set callbacks to the host when specific events
 * occur.  Note that you must enable the corresponding value in m68kconf.h
 * in order for these to do anything useful.
 * Note: I have defined default callbacks which are used if you have enabled
 * the corresponding #define in m68kconf.h but either haven't assigned a
 * callback or have assigned a callback of NULL.
 */

/* Set the callback for an interrupt acknowledge.
 * You must enable M68K_EMULATE_INT_ACK in m68kconf.h.
 * The CPU will call the callback with the interrupt level being acknowledged.
 * The host program must return either a vector from 0x02-0xff, or one of the
 * special interrupt acknowledge values specified earlier in this header.
 * If this is not implemented, the CPU will always assume an autovectored
 * interrupt, and will automatically clear the interrupt request when it
 * services the interrupt.
 * Default behavior: return M68K_INT_ACK_AUTOVECTOR.
 */
void m68k_set_int_ack_callback(M68KCPU &m68ki_cpu, int  (*callback)(M68KCPU &m68ki_cpu, int int_level));

/* Set the callback for a breakpoint acknowledge (68010+).
 * You must enable M68K_EMULATE_BKPT_ACK in m68kconf.h.
 * The CPU will call the callback with whatever was in the data field of the
 * BKPT instruction for 68020+, or 0 for 68010.
 * Default behavior: do nothing.
 */
void m68k_set_bkpt_ack_callback(void (*callback)(M68KCPU &m68ki_cpu, unsigned int data));


/* Set the callback for the RESET instruction.
 * You must enable M68K_EMULATE_RESET in m68kconf.h.
 * The CPU calls this callback every time it encounters a RESET instruction.
 * Default behavior: do nothing.
 */
void m68k_set_reset_instr_callback(void  (*callback)(M68KCPU &m68ki_cpu));


/* Set the callback for the CMPI.L #v, Dn instruction.
 * You must enable M68K_CMPILD_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a CMPI.L #v, Dn instruction.
 * Default behavior: do nothing.
 */
void m68k_set_cmpild_instr_callback(void  (*callback)(M68KCPU &m68ki_cpu, unsigned int val, int reg));


/* Set the callback for the RTE instruction.
 * You must enable M68K_RTE_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a RTE instruction.
 * Default behavior: do nothing.
 */
void m68k_set_rte_instr_callback(void  (*callback)(M68KCPU &m68ki_cpu));

/* Set the callback for the TAS instruction.
 * You must enable M68K_TAS_HAS_CALLBACK in m68kconf.h.
 * The CPU calls this callback every time it encounters a TAS instruction.
 * Default behavior: return 1, allow writeback.
 */
void m68k_set_tas_instr_callback(int  (*callback)(M68KCPU &m68ki_cpu));



/* Set the callback for informing of a large PC change.
 * You must enable M68K_MONITOR_PC in m68kconf.h.
 * The CPU calls this callback with the new PC value every time the PC changes
 * by a large value (currently set for changes by longwords).
 * Default behavior: do nothing.
 */
void m68k_set_pc_changed_callback(void  (*callback)(M68KCPU &m68ki_cpu,  unsigned oldPC, unsigned PC));


/* Set the callback for CPU function code changes.
 * You must enable M68K_EMULATE_FC in m68kconf.h.
 * The CPU calls this callback with the function code before every memory
 * access to set the CPU's function code according to what kind of memory
 * access it is (supervisor/user, program/data and such).
 * Default behavior: do nothing.
 */
void m68k_set_fc_callback(void  (*callback)(M68KCPU &m68ki_cpu, unsigned int new_fc));


/* Set a callback for the instruction cycle of the CPU.
 * You must enable M68K_INSTRUCTION_HOOK in m68kconf.h.
 * The CPU calls this callback just before fetching the opcode in the
 * instruction cycle.
 * Default behavior: do nothing.
 */
void m68k_set_instr_hook_callback(void  (*callback)(M68KCPU &m68ki_cpu, unsigned int pc));



/* ======================================================================== */
/* ====================== FUNCTIONS TO ACCESS THE CPU ===================== */
/* ======================================================================== */

/* Use this function to set the CPU type you want to emulate.
 * Currently supported types are: M68K_CPU_TYPE_68000, M68K_CPU_TYPE_68008,
 * M68K_CPU_TYPE_68010, M68K_CPU_TYPE_EC020, and M68K_CPU_TYPE_68020.
 */
// TODO: set other CPU types besides 68000 via templates
//static void m68k_set_cpu_type(M68KCPU &m68ki_cpu, unsigned int cpu_type) { }

/* Do whatever initialisations the core requires.  Should be called
 * at least once at init time.
 */
void m68k_init(M68KCPU &m68ki_cpu);

/* Pulse the RESET pin on the CPU.
 * You *MUST* reset the CPU at least once to initialize the emulation
 * Note: If you didn't call m68k_set_cpu_type() before resetting
 *       the CPU for the first time, the CPU will be set to
 *       M68K_CPU_TYPE_68000.
 */
void m68k_pulse_reset(M68KCPU &m68ki_cpu);

/* execute num_cycles worth of instructions.  returns number of cycles used */
//int m68k_execute(int num_cycles);

/* run until global cycle count is reached */
void m68k_run(M68KCPU &m68ki_cpu, int cycles) __attribute__((hot));

/* These functions let you read/write/modify the number of cycles left to run
 * while m68k_execute() is running.
 * These are useful if the 68k accesses a memory-mapped port on another device
 * that requires immediate processing by another CPU.
 */
//int m68k_cycles_run(void);              /* Number of cycles run so far */
//int m68k_cycles_remaining(void);        /* Number of cycles left */
//void m68k_modify_timeslice(int cycles); /* Modify cycles left */
//void m68k_end_timeslice(void);          /* End timeslice now */

/* Halt the CPU as if you pulsed the HALT pin. */
void m68k_pulse_halt(M68KCPU &m68ki_cpu);

/* Register the CPU state information */
void m68k_state_register(M68KCPU &m68ki_cpu, const char *type, int index);


/* Peek at the internals of a CPU context.  This can either be a context
 * retrieved using m68k_get_context() or the currently running context.
 * If context is NULL, the currently running CPU context will be used.
 */
unsigned int m68k_get_reg(M68KCPU &m68ki_cpu, m68k_register_t reg);

/* Poke values into the internals of the currently running CPU context */
void m68k_set_reg(M68KCPU &m68ki_cpu, m68k_register_t reg, unsigned int value);

/* Check if an instruction is valid for the specified CPU type */
unsigned int m68k_is_valid_instruction(unsigned int instruction, unsigned int cpu_type);

/* Disassemble 1 instruction using the epecified CPU type at pc.  Stores
 * disassembly in str_buff and returns the size of the instruction in bytes.
 */
unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);

/* Same as above but accepts raw opcode data directly rather than fetching
 * via the read/write interfaces.
 */
unsigned int m68k_disassemble_raw(char* str_buff, unsigned int pc, const unsigned char* opdata, const unsigned char* argdata, unsigned int cpu_type);

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68K__HEADER */
