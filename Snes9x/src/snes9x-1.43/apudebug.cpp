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

#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"
#include "cpuexec.h"

#ifdef SPCTOOL
#include "spctool/spc700.h"
#endif

#ifdef DEBUGGER
extern int NoiseFreq [32];

FILE *apu_trace = NULL;

static char *S9xMnemonics [256] = {
"NOP", "TCALL0", "SET0 $%02X", "BBS0 $%02X,$%04X",
"OR A,$%02X", "OR A,$%04X", "OR A,(X)", "OR A,($%02X+X)",
"OR A,#$%02X", "OR $%02X,$%02X", "OR1 C,$%04X,%d", "ASL $%02X",
"ASL $%04X", "PUSH PSW", "TSET1 $%04X", "BRK",
"BPL $%04X", "TCALL1", "CLR0 $%02X", "BBC0 $%02X,$%04X",
"OR A,$%02X+X", "OR A,$%04X+X", "OR A,$%04X+Y", "OR A,($%02X)+Y",
"OR $%02X,#$%02X", "OR (X),(Y)", "DECW $%02X", "ASL $%02X+X",
"ASL A", "DEC X", "CMP X,$%04X", "JMP ($%04X+X)",
"CLRP", "TCALL2", "SET1 $%02X", "BBS1 $%02X,$%04X",
"AND A,$%02X", "AND A,$%04X", "AND A,(X)", "AND A,($%02X+X)",
"AND A,$%02X", "AND $%02X,$%02X", "OR1 C,$%04X, not %d", "ROL $%02X",
"ROL $%04X", "PUSH A", "CBNE $%02X,$%04X", "BRA $%04X",
"BMI $%04X", "TCALL3", "CLR1 $%02X", "BBC1 $%02X,$%04X",
"AND A,$%02X+X", "AND A,$%04X+X", "AND A,$%04X+Y", "AND A,($%02X)+Y",
"AND $%02X,#$%02X", "AND (X),(Y)", "INCW $%02X", "ROL $%02X+X",
"ROL A", "INC X", "CMP X,$%02X", "CALL $%04X",
"SETP", "TCALL4", "SET2 $%02X", "BBS2 $%02X,$%04X",
"EOR A,$%02X", "EOR A,$%04X", "EOR A,(X)", "EOR A,($%02X+X)",
"EOR A,#$%02X", "EOR $%02X,$%02X", "AND1 C,$%04X,%d", "LSR $%02X",
"LSR $%04X", "PUSH X", "TCLR1 $%04X", "PCALL $%02X",
"BVC $%04X", "TCALL5", "CLR2 $%02X", "BBC2 $%02X,$%04X",
"EOR A,$%02X+X", "EOR A,$%04X+X", "EOR A,$%04X+Y", "EOR A,($%02X)+Y",
"EOR $%02X,#$%02X", "EOR (X),(Y)", "CMPW YA,$%02X", "LSR $%02X+X",
"LSR A", "MOV X,A", "CMP Y,$%04X", "JMP $%04X",
"CLRC", "TCALL6", "SET3 $%02X", "BBS3 $%02X,$%04X",
"CMP A,$%02X", "CMP A,$%04X", "CMP A,(X)", "CMP A,($%02X+X)",
"CMP A,#$%02X", "CMP $%02X,$%02X", "AND1 C, $%04X, not %d", "ROR $%02X",
"ROR $%04X", "PUSH Y", "DBNZ $%02X,$%04X", "RET",
"BVS $%04X", "TCALL7", "CLR3 $%02X", "BBC3 $%02X,$%04X",
"CMP A,$%02X+X", "CMP A,$%04X+X", "CMP A,$%04X+Y", "CMP A,($%02X)+Y",
"CMP $%02X,#$%02X", "CMP (X),(Y)", "ADDW YA,$%02X", "ROR $%02X+X",
"ROR A", "MOV A,X", "CMP Y,$%02X", "RETI",
"SETC", "TCALL8", "SET4 $%02X", "BBS4 $%02X,$%04X",
"ADC A,$%02X", "ADC A,$%04X", "ADC A,(X)", "ADC A,($%02X+X)",
"ADC A,#$%02X", "ADC $%02X,$%02X", "EOR1 C,%04,%d", "DEC $%02X",
"DEC $%04X", "MOV Y,#$%02X", "POP PSW", "MOV $%02X,#$%02X",
"BCC $%04X", "TCALL9", "CLR4 $%02X", "BBC4 $%02X,$%04X",
"ADC A,$%02X+X", "ADC A,$%04X+X", "ADC A,$%04X+Y", "ADC A,($%02X)+Y",
"ADC $%02X,#$%02X", "ADC (X),(Y)", "SUBW YA,$%02X", "DEC $%02X+X",
"DEC A", "MOV X,SP", "DIV YA,X", "XCN A",
"EI", "TCALL10", "SET5 $%02X", "BBS5 $%02X,$%04X",
"SBC A,$%02X", "SBC A,$%04X", "SBC A,(X)", "SBC A,($%02X+X)",
"SBC A,#$%02X", "SBC $%02X,$%02X", "MOV1 C,$%04X,%d", "INC $%02X",
"INC $%04X", "CMP Y,#$%02X", "POP A", "MOV (X)+,A",
"BCS $%04X", "TCALL11", "CLR5 $%02X", "BBC5 $%02X,$%04X",
"SBC A,$%02X+X", "SBC A,$%04X+X", "SBC A,$%04X+Y", "SBC A,($%02X)+Y",
"SBC $%02X,#$%02X", "SBC (X),(Y)", "MOVW YA,$%02X", "INC $%02X+X",
"INC A", "MOV SP,X", "DAS", "MOV A,(X)+",
"DI", "TCALL12", "SET6 $%02X", "BBS6 $%02X,$%04X",
"MOV $%02X,A", "MOV $%04X,A", "MOV (X),A", "MOV ($%02X+X),A",
"CMP X,#$%02X", "MOV $%04X,X", "MOV1 $%04X,%d,C", "MOV $%02X,Y",
"MOV $%04X,Y", "MOV X,#$%02X", "POP X", "MUL YA",
"BNE $%04X", "TCALL13", "CLR6 $%02X", "BBC6 $%02X,$%04X",
"MOV $%02X+X,A", "MOV $%04X+X,A", "MOV $%04X+Y,A", "MOV ($%02X)+Y,A",
"MOV $%02X,X", "MOV $%02X+Y,X", "MOVW $%02X,YA", "MOV $%02X+X,Y",
"DEC Y", "MOV A,Y", "CBNE $%02X+X,$%04X", "DAA",
"CLRV", "TCALL14", "SET7 $%02X", "BBS7 $%02X,$%04X",
"MOV A,$%02X", "MOV A,$%04X", "MOV A,(X)", "MOV A,($%02X+X)",
"MOV A,#$%02X", "MOV X,$%04X", "NOT1 $%04X,%d", "MOV Y,$%02X",
"MOV Y,$%04X", "NOTC", "POP Y", "SLEEP",
"BEQ $%04X", "TCALL15", "CLR7 $%02X", "BBC7 $%02X,$%04X",
"MOV A,$%02X+X", "MOV A,$%04X+X", "MOV A,$%04X+Y", "MOV A,($%02X)+Y",
"MOV X,$%02X", "MOV X,$%02X+Y", "MOV $%02X,$%02X", "MOV Y,$%02X+X",
"INC Y", "MOV Y,A", "DBNZ Y,$%04X", "STOP"
};

#undef ABS

#define DP 0
#define ABS 1
#define IM 2
#define DP2DP 3
#define DPIM 4
#define DPREL 5
#define ABSBIT 6
#define REL 7

static uint8 Modes [256] = {
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, ABS, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, ABS, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, DPREL, REL,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, DP, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, ABS, DP,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, ABS, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, DPREL, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, DP, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, DP, IM, DPIM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, IM, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, DP, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, IM, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, ABS, ABSBIT, DP,
    ABS, DP, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DP, DP, DP, DP,
    IM, IM, DPREL, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, ABS, ABSBIT, DP,
    ABS, IM, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DP, DP, DP2DP, DP,
    IM, IM, REL, IM
};

static uint8 ModesToBytes [] = {
    2, 3, 1, 3, 3, 3, 3, 2
};

static FILE *SoundTracing = NULL;

void S9xOpenCloseSoundTracingFile (bool8 open)
{
    if (open && !SoundTracing)
    {
	SoundTracing = fopen ("sound_trace.log", "w");
    }
    else
    if (!open && SoundTracing)
    {
	fclose (SoundTracing);
	SoundTracing = NULL;
    }
}

void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
		       int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0)
{
    fprintf (SoundTracing, s, i1, i2, i3, i4, i5, i6, i7);
}

int S9xTraceAPU ()
{
    char buffer [200];
    
    uint8 b = S9xAPUOPrint (buffer, IAPU.PC - IAPU.RAM);
    if (apu_trace == NULL)
	apu_trace = fopen ("apu_trace.log", "wb");

    fprintf (apu_trace, "%s\n", buffer);
    return (b);
}

int S9xAPUOPrint (char *buffer, uint16 Address)
{
    char mnem [100];
    uint8 *p = IAPU.RAM + Address;
    int mode = Modes [*p];
    int bytes = ModesToBytes [mode];
    
    switch (bytes)
    {
    case 1:
	sprintf (buffer, "%04X %02X       ", p - IAPU.RAM, *p);
	break;
    case 2:
	sprintf (buffer, "%04X %02X %02X    ", p - IAPU.RAM, *p,
		 *(p + 1));
	break;
    case 3:
	sprintf (buffer, "%04X %02X %02X %02X ", p - IAPU.RAM, *p,
		 *(p + 1), *(p + 2));
	break;
    }

    switch (mode)
    {
    case DP:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1));
	break;
    case ABS:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1) + (*(p + 2) << 8));
	break;
    case IM:
	sprintf (mnem, S9xMnemonics [*p]);
	break;
    case DP2DP:
	sprintf (mnem, S9xMnemonics [*p], *(p + 2), *(p + 1));;
	break;
    case DPIM:
	sprintf (mnem, S9xMnemonics [*p], *(p + 2), *(p + 1));;
	break;
    case DPREL:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1),
		(int) (p + 3 - IAPU.RAM) + (signed char) *(p + 2));
	break;
    case ABSBIT:
	sprintf (mnem, S9xMnemonics [*p], (*(p + 1) + (*(p + 2) << 8)) & 0x1fff,
		*(p + 2) >> 5);
	break;
    case REL:
	sprintf (mnem, S9xMnemonics [*p],
		(int) (p + 2 - IAPU.RAM) + (signed char) *(p + 1));
	break;
    }

    sprintf (buffer, "%s %-20s A:%02X X:%02X Y:%02X S:%02X P:%c%c%c%c%c%c%c%c %03dl %04dl %04dl",
	     buffer, mnem,
	     APURegisters.YA.B.A, APURegisters.X, APURegisters.YA.B.Y,
	     APURegisters.S,
	     APUCheckNegative () ? 'N' : 'n',
	     APUCheckOverflow () ? 'V' : 'v',
	     APUCheckDirectPage () ? 'P' : 'p',
	     APUCheckBreak () ? 'B' : 'b',
	     APUCheckHalfCarry () ? 'H' : 'h',
	     APUCheckInterrupt () ? 'I' : 'i',
	     APUCheckZero () ? 'Z' : 'z',
	     APUCheckCarry () ? 'C' : 'c',
	     CPU.V_Counter,
	     CPU.Cycles,
	     APU.Cycles);
		
    return (bytes);
}

const char *as_binary (uint8 data)
{
    static char buf [9];

    for (int i = 7; i >= 0; i--)
	buf [7 - i] = ((data & (1 << i)) != 0) + '0';

    buf [8] = 0;
    return (buf);
}

void S9xPrintAPUState ()
{
    printf ("Master volume left: %d, right: %d\n",
	    SoundData.master_volume_left, SoundData.master_volume_right);
    printf ("Echo: %s %s, Delay: %d Feedback: %d Left: %d Right: %d\n",
	    SoundData.echo_write_enabled ? "on" : "off",
	    as_binary (SoundData.echo_enable),
	    SoundData.echo_buffer_size >> 9,
	    SoundData.echo_feedback, SoundData.echo_volume_left,
	    SoundData.echo_volume_right);

    printf ("Noise: %s, Frequency: %d, Pitch mod: %s\n", as_binary (APU.DSP [APU_NON]),
	    NoiseFreq [APU.DSP [APU_FLG] & 0x1f],
	    as_binary (SoundData.pitch_mod));
    extern int FilterTaps [8];

    printf ("Filter: ");
    for (int i = 0; i < 8; i++)
	printf ("%03d, ", FilterTaps [i]);
    printf ("\n");
    for (int J = 0; J < 8; J++)
    {
	register Channel *ch = &SoundData.channels[J];

	printf ("%d: ", J);
	if (ch->state == SOUND_SILENT)
	{
	    printf ("off\n");
	}
	else
	if (!(so.sound_switch & (1 << J)))
	    printf ("muted by user using channel on/off toggle\n");
	else
	{
	    int freq = ch->hertz;
	    if (APU.DSP [APU_NON] & (1 << J)) //ch->type == SOUND_NOISE)
	    {
		freq = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
		printf ("noise, ");
	    }
	    else
		printf ("sample %d, ", APU.DSP [APU_SRCN + J * 0x10]);

	    printf ("freq: %d", freq);
	    if (J > 0 && (SoundData.pitch_mod & (1 << J)) &&
		ch->type != SOUND_NOISE)
	    {
		printf ("(mod), ");
	    }
	    else
		printf (", ");

	    printf ("left: %d, right: %d, ",
		    ch->volume_left, ch->volume_right);

	    static char* envelope [] = 
	    {
		"silent", "attack", "decay", "sustain", "release", "gain",
		"inc_lin", "inc_bent", "dec_lin", "dec_exp"
	    };
	    printf ("%s envx: %d, target: %d, %ld", ch->state > 9 ? "???" : envelope [ch->state],
		    ch->envx, ch->envx_target, ch->erate);
	    printf ("\n");
	}
    }
}
#endif


