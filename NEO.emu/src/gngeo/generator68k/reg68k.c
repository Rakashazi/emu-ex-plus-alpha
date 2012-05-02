/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include "registers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "reg68k.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "compile.h"


/*** global variables ***/
/*
#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) \
       || defined(PROCESSOR_INTEL)))
#endif
*/

//#endif


//extern uint8 current_cpu_bank;
extern uint32 bankaddress;

/*** forward references ***/

void reg68k_printstat(void) {
    unsigned int i;
    t_ipclist *list;
    for (i=0;i<LEN_IPCLISTTABLE;i++) {
	list = ipclist[i];
        while (list) {
        	logMsg("%08x %d\n", list->pc, (unsigned)list->pass);
	    list = list->next;
        }
    }
}	



/*** reg68k_external_step - execute one instruction ***/


unsigned int reg68k_external_step(void)
{
  static t_ipc ipc;
  static t_iib *piib;
  static unsigned int clks;

  /* !!! entering global register usage area !!! */

  if (regs.pending && ((regs.sr.sr_int >> 8) & 7) < regs.pending)
      reg68k_internal_autovector(regs.pending);
  
  if (!(piib = cpu68k_iibtable[fetchword(regs.pc)]))
  	logMsg("Invalid instruction @ %08X [%04X]\n", (unsigned)regs.pc,
	     fetchword(regs.pc));
  
  cpu68k_ipc(regs.pc,
	     mem68k_memptr[(regs.pc >> 12) & 0xfff] (regs.pc &
						       0xFFFFFF), piib,
	     &ipc);
  cpu68k_functable[fetchword(regs.pc) * 2 + 1] (&ipc);
  clks = piib->clocks;

  cpu68k_clocks += clks;
  return clks;                  /* number of clocks done */
}

/*** reg68k_external_execute - execute at least given number of clocks,
     and return number of clocks executed too much ***/

unsigned int reg68k_external_execute(unsigned int clocks)
{
  unsigned int index;
  t_ipclist *list;
  t_ipc *ipc;
  uint32 pc24;

  uint32 bank;
  static t_ipc step_ipc;
  static t_iib *step_piib;
  static int clks;


  clks = clocks;

    if (regs.pending && ((regs.sr.sr_int >> 8) & 7) < regs.pending)
      reg68k_internal_autovector(regs.pending);

    do {
      pc24 = regs.pc & 0xffffff;
//      if ((pc24 & 0xff0000) == 0xff0000) {
      if ((pc24&0xF00000)==0x200000)
	  bank=bankaddress;
      else
	  bank=0;
      if ((pc24>>16) == 0x10) { /* Modif : neogeo RAM is 0x100000 - 0x10FFFF */

        /* executing code from RAM, do not use compiled information */
        do {
          step_piib = cpu68k_iibtable[fetchword(regs.pc)];
          if (!step_piib)
          	logMsg("Invalid instruction (iib assert) @ %08X\n",
		  (unsigned)regs.pc);
          cpu68k_ipc(regs.pc,
                     mem68k_memptr[(regs.pc >> 12) &
				  0xfff] (regs.pc & 0xFFFFFF),
                     step_piib, &step_ipc);
          cpu68k_functable[fetchword(regs.pc) * 2 + 1] (&step_ipc);
          clks -= step_piib->clocks;
          cpu68k_clocks += step_piib->clocks;
        }
        while (!step_piib->flags.endblk);
        list = NULL;            /* stop compiler warning ;(  */
      } else {
        index = (pc24 >> 1) & (LEN_IPCLISTTABLE - 1);
        list = ipclist[index];
        while (list && (list->pc != pc24 || list->bank!=bank)) {
	    //while (list && (list->pc != pc24)) {
          list = list->next;
        }
	//printf("TOOT\n");
//#ifdef GENERATOR_JIT
#if ((defined PROCESSOR_SPARC) && (defined GENERATOR_JIT))
	if (!list) {
	    list = cpu68k_makeipclist(pc24);
	    list->pass=0;
	    list->next = ipclist[index];
	    ipclist[index] = list;
	    list->compiled = compile_make(list);
	}
	list->pass++;
	//printf("first ipc=%p\n",(t_ipc *) (list + 1));
	list->compiled((t_ipc *) (list + 1));
	//printf("PC=%x\n",regs.pc);
#else
        if (!list) {
          /* LOG_USER(("Making IPC list @ %08x", pc24)); */
	    //printf("Making IPC list @ %08x\n", pc24);
          list = cpu68k_makeipclist(pc24);
          list->next = ipclist[index];
          ipclist[index] = list;
        }
        ipc = (t_ipc *) (list + 1);
	//printf("Exe IPC list @ %08x\n", pc24);
        do {
          ipc->function(ipc);
          ipc++;
        }
        while (*(int *)ipc);
#endif
        //do {
          clks -= list->clocks;
          cpu68k_clocks += list->clocks;
	  //} while (list->norepeat && clks > 0);
      }
    }
    while (clks > 0);

  return -clks;                 /* i.e. number of clocks done too much */
}

/*** reg68k_external_autovector - for external use ***/

void reg68k_external_autovector(int avno)
{

  reg68k_internal_autovector(avno);

}

/*** reg68k_internal_autovector - go to autovector - this call assumes global
     registers are already setup ***/

/* interrupts must not occur during cpu68k_frozen, as this flag indicates
   that we are catching up events due to a dma transfer.  Since the dma
   transfer is triggered by a memory write at which stage the current value
   of the PC is not written anywhere (due to being in the middle of a 68k
   block and it's in a local register), we MUST NOT use regs.pc - this
   routine uses reg68k_pc but this is loaded by reg68k_external_autovector,
   which is called by event_nextevent() and therefore will be a *WRONG*
   reg68k_pc! */

void reg68k_internal_autovector(int avno)
{
  int curlevel = (regs.sr.sr_int >> 8) & 7;
  uint32 tmpaddr;

  if ((curlevel < avno || avno == 7) && !cpu68k_frozen) {
    if (regs.stop) {
      LOG_DEBUG1(("stop finished"));
      /* autovector whilst in a STOP instruction */
      regs.pc += 4;
      regs.stop = 0;
    }
    if (!regs.sr.sr_struct.s) {
      regs.regs[15] ^= regs.sp;       /* swap A7 and SP */
      regs.sp ^= regs.regs[15];
      regs.regs[15] ^= regs.sp;
      regs.sr.sr_struct.s = 1;
    }
    regs.regs[15] -= 4;
    storelong(regs.regs[15], regs.pc);
    regs.regs[15] -= 2;
    storeword(regs.regs[15], regs.sr.sr_int);
    regs.sr.sr_struct.t = 0;
    regs.sr.sr_int &= ~0x0700;
    regs.sr.sr_int |= avno << 8;
    tmpaddr = regs.pc;
    regs.pc = fetchlong((V_AUTO + avno - 1) * 4);
    LOG_USER(("AUTOVECTOR %d: %X -> %X", avno, tmpaddr, regs.pc));
    regs.pending = 0;
  } else {
    LOG_USER(("%08X autovector %d pending", regs.pc, avno));
    // if (!regs.pending || regs.pending < avno) - not sure about this
    regs.pending = avno;
  }
}

/*** reg68k_internal_vector - go to vector - this call assumes global
     registers are already setup ***/

void reg68k_internal_vector(int vno, uint32 oldpc)
{
  if (!regs.sr.sr_struct.s) {
    regs.regs[15] ^= regs.sp; /* swap A7 and SP */
    regs.sp ^= regs.regs[15];
    regs.regs[15] ^= regs.sp;
    regs.sr.sr_struct.s = 1;
  }
  regs.regs[15] -= 4;
  storelong(regs.regs[15], oldpc);
  regs.regs[15] -= 2;
  storeword(regs.regs[15], regs.sr.sr_int);
  regs.pc = fetchlong(vno * 4);

}

