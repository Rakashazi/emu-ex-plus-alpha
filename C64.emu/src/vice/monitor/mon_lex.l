/* -*- C -*-
 *
 * mon_lex.l - Lexer for the VICE built-in monitor.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Andreas Boose <viceteam@t-online.de>
 *  Thomas Giesel <skoe@directbox.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

%{

/* Lexer for x64 monitor */

#include "vice.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "lib.h"
#include "mon_command.h"
#include "montypes.h"
#include "asm.h"
#include "mon_parse.h" /* FIXME ! */
#include "types.h"


#define YY_ALWAYS_INTERACTIVE 1
#define YY_NO_UNISTD_H

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

int new_cmd = 1, quote = 0, opt_asm = 0, cur_len = 0, last_len = 0, dont_match_reg_a = 0;
int temp, push_back;

#ifndef YY_SKIP_YYWRAP
static int yywrap(void);
#endif

YY_BUFFER_STATE my_state;

#define YY_USER_ACTION { last_len = cur_len; cur_len += yyleng; }

static int hexint(char n) {
    if ((n >= '0') && (n <= '9')) {
        return n - '0';
    } else if ((n >= 'a') && (n <= 'f')) {
        return n - ('a' - 0x0a);
    } else if ((n >= 'A') && (n <= 'F')) {
        return n - ('A' - 0x0a);
    }
    return 0;
}

/* remove escaped codes from string and replace them by their actual character
   codes. */
static void unescape(char *s) {
    char *d = s;
    int n;
    while (*s) {
        if (*s == '\\') {
            s++;
            switch (*s) {
                case '\\': *d = '\\'; break; /* backslash */
                case 'n': *d = '\n'; break; /* lf */
                case 'r': *d = '\r'; break; /* cr */
                case '"': *d = '"';  break; /* quote */
                case 'x': /* hex codes */
                    s++; if (*s == 0) { goto send; }
                    n = hexint(*s) << 4;
                    s++; if (*s == 0) { goto send; }
                    *d = n | hexint(*s);
                    break;
                default:
                    *d = '\\'; d++;
                    if (*s == 0) { goto send; }
                    *d = *s;
                    break;
            }
        } else {
            *d = *s;
        }
        s++;d++;
    }
send:
    *d = 0;
}

%}

%option case-insensitive
%option noinput
%option nounput

HDIGIT  [0-9a-f]

%x FNAME CMD STR ROL ASM_MODE COND_MODE REG_ASGN LABEL_ASGN BNAME CTYPE RADIX

%%

%{
   if (new_cmd) {
      if (!(asm_mode && opt_asm)) {
         last_len = cur_len = 0;
      }
      if (asm_mode) {
         BEGIN (ASM_MODE);
         opt_asm = 0;
      } else {
         BEGIN (CMD);
      }
      new_cmd = 0;
   }
%}

<CMD>{
        ";"             { BEGIN(ROL);           return CMD_COMMENT; }
        ~               { BEGIN(INITIAL);       return CONVERT_OP; }
        >               { BEGIN(INITIAL);       return CMD_ENTER_DATA; }
        @               { BEGIN(ROL);           return CMD_DISK; }
        a               { opt_asm = 1; BEGIN(INITIAL); return CMD_ASSEMBLE; }
        add_label|al    { BEGIN(INITIAL);       return CMD_ADD_LABEL; }
        attach          { BEGIN(FNAME);         return CMD_ATTACH; }
        autostart       { BEGIN(FNAME);         return CMD_AUTOSTART; }
        autoload        { BEGIN(FNAME);         return CMD_AUTOLOAD; }
        bank            { BEGIN(BNAME);         return CMD_BANK; }
        bload|bl        { BEGIN(FNAME);         return CMD_BLOAD; }
        block_read|br   { BEGIN(INITIAL);       return CMD_BLOCK_READ; }
        break|bk        { BEGIN(INITIAL);       return CMD_BREAK; }
        bsave|bs        { BEGIN(FNAME);         return CMD_BSAVE; }
        backtrace|bt    { BEGIN(INITIAL);       return CMD_BACKTRACE; }
        block_write|bw  { BEGIN(INITIAL);       return CMD_BLOCK_WRITE; }
        cartfreeze      { BEGIN(INITIAL);       return CMD_CARTFREEZE; }
        cd              { BEGIN(ROL);           return CMD_CHDIR; }
        clear_labels|cl { BEGIN(INITIAL);       return CMD_CLEAR_LABELS; }
        command         { BEGIN(INITIAL);       return CMD_COMMAND; }
        compare|c       { BEGIN(INITIAL);       return CMD_COMPARE; }
        condition|cond  { BEGIN(INITIAL);       return CMD_CONDITION; }
        cpu             { BEGIN(CTYPE);         return CMD_CPU; }
        cpuhistory|chis { BEGIN(INITIAL);       return CMD_CPUHISTORY; }
        dir|ls          { BEGIN(ROL);           return CMD_DIR; }
        disass|d        { BEGIN(INITIAL);       return CMD_DISASSEMBLE; }
        delete|del      { BEGIN(INITIAL);       return CMD_DELETE; }
        delete_label|dl { BEGIN(INITIAL);       return CMD_DEL_LABEL; }
        device|dev      { BEGIN(INITIAL);       return CMD_DEVICE; }
        detach          { BEGIN(INITIAL);       return CMD_DETACH; }
        disable|dis     { BEGIN(INITIAL);       return CMD_CHECKPT_OFF; }
        dump            { BEGIN(FNAME);         return CMD_DUMP; }
        enable|en       { BEGIN(INITIAL);       return CMD_CHECKPT_ON; }
        exit|x          { BEGIN(INITIAL);       return CMD_EXIT; }
        export|exp      { BEGIN(INITIAL);       return CMD_EXPORT; }
        fill|f          { BEGIN(INITIAL);       return CMD_FILL; }
        goto|g          { BEGIN(INITIAL);       return CMD_GOTO; }
        help|"?"        { BEGIN(ROL);           return CMD_HELP; }
        hunt|h          { BEGIN(INITIAL);       return CMD_HUNT; }
        i               { BEGIN(INITIAL);       return CMD_TEXT_DISPLAY; }
        ii              { BEGIN(INITIAL);       return CMD_SCREENCODE_DISPLAY; }
        ignore          { BEGIN(INITIAL);       return CMD_IGNORE; }
        io              { BEGIN(INITIAL);       return CMD_IO; }
        keybuf          { BEGIN(ROL);           return CMD_KEYBUF; }
        list            { BEGIN(INITIAL);       return CMD_LIST; }
        load|l          { BEGIN(FNAME);         return CMD_LOAD; }
        load_labels|ll  { BEGIN(FNAME);         return CMD_LOAD_LABELS; }
        mem|m           { BEGIN(INITIAL);       return CMD_MEM_DISPLAY; }
        memchar|mc      { BEGIN(INITIAL);       return CMD_CHAR_DISPLAY; }
        memmapsave|mmsave { BEGIN(FNAME);       return CMD_MEMMAPSAVE; }
        memmapshow|mmsh { BEGIN(INITIAL);       return CMD_MEMMAPSHOW; }
        memmapzap|mmzap { BEGIN(INITIAL);       return CMD_MEMMAPZAP; }
        move|t          { BEGIN(INITIAL);       return CMD_MOVE; }
        memsprite|ms    { BEGIN(INITIAL);       return CMD_SPRITE_DISPLAY; }
        next|n          { BEGIN(INITIAL);       return CMD_NEXT; }
        playback|pb     { BEGIN(FNAME);         return CMD_PLAYBACK; }
        print|p         { BEGIN(INITIAL);       return CMD_PRINT; }
        pwd             { BEGIN(INITIAL);       return CMD_PWD; }
        quit            { BEGIN(INITIAL);       return CMD_QUIT; }
        radix|rad       { BEGIN(RADIX);         return CMD_RADIX; }
        record|rec      { BEGIN(FNAME);         return CMD_RECORD; }
        registers|r     { BEGIN(REG_ASGN);      return CMD_REGISTERS; }
        reset           { BEGIN(INITIAL);       return CMD_MON_RESET; }
        resourceget|resget { BEGIN(INITIAL);    return CMD_RESOURCE_GET; }
        resourceset|resset { BEGIN(INITIAL);    return CMD_RESOURCE_SET; }
        load_resources|resload  { BEGIN(FNAME); return CMD_LOAD_RESOURCES; }
        save_resources|ressave  { BEGIN(FNAME); return CMD_SAVE_RESOURCES; }
        return|ret      { BEGIN(INITIAL);       return CMD_RETURN; }
        save|s          { BEGIN(FNAME);         return CMD_SAVE; }
        save_labels|sl  { BEGIN(FNAME);         return CMD_SAVE_LABELS; }
        screen|sc       { BEGIN(INITIAL);       return CMD_SCREEN; }
        screenshot|scrsh { BEGIN(FNAME);        return CMD_SCREENSHOT; }
        show_labels|shl { BEGIN(INITIAL);       return CMD_SHOW_LABELS; }
        sidefx|sfx      { BEGIN(INITIAL);       return CMD_SIDEFX; }
        step|z          { BEGIN(INITIAL);       return CMD_STEP; }
        stop            { BEGIN(INITIAL);       return CMD_MON_STOP; }
        stopwatch|sw    { BEGIN(INITIAL);       return CMD_STOPWATCH; }
        tapectrl        { BEGIN(INITIAL);       return CMD_TAPECTRL; }
        trace|tr        { BEGIN(INITIAL);       return CMD_TRACE; }
        until|un        { BEGIN(INITIAL);       return CMD_UNTIL; }
        undump          { BEGIN(FNAME);         return CMD_UNDUMP; }
        watch|w         { BEGIN(INITIAL);       return CMD_WATCH; }
        yydebug         { BEGIN(INITIAL);       return CMD_YYDEBUG; }
}
 /* this is not a real command, but could be a label name for a label assignment */
<CMD>[_a-zA-Z][_a-zA-Z0-9]* { BEGIN(LABEL_ASGN); yylval.str = lib_stralloc(yytext); return CMD_LABEL_ASGN; }

;		{ new_cmd = 1; return CMD_SEP; }

on  		{ yylval.i = e_ON; return TOGGLE; }
off 		{ yylval.i = e_OFF; return TOGGLE; }
toggle		{ yylval.i = e_TOGGLE; return TOGGLE; }

load { yylval.i = e_load; return MEM_OP; }
store { yylval.i = e_store; return MEM_OP; }
exec { yylval.i = e_exec; return MEM_OP; }

reset		{ return RESET; }

if		{ BEGIN (COND_MODE); return IF; }

\"		{ if (!quote) {
                     quote = 1;
                     BEGIN (STR);
                  }
                }

<STR>\" 	{ if (quote) {
                     if ((yyleng > 1) && (yytext[yyleng - 2] == '\\')) {
                         yymore(); /* skip escaped quote */
                     } else if (((yyleng < 4) || (yytext[yyleng - 4] != '\\')) &&
                                (yyleng > 2) && (yytext[yyleng - 3] == '\\') &&
                                (yytext[yyleng - 2] == '"')) {
                         /* special case, \"" at end of string */
                         yylval.str = lib_stralloc(yytext);
                         yylval.str[yyleng -1] = 0;
                         unescape(yylval.str);
                         quote = 0;
                         BEGIN (INITIAL);
                         return STRING;
                     } else {
                         quote = 0;
                         BEGIN (INITIAL);
                     }
                  }
                }

<STR>[^\"\n]*   { if (yytext[yyleng - 1] == '\\') {
                     yymore(); /* skip escaped quote */
                  } else {
                     yylval.str = lib_stralloc(yytext);
                     unescape(yylval.str);
                     return STRING;
                  }
                }

 /* prefixes for numbers */
\.\$		{ yylval.i = e_hexadecimal; return INPUT_SPEC; }
\.\+		{ yylval.i = e_decimal; return INPUT_SPEC; }
\.%		{ yylval.i = e_binary; return INPUT_SPEC; }
\.&		{ yylval.i = e_octal; return INPUT_SPEC; }
 /* \.TA		{ yylval.i = e_text_ascii; return INPUT_SPEC; } */
 /* \.TP		{ yylval.i = e_text_petscii; return INPUT_SPEC; } */
 /* \.\>		{ yylval.i = e_6502_asm; return INPUT_SPEC; } */
 /* \.S		{ yylval.i = e_sprite; return INPUT_SPEC; } */
 /* \.C		{ yylval.i = e_character; return INPUT_SPEC; } */

 /* used by the "radix" command */
<RADIX>D|d		{ yylval.i = e_decimal; return RADIX_TYPE; }
<RADIX>H|h		{ yylval.i = e_hexadecimal; return RADIX_TYPE; }
<RADIX>B|b		{ yylval.i = e_binary; return RADIX_TYPE; }
<RADIX>O|o		{ yylval.i = e_octal; return RADIX_TYPE; }
 /* <RADIX>C		{ yylval.i = e_character; return RADIX_TYPE; } */
 /* <RADIX>S		{ yylval.i = e_sprite; return RADIX_TYPE; } */
 /* <RADIX>TA		{ yylval.i = e_text_ascii; return RADIX_TYPE; } */
 /* <RADIX>TP		{ yylval.i = e_text_petscii; return RADIX_TYPE; } */

 /* used (currently) by the "memory" command */
<INITIAL>H|h        { yylval.i = e_hexadecimal; return RADIX_TYPE; }
<INITIAL>O|o        { yylval.i = e_octal; return RADIX_TYPE; }
 /* 'b' and 'd' are ambiguous (could be either radix or hex value),
    work around this by only accepting uppercase 'B' and 'D' as the
    (fortunately, rarely used) radix argument to the memory command */
<INITIAL>D          { if (yytext[0] == 'D') {
                        yylval.i = e_decimal; return RADIX_TYPE;
                      } else {
                        yylval.i = 0x0d; return H_NUMBER;
                      }
                    }
<INITIAL>B          { if (yytext[0] == 'B') {
                        yylval.i = e_binary; return RADIX_TYPE;
                      } else {
                        yylval.i = 0x0b; return H_NUMBER;
                      }
                    }

<BNAME,INITIAL,COND_MODE>(c|C):          { return MEM_COMP; }
<BNAME,INITIAL,COND_MODE>8:              { return MEM_DISK8; }
<BNAME,INITIAL,COND_MODE>9:              { return MEM_DISK9; }
<BNAME,INITIAL,COND_MODE>10:             { return MEM_DISK10; }
<BNAME,INITIAL,COND_MODE>11:             { return MEM_DISK11; }

<CTYPE>[_a-zA-Z0-9]*      { yylval.str = lib_stralloc(yytext); return CPUTYPE; }

<*>[ \t]*
<*>\n          { dont_match_reg_a = 0;
                 new_cmd = 1;
                 opt_asm = 0;
                 quote = 0;
                 BEGIN(INITIAL);
                 return TRAIL;
               }

<FNAME>\".+\" { yytext[yyleng-1] = '\0';
                yylval.str = lib_stralloc(yytext+1);
                BEGIN (INITIAL); return FILENAME; }

<BNAME>[_a-zA-Z0-9]*      { yylval.str = lib_stralloc(yytext); return BANKNAME; }

<ROL>[^ \t\n][^\n]*          { yylval.str = lib_stralloc(yytext); return R_O_L; }

 /* see monitor.c and montypes.h for the commented list */
<REG_ASGN>{
        a   { yylval.i = e_A; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
        x   { yylval.i = e_X; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09 */
        y   { yylval.i = e_Y; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09 */
        pc  { yylval.i = e_PC; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
        sp  { yylval.i = e_SP; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
        fl  { yylval.i = e_FLAGS; return MON_REGISTER; }	/* 65xx/c64dtv/658xx */
        cc  { yylval.i = e_FLAGS; return MON_REGISTER; }	/* 6x09 */

        b   { yylval.i = e_B; return MON_REGISTER; }		/* 658xx/6x09/z80 */
        c   { yylval.i = e_C; return MON_REGISTER; }		/* 658xx/z80 */
        dpr { yylval.i = e_DPR; return MON_REGISTER; }	/* 658xx */
        pbr { yylval.i = e_PBR; return MON_REGISTER; }	/* 658xx */
        dbr { yylval.i = e_DBR; return MON_REGISTER; }	/* 658xx */
        e   { yylval.i = e_E; return MON_REGISTER; }		/* 658xx/6309/z80 */

        r3  { yylval.i = e_R3; return MON_REGISTER; }		/* c64dtv */
        r4  { yylval.i = e_R4; return MON_REGISTER; }		/* c64dtv */
        r5  { yylval.i = e_R5; return MON_REGISTER; }		/* c64dtv */
        r6  { yylval.i = e_R6; return MON_REGISTER; }		/* c64dtv */
        r7  { yylval.i = e_R7; return MON_REGISTER; }		/* c64dtv */
        r8  { yylval.i = e_R8; return MON_REGISTER; }		/* c64dtv */
        r9  { yylval.i = e_R9; return MON_REGISTER; }		/* c64dtv */
        r10 { yylval.i = e_R10; return MON_REGISTER; }	/* c64dtv */
        r11 { yylval.i = e_R11; return MON_REGISTER; }	/* c64dtv */
        r12 { yylval.i = e_R12; return MON_REGISTER; }	/* c64dtv */
        r13 { yylval.i = e_R13; return MON_REGISTER; }	/* c64dtv */
        r14 { yylval.i = e_R14; return MON_REGISTER; }	/* c64dtv */
        r15 { yylval.i = e_R15; return MON_REGISTER; }	/* c64dtv */
        acm { yylval.i = e_ACM; return MON_REGISTER; }	/* c64dtv */
        yxm { yylval.i = e_YXM; return MON_REGISTER; }	/* c64dtv */

        d   { yylval.i = e_D; return MON_REGISTER; }		/* 6x09/z80 */
        dp  { yylval.i = e_DP; return MON_REGISTER; }		/* 6x09 */
        u   { yylval.i = e_U; return MON_REGISTER; }		/* 6x09 */

        af  { yylval.i = e_AF; return MON_REGISTER; }		/* z80 */
        bc  { yylval.i = e_BC; return MON_REGISTER; }		/* z80 */
        de  { yylval.i = e_DE; return MON_REGISTER; }		/* z80 */
        hl  { yylval.i = e_HL; return MON_REGISTER; }		/* z80 */
        ix  { yylval.i = e_IX; return MON_REGISTER; }		/* z80 */
        iy  { yylval.i = e_IY; return MON_REGISTER; }		/* z80 */
        i   { yylval.i = e_I; return MON_REGISTER; }		/* z80 */
        r   { yylval.i = e_R; return MON_REGISTER; }		/* z80 */
        af2 { yylval.i = e_AF2; return MON_REGISTER; }	/* z80 */
        bc2 { yylval.i = e_BC2; return MON_REGISTER; }	/* z80 */
        de2 { yylval.i = e_DE2; return MON_REGISTER; }	/* z80 */
        hl2 { yylval.i = e_HL2; return MON_REGISTER; }	/* z80 */

        f   { yylval.i = e_F; return MON_REGISTER; }		/* 6309 */
        w   { yylval.i = e_W; return MON_REGISTER; }		/* 6309 */
        q   { yylval.i = e_Q; return MON_REGISTER; }		/* 6309 */
        v   { yylval.i = e_V; return MON_REGISTER; }		/* 6309 */
        md  { yylval.i = e_MD; return MON_REGISTER; }		/* 6309 */

        h   { yylval.i = e_H; return MON_REGISTER; }		/* z80 */
        l   { yylval.i = e_L; return MON_REGISTER; }		/* z80 */
        ixl { yylval.i = e_IXL; return MON_REGISTER; }	/* z80 */
        ixh { yylval.i = e_IXH; return MON_REGISTER; }	/* z80 */
        iyl { yylval.i = e_IYL; return MON_REGISTER; }	/* z80 */
        iyh { yylval.i = e_IYH; return MON_REGISTER; }	/* z80 */
}

<COND_MODE>{
	== 		{ yylval.i = e_EQU; return COMPARE_OP; }
	!= 		{ yylval.i = e_NEQ; return COMPARE_OP; }
	\<=		{ yylval.i = e_LTE; return COMPARE_OP; }
	\<		{ yylval.i = e_LT;  return COMPARE_OP; }
	\>		{ yylval.i = e_GT;  return COMPARE_OP; }
	\>=		{ yylval.i = e_GTE; return COMPARE_OP; }
	&&		{ yylval.i = e_AND; return COMPARE_OP; }
	\|\|		{ yylval.i = e_OR;  return COMPARE_OP; }

	A 		{ yylval.i = e_A; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
	X 		{ yylval.i = e_X; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09 */
	Y 		{ yylval.i = e_Y; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09 */
	PC 		{ yylval.i = e_PC; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
	SP		{ yylval.i = e_SP; return MON_REGISTER; }		/* 65xx/c64dtv/658xx/6x09/z80 */
	FL		{ yylval.i = e_FLAGS; return MON_REGISTER; }	/* 65xx/c64dtv/658xx */
	CC		{ yylval.i = e_FLAGS; return MON_REGISTER; }	/* 6x09 */

	B		{ yylval.i = e_B; return MON_REGISTER; }		/* 658xx/6x09/z80 */
	C		{ yylval.i = e_C; return MON_REGISTER; }		/* 658xx/z80 */
	DPR		{ yylval.i = e_DPR; return MON_REGISTER; }		/* 658xx */
	PBR		{ yylval.i = e_PBR; return MON_REGISTER; }		/* 658xx */
	DBR		{ yylval.i = e_DBR; return MON_REGISTER; }		/* 658xx */
	E		{ yylval.i = e_E; return MON_REGISTER; }		/* 658xx/6309/z80 */

	R3		{ yylval.i = e_R3; return MON_REGISTER; }		/* c64dtv */
	R4		{ yylval.i = e_R4; return MON_REGISTER; }		/* c64dtv */
	R5		{ yylval.i = e_R5; return MON_REGISTER; }		/* c64dtv */
	R6		{ yylval.i = e_R6; return MON_REGISTER; }		/* c64dtv */
	R7		{ yylval.i = e_R7; return MON_REGISTER; }		/* c64dtv */
	R8		{ yylval.i = e_R8; return MON_REGISTER; }		/* c64dtv */
	R9		{ yylval.i = e_R9; return MON_REGISTER; }		/* c64dtv */
	R10		{ yylval.i = e_R10; return MON_REGISTER; }		/* c64dtv */
	R11		{ yylval.i = e_R11; return MON_REGISTER; }		/* c64dtv */
	R12		{ yylval.i = e_R12; return MON_REGISTER; }		/* c64dtv */
	R13		{ yylval.i = e_R13; return MON_REGISTER; }		/* c64dtv */
	R14		{ yylval.i = e_R14; return MON_REGISTER; }		/* c64dtv */
	R15		{ yylval.i = e_R15; return MON_REGISTER; }		/* c64dtv */
	ACM		{ yylval.i = e_ACM; return MON_REGISTER; }		/* c64dtv */
	YXM		{ yylval.i = e_YXM; return MON_REGISTER; }		/* c64dtv */

	D		{ yylval.i = e_D; return MON_REGISTER; }		/* 6x09/z80 */
	DP		{ yylval.i = e_DP; return MON_REGISTER; }		/* 6x09 */
	U		{ yylval.i = e_U; return MON_REGISTER; }		/* 6x09 */

	AF		{ yylval.i = e_AF; return MON_REGISTER; }		/* z80 */
	BC		{ yylval.i = e_BC; return MON_REGISTER; }		/* z80 */
	DE		{ yylval.i = e_DE; return MON_REGISTER; }		/* z80 */
	HL		{ yylval.i = e_HL; return MON_REGISTER; }		/* z80 */
	IX		{ yylval.i = e_IX; return MON_REGISTER; }		/* z80 */
	IY		{ yylval.i = e_IY; return MON_REGISTER; }		/* z80 */
	I		{ yylval.i = e_I; return MON_REGISTER; }		/* z80 */
	R		{ yylval.i = e_R; return MON_REGISTER; }		/* z80 */
	AF2		{ yylval.i = e_AF2; return MON_REGISTER; }		/* z80 */
	BC2		{ yylval.i = e_BC2; return MON_REGISTER; }		/* z80 */
 	DE2		{ yylval.i = e_DE2; return MON_REGISTER; }		/* z80 */
	HL2		{ yylval.i = e_HL2; return MON_REGISTER; }		/* z80 */

	F		{ yylval.i = e_F; return MON_REGISTER; }		/* 6309 */
	W		{ yylval.i = e_W; return MON_REGISTER; }		/* 6309 */
	Q		{ yylval.i = e_Q; return MON_REGISTER; }		/* 6309 */
	V		{ yylval.i = e_V; return MON_REGISTER; }		/* 6309 */
	MD		{ yylval.i = e_MD; return MON_REGISTER; }		/* 6309 */

	H		{ yylval.i = e_H; return MON_REGISTER; }		/* z80 */
	L		{ yylval.i = e_L; return MON_REGISTER; }		/* z80 */
	IXH		{ yylval.i = e_IXH; return MON_REGISTER; }		/* z80 */
	IXL		{ yylval.i = e_IXL; return MON_REGISTER; }		/* z80 */
	IYH		{ yylval.i = e_IYH; return MON_REGISTER; }		/* z80 */
	IYL		{ yylval.i = e_IYL; return MON_REGISTER; }		/* z80 */

	\(		{ return L_PAREN; }
	\)		{ return R_PAREN; }
        \[              { return L_BRACKET; }
        \]              { return R_BRACKET; }
}

<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\,[a-zA-Z] {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\,\([a-zA-Z][a-zA-Z]\) {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ \([a-zA-Z][a-zA-Z]\)\,[a-zA-Z] {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ \([a-zA-Z][a-zA-Z]\)\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z][a-zA-Z]\ [0-7]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z][a-zA-Z][a-zA-Z]\ [a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z][a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>af|AF         { return REG_AF; }
<ASM_MODE>bc|BC         { return REG_BC; }
<ASM_MODE>de|DE         { return REG_DE; }
<ASM_MODE>hl|HL         { return REG_HL; }
<ASM_MODE>ix|IX         { return REG_IX; }
<ASM_MODE>ixh|IXH       { return REG_IXH; }
<ASM_MODE>ixl|IXL       { return REG_IXL; }
<ASM_MODE>iy|IY         { return REG_IY; }
<ASM_MODE>iyh|IYH       { return REG_IYH; }
<ASM_MODE>iyl|IYL       { return REG_IYL; }
<ASM_MODE>[a-zA-Z]{2,4}                           { yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE,INITIAL>\.[_a-zA-Z][_a-zA-Z0-9]*        { yylval.str = lib_stralloc(yytext); return LABEL; }
<ASM_MODE,INITIAL>\.[_@\?:a-zA-Z][_@\?:a-zA-Z0-9]*        { yylval.str = lib_stralloc(yytext); return LABEL; }

<ASM_MODE>(a|A)/[ \t]*[:\n]           { if (!dont_match_reg_a) return REG_A;
                                        yylval.i = 0x0a; return H_NUMBER; }
<ASM_MODE>b|B           { return REG_B; }
<ASM_MODE>c|C           { return REG_C; }
<ASM_MODE>d|D           { return REG_D; }
<ASM_MODE>e|E           { return REG_E; }
<ASM_MODE>h|H           { return REG_H; }
<ASM_MODE>l|L           { return REG_L; }
<ASM_MODE>s|S           { return REG_S; }
<ASM_MODE>u|U           { return REG_U; }
<ASM_MODE>x|X           { return REG_X; }
<ASM_MODE>y|Y           { return REG_Y; }

<ASM_MODE>\(            { dont_match_reg_a = 1; return L_PAREN; }
<ASM_MODE>\)            { dont_match_reg_a = 0; return R_PAREN; }
<ASM_MODE>\[            { dont_match_reg_a = 1; return L_BRACKET; }
<ASM_MODE>\]            { dont_match_reg_a = 0; return R_BRACKET; }
<ASM_MODE>#             { dont_match_reg_a = 1; return ARG_IMMEDIATE; }
<ASM_MODE>:             { dont_match_reg_a = 0; return INST_SEP; }
<ASM_MODE>\<            { dont_match_reg_a = 1; return LESS_THAN; }
<ASM_MODE>\+            { return PLUS; }
<ASM_MODE>-             { return MINUS; }

<ASM_MODE,INITIAL,COND_MODE,REG_ASGN,LABEL_ASGN>{
    {HDIGIT}{8}             { yylval.str = lib_stralloc(yytext); return H_RANGE_GUESS; }
    1[01]*                  { yylval.str = lib_stralloc(yytext); return B_NUMBER_GUESS; }
    %[01]+                  { yylval.i = strtol(yytext+1, NULL, 2); return B_NUMBER; }
    &[0-7]+                 { yylval.i = strtol(yytext+1, NULL, 8); return O_NUMBER; }
    [0-7]*                  { yylval.str = lib_stralloc(yytext); return O_NUMBER_GUESS; }
    \+[0-9]+                { yylval.i = strtol(yytext+1, NULL, 10); return D_NUMBER; }
    [0-9]+                  { yylval.str = lib_stralloc(yytext); return D_NUMBER_GUESS; }
    xx                      { yylval.i = 0x00; return MASK; }
    \${HDIGIT}+             { yylval.i = strtol(yytext+1, NULL, 16); return H_NUMBER; }
    {HDIGIT}+               { yylval.i = strtol(yytext, NULL, 16); return H_NUMBER; }
}

<*>=	{ return EQUALS; }
<*>,	{ return COMMA; }

 /* After a label assignment there may be a ; comment to EOL */
<LABEL_ASGN>;.*[^\n]	{ return LABEL_ASGN_COMMENT; }

<*>[^ \t]		{ return yytext[0]; }

%%

void free_buffer(void)
{
   yy_delete_buffer(my_state);
}

void make_buffer(char *str)
{
    my_state = yy_scan_buffer(str, strlen(str) + 2);
    assert(my_state);
}

#ifndef YY_SKIP_YYWRAP
static int yywrap(void)
{
    return 1;
}
#endif
