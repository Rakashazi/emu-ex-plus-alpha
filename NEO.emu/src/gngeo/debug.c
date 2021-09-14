#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif


//#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include <imagine/logger/logger.h>

/* TODO: finish it ...... */

#define MAX_BP 500
#define MAX_BT 20

int breakpoints[MAX_BP];
int nb_breakpoints = 0;
int dbg_step = 0;
Uint32 backtrace[MAX_BT];

void add_bt(Uint32 pc)
{
    int i;
    for(i=MAX_BT-1;i>=0;i--)
	backtrace[i+1]=backtrace[i];
    backtrace[0]=pc;
}

void show_bt(void) {
    int i;
    for(i=MAX_BT-1;i>=0;i--)
    	logMsg("%08x\n",backtrace[i]);
}

void add_cond(Uint8 type,int reg,Uint32 val) {
}

int check_bp(int pc)
{
    int i;
    for (i = 0; i < nb_breakpoints; i++) {
	if (breakpoints[i] == pc)
	    return 1;
    }
    return 0;
}

void add_bp(int pc)
{
    if (nb_breakpoints > MAX_BP) {
    	logMsg("Too many breakpoint\n");
	return;
    }
    breakpoints[nb_breakpoints++] = pc;
}

void del_bp(int pc)
{
    int i;
    for (i = 0; i < nb_breakpoints; i++) {
	if (breakpoints[i] == pc)
	    breakpoints[i] = -1;
    }
}
#if 0
void debug_interf(void)
{
    char in_buf[256];
    char val[32];
    char cmd;
    int in_a, in_b, in_c;
    int i, j;
    while (1) {
	printf("> ");
	fflush(stdout);
	//    in_buf[0]=0;
	// memset(in_buf,0,255);
	//scanf("%s %s",in_buf);
	fgets(in_buf, 255, stdin);
	switch (in_buf[0]) {
	case 'q':
	case 'c':
	    dbg_step = 0;
	    return;
	    break;
	case 's':
	    dbg_step = 1;
	    return;
	case 'b':
	    sscanf(in_buf, "%c %x", &cmd, &in_a);
	    add_bp(in_a);
	    break;
	case 'B':
	    sscanf(in_buf, "%c %x", &cmd, &in_a);
	    del_bp(in_a);
	    break;
	case 'd':
	    sscanf(in_buf, "%c %s", &cmd, val);
	    /*
	       printf("in : %s\n",in_buf);
	       printf("val: %s\n",val);
	     */
	    in_a = strtol(val, NULL, 0);
	    //      printf("ina: %x\n",in_a);
	    cpu_68k_disassemble(in_a, 10);
	    break;
	case 'p':
	    cpu_68k_dumpreg();
	    break;
	}
	printf("\n");
    }
}

int dbg_68k_run(Uint32 nbcycle)
{
    int i = 0;
    while (i < nbcycle) {
	//    printf("%x\n",cpu_68k_getpc());
	if (check_bp(cpu_68k_getpc())) {
	    cpu_68k_disassemble(cpu_68k_getpc(), 1);
	    debug_interf();
	}
	i += cpu_68k_run_step();
    }
    return i;
}

#endif
