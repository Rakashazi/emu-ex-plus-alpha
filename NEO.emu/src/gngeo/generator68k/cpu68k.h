#ifndef _CPU68K_H_
#define _CPU68K_H_

typedef struct _t_ipc {
  void (*function)(struct _t_ipc *ipc);
  uint8 used;               /* bitmap of XNZVC flags inspected */
  uint8 set;                /* bitmap of XNZVC flags altered */
  uint16 opcode;
  uint16 wordlen;
  unsigned int :0;
  uint32 src;
  uint32 dst;
} t_ipc;

typedef struct _t_ipclist {
    struct _t_ipclist *next;
    uint8 norepeat;
    uint32 pc;
    uint32 pass;
    uint32  bank;
    uint32 clocks;
    void (*compiled)(struct _t_ipc *ipc);
} t_ipclist;

extern uint8 *cpu68k_rom;
extern unsigned int cpu68k_romlen;
extern uint8 *cpu68k_ram;
extern t_iib *cpu68k_iibtable[65536];
extern void (*cpu68k_functable[65536*2])(t_ipc *ipc);
extern int cpu68k_totalinstr;
extern int cpu68k_totalfuncs;
extern unsigned int cpu68k_clocks;
extern unsigned int cpu68k_clocks_curevent;
extern unsigned int cpu68k_frames;
extern unsigned int cpu68k_line;
extern t_regs regs;
extern t_ipclist *ipclist[LEN_IPCLISTTABLE];
extern uint8 movem_bit[256];
extern unsigned int cpu68k_adaptive;
extern unsigned int cpu68k_frozen;

extern t_iib iibs[];
extern int iibs_num;


int cpu68k_init(void);
void cpu68k_printipc(t_ipc *ipc);
void cpu68k_ipc(uint32 addr68k, uint8 *addr, t_iib *iib, t_ipc *ipc);
void cpu68k_reset(void);
void cpu68k_step(void);
void cpu68k_framestep(void);
t_ipclist *cpu68k_makeipclist(uint32 pc);
void cpu68k_endfield(void);
void cpu68k_clearcache(void);

#define V_RESETSSP   0
#define V_RESETPC    1
#define V_BUSERR     2
#define V_ADDRESS    3
#define V_ILLEGAL    4
#define V_ZERO       5
#define V_CHK        6
#define V_TRAPV      7
#define V_PRIVILEGE  8
#define V_TRACE      9
#define V_LINE10    10
#define V_LINE15    11
#define V_UNINIT    15
#define V_SPURIOUS  24
#define V_AUTO      25
#define V_TRAP      32
#define V_USER      64

#endif
