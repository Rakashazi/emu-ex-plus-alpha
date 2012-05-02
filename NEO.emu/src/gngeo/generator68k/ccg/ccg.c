/* ccg.c -- preprocessor for dynamic assemblers
 *
 * Copyright (C) 1999, 2000 Ian Piumarta <ian.piumarta@inria.fr>
 *
 * This file is part of CCG.
 *
 * CCG is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CCG is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the file COPYING for more details.
 *
 * Last edited: Thu Jan 13 12:02:56 2000 by piumarta (Ian Piumarta) on pingu
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#ifdef __STRICT_ANSI__
  /* a BSD function not declared in ANSI string.h */
  extern int strcasecmp();
#endif

#if defined(sun) && defined(__sparc__)
# include <errno.h>
# if !defined(ECHRNG)
    /* Poor old SunOS needs a little coaxing along */
    extern int vfprintf();
    extern int bcopy();
    extern int strcasecmp();
#   define memmove(d,s,l) bcopy(s,d,l)
# endif
#endif

#define	REWRITE_PSEUDO_ARGS

#define HEADER_PREFIX	"ccg/asm-"
#define LABEL_CHARS	"_A-Za-z0-9"

#define ASM_1_OPEN	"#["
#define ASM_1_CLOSE	"]#"
#define ASM_2_OPEN	"#{"
#define ASM_2_CLOSE	"}#"

#define REG_OPEN	"#("
#define REG_CLOSE	")#"

char commentChar= '#';
char escapeChar= '!';

#define ASM_1_BEGIN	"_ASM_APP_1"
#define ASM_1_END	"_ASM_NOAPP_1"
#define ASM_2_BEGIN	"_ASM_APP_2"
#define ASM_2_END	"_ASM_NOAPP_2"

#define ASM_ORG		"_ASM_ORG"
#define ASM_LBL		"_ASM_LBL"
#define ASM_DEF		"_ASM_DEF"

char *fileName= 0;
int   lineNo= 0;
char  line[256];

int quiet= 0;
int dodot= 1;
int check_insns= 1;

int asmFwdWarned= 0;

char **insn_tab= 0;

typedef char *(*rewriter)(int code);

extern rewriter rewrite;

void warning(char *fmt, ...)
{
  if (!quiet) {
    va_list ap;
    if (fileName) fprintf(stderr, "%s:%d: warning: ", fileName, lineNo);
    va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fprintf(stderr, "\n");
    if (fileName) fprintf(stderr, "%s:%d: %s\n", fileName, lineNo, line);
  }
}

void error(char *fmt, ...)
{
  va_list ap;
  if (fileName) fprintf(stderr, "%s:%d: error: ", fileName, lineNo);
  va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
  fprintf(stderr, "\n");
  if (fileName) fprintf(stderr, "%s:%d: %s\n", fileName, lineNo, line);
  exit(1);
}

void usage(char *progName)
{
  fprintf(stderr,
CCGVERSION" Copyright (C) 1999, 2000 Ian Piumarta <ian.piumarta@inria.fr>\n"
"Usage: %s [<option>* fileName]+\n"
"where <option> is one of:\n"
"  -c <char>        change the comment delimiter from the default `%c'\n"
"  -e <char>        change the escape character from the default `%c'\n"
"  -n               disable opcode/operand validation\n"
"  -o <fileName>    start a new output file (`-' means stdout)\n"
"  -q               quiet (suppress forward reference warning messages)\n"
"Multiple input files are concatenated onto the current output file unless\n"
"redirected with -o.  The initial output file is stdout.\n",
	  progName, commentChar, escapeChar);
  exit(1);
}

int inAsm= 0;

char lb[256];
char op[256];
char ox[256], *oxp;
char a1[256];
char a2[256];
char a3[256];
char a4[256];
char a5[256];	/* PowerPC requires up to 5 operands */

/*
 *	rewriting -- support
 */

void check_insn(char *insn)
{
  char **i;
  for (i= insn_tab; *i != (char *)0; ++i)
    if (!strcmp(*i, insn)) return;
  error("unrecognised instruction: %s", insn);
}

void upcase(char *ptr, int n)
{
  while (n--) {
    *ptr= toupper(*ptr);
    ++ptr;
  }
}

char *insert(char *dest,  char *src)
{
  int destLen= strlen(dest);
  int srcLen= strlen(src);
  memmove(dest + srcLen, dest, destLen + 1);
  memcpy(dest, src, srcLen);
  return dest + srcLen;
}

void delete(char *dest, int n)
{
  memcpy(dest, dest + n, strlen(dest) - n + 1);
}

char *skipParens(char *arg)
{
  int nest= 0;
  for (;;) {
    int c= *arg++;
    if (c == 0) {
      if (nest > 0) error("mismatched parentheses");
      return arg - 1;
    }
    if ((c == '(') || (c == '['))
      ++nest;
    else
      if ((c == ')') || (c == ']')) {
	if (nest == 0)
	  return arg - 1;
	else
	  if (--nest == 0)
	    return arg;
      }
  }
  return arg;
}

char *skipDigits(char *arg)
{
  while (isdigit(*arg)) ++arg;
  return arg;
}

int is_alnum(char c)	{ return isalnum(c) || c == '_'; }
int isopchar(char c)	{ return is_alnum(c) || c == ',' || c == '.'; }

/*
 *	rewriting -- Intel
 *
 *	OPCODE	+ i		= immediate operand
 *		+ r		= register operand
 *		+ m		= memory operand (rewrite to: disp,base,index,scale)
 */

char *rewrite_i386_reg(char *arg)
{
  if (*arg == '(') return skipParens(arg);

  /* in approximate order of likely usage */

  /* 32-bit registers */
  if      (!strncmp(arg, "eax", 3)) { delete(arg, 3); arg= insert(arg, "_EAX"); }
  else if (!strncmp(arg, "ecx", 3)) { delete(arg, 3); arg= insert(arg, "_ECX"); }
  else if (!strncmp(arg, "edx", 3)) { delete(arg, 3); arg= insert(arg, "_EDX"); }
  else if (!strncmp(arg, "ebx", 3)) { delete(arg, 3); arg= insert(arg, "_EBX"); }
  else if (!strncmp(arg, "esp", 3)) { delete(arg, 3); arg= insert(arg, "_ESP"); }
  else if (!strncmp(arg, "ebp", 3)) { delete(arg, 3); arg= insert(arg, "_EBP"); }
  else if (!strncmp(arg, "esi", 3)) { delete(arg, 3); arg= insert(arg, "_ESI"); }
  else if (!strncmp(arg, "edi", 3)) { delete(arg, 3); arg= insert(arg, "_EDI"); }
  /* 8-bit registers */
  else if (!strncmp(arg,  "al", 2)) { delete(arg, 2); arg= insert(arg, "_AL"); }
  else if (!strncmp(arg,  "cl", 2)) { delete(arg, 2); arg= insert(arg, "_CL"); }
  else if (!strncmp(arg,  "dl", 2)) { delete(arg, 2); arg= insert(arg, "_DL"); }
  else if (!strncmp(arg,  "bl", 2)) { delete(arg, 2); arg= insert(arg, "_BL"); }
  else if (!strncmp(arg,  "ah", 2)) { delete(arg, 2); arg= insert(arg, "_AH"); }
  else if (!strncmp(arg,  "ch", 2)) { delete(arg, 2); arg= insert(arg, "_CH"); }
  else if (!strncmp(arg,  "dh", 2)) { delete(arg, 2); arg= insert(arg, "_DH"); }
  else if (!strncmp(arg,  "bh", 2)) { delete(arg, 2); arg= insert(arg, "_BH"); }
  /* 16-bit registers */
  else if (!strncmp(arg,  "ax", 2)) { delete(arg, 2); arg= insert(arg, "_AX"); }
  else if (!strncmp(arg,  "cx", 2)) { delete(arg, 2); arg= insert(arg, "_CX"); }
  else if (!strncmp(arg,  "dx", 2)) { delete(arg, 2); arg= insert(arg, "_DX"); }
  else if (!strncmp(arg,  "bx", 2)) { delete(arg, 2); arg= insert(arg, "_BX"); }
  else if (!strncmp(arg,  "sp", 2)) { delete(arg, 2); arg= insert(arg, "_SP"); }
  else if (!strncmp(arg,  "bp", 2)) { delete(arg, 2); arg= insert(arg, "_BP"); }
  else if (!strncmp(arg,  "si", 2)) { delete(arg, 2); arg= insert(arg, "_SI"); }
  else if (!strncmp(arg,  "di", 2)) { delete(arg, 2); arg= insert(arg, "_DI"); }
  /* special registers would go here... */
  else
    error("unknown register name: %s", arg);
  return arg;
}

char *rewrite_i386_mem(char *arg)
{
  /* deference */
  if (arg[0] == '*') {
    if (arg[1] != '%') error("bad register in *%%reg");
    *arg++= '0';		/* disp */
    *arg++= ',';
    arg= rewrite_i386_reg(arg);	/* base */
    if (*arg != '\0') error("junk after *%%reg");
    return insert(arg, ",0,0");	/* index, scale */
  }
  /* displacement */
  if (*arg == '(') {
    if (arg[1] == '%') {
      /* (base...) */
      arg= insert(arg, "0");
    } else {
      /* (disp)... */
      arg= skipParens(arg);
    }
  } else {
    /* disp... */
    while (*arg != '\0' && *arg != '(') ++arg;
  }
  if (*arg == '\0') {
    /* absolute address */
    return insert(arg, ",0,0,0");
  }
  if (*arg != '(') error("junk after absolute address");
  delete(arg, 1); /* '(' */
  arg= insert(arg, ",");
  if (*arg != '%') {
    while (*arg != '\0' && *arg != ',' && *arg != ')') ++arg;
  } else {
    delete(arg, 1);
    arg= rewrite_i386_reg(arg);
  }
  if (*arg == '\0') error("missing close parenthesis in disp(base))");
  if (*arg == ')') {
    delete(arg, 1);
    if (*arg != '\0') error("junk after disp(base)");
    return insert(arg, ",0,0");
  }
  if (*arg != ',') error("missing comma in disp(base,index)");
  ++arg;
  if (*arg == '\0') error("missing index in disp(base,index)");
  if (*arg != '%') {
    while (*arg != '\0' && *arg != ',' && *arg != ')') ++arg;
  } else {
    delete(arg, 1);
    arg= rewrite_i386_reg(arg);
  }
  if (*arg == '\0') error("missing close parenthesis in disp(base,index)");
  if (*arg == ')') {
    delete(arg, 1);
    if (*arg != '\0') error("junk after disp(base)");
    return insert(arg, ",1");
  }
  if (*arg != ',') error("missing comma in disp(base,index,scale)");
  ++arg;
  if (*arg == '\0') error("missing scale in disp(base,index,scale)");
  if (*arg != '%') {
    while (*arg != '\0' && *arg != ',' && *arg != ')') ++arg;
  } else {
    delete(arg, 1);
    arg= rewrite_i386_reg(arg);
  }
  if (*arg != ')') error("missing close parenthesis in disp(base,index,scale)");
  delete(arg, 1);
  return arg;
}

char *rewrite_i386_arg(char *arg, int argind)
{
  if (*arg == '\0') return arg;
  /* immediate */
  if (arg[0] == '$') {
    oxp+= sprintf(oxp, "i");
    delete(arg, 1);
    return arg;
  }
  /* register */
  if (arg[0] == '%') {
    oxp+= sprintf(oxp, "r");
    delete(arg, 1);
    rewrite_i386_reg(arg);
    return arg;
  }
  /* memory */
  oxp+= sprintf(oxp, "m");
  rewrite_i386_mem(arg);
  return arg;
}

char *(i386_insns[])= {
#include "insns-i386.h"
  (char *)0
};

char *rewrite_i386(int code)
{
  if (code == 0) {
    insn_tab= i386_insns;
    return "i386";
  }
  if (code == 1) {
    if (a1[0] != '%') error("malformed register expression");
    delete(a1, 1);
    return rewrite_i386_reg(a1);
  }
  rewrite_i386_arg(a1, 1);
  rewrite_i386_arg(a2, 2);
  rewrite_i386_arg(a3, 3);
  return 0;
}

/*
 *	rewriting -- PowerPC
 *
 * <imm> = [0-9]+ | (.+)	-> i(imm)
 * <imm> = <imm>@FN		-> i(_FN(imm))	// for varName@ha, etc...
 * <reg> = r<imm>		-> r(imm)
 * <mem> = <imm>(<reg>)		-> m(imm,reg)
 * <idx> = <reg>(<reg>)		-> x(reg,reg)
 *
 */

int is_ppc_reg(char *arg)
{
  return ((arg[0] == 'r') && ((arg[1] == '(') || isdigit(arg[1])));
}

char *rewrite_ppc_imm(char *arg, int argind)
{
  int c= *arg;
  if (isdigit(c) || (c == '-' && isdigit(arg[1]))) {
    /* numeric */
    while (isdigit(*++arg));
  } else if (isalpha(c) || (c == '_')) {
    /* macro */
    while (isalnum(*arg) || (*arg == '_')) ++arg;
    if (*arg == '(') {
      if (is_ppc_reg(arg + 1)) return arg;	/* done imm part of mem */
      /* macro(arg) */
      arg= skipParens(arg);
    }
  } else if (*arg == '(') {
    /* expression */
    arg= skipParens(arg);
  } else {
    error("unrecognised operand, position %d", argind);
  }
  return arg;
}

char *rewrite_ppc_reg(char *arg, int argind)
{
  if (!is_ppc_reg(arg)) error("malformed register operand, position %d", argind);
  delete(arg, 1);
  return rewrite_ppc_imm(arg, argind);
}

char *rewrite_ppc_arg(char *arg, int argind)
{
  int immediate= 0;
  if (*arg == '\0') return arg;
  if (is_ppc_reg(arg)) {
    /* register */
    arg= rewrite_ppc_reg(arg, argind);
  } else {
    /* immediate */
    char *base= arg;
    immediate= 1;
    arg= rewrite_ppc_imm(arg, argind);
    while (*arg == '@') {
      char fnName[64];
      char *in= arg + 1, *out= fnName;
      *out++= '_';	/* prefix FN with '_'  (replaces '@' from arg) */
      while (isalpha(*in) || *in == '_')
	*out++= toupper(*in++);
      *out= '\0';
      {
	int len= out - fnName;
	if (len < 2)	/* only got the _ */
	  error("malformed immediate modifier, position %d", argind);
	delete(arg, len);	/* drop '@' + FN */
	insert(base, "(");
	arg+= 1;
	insert(base, fnName);
	arg+= len;
	arg= insert(arg, ")");
      }
    }
  }
  if (*arg != '(') {
    /* non-memory */
    if (*arg != '\0') {
      error("junk after %s operand, position %d",
	    (immediate ? "immediate" : "register"), argind);
    }
    if (immediate) {
      oxp+= sprintf(oxp, "i");
      return arg;
    }
    oxp+= sprintf(oxp, "r");
    return arg;
  }
  /* memory */
  *arg++= ',';
  arg= rewrite_ppc_reg(arg, argind);
  if (*arg != ')') error("junk after index register, position %d", argind);
  delete(arg, 1);
  if (*arg != '\0') error("junk after memory operand, position %d", argind);
  if (immediate) {
    oxp+= sprintf(oxp, "m");
  } else {
    oxp+= sprintf(oxp, "x");
  }
  return arg;
}

char *(ppc_insns[])= {
#include "insns-ppc.h"
  (char *)0
};

char *rewrite_ppc(int code)
{
  if (code == 0) {
    insn_tab= ppc_insns;
    return "ppc";
  }
  if (code == 1) {
    return rewrite_ppc_reg(a1, 1);
  }
  rewrite_ppc_arg(a1, 1);
  rewrite_ppc_arg(a2, 2);
  rewrite_ppc_arg(a3, 3);
  rewrite_ppc_arg(a4, 4);
  rewrite_ppc_arg(a5, 5);
  return 0;
}

/*
 *	rewriting -- Sparc
 *
 * %reg		-> r(R)
 * imm		-> i(I)
 * [%reg+%reg]	-> x(R,R)
 * [%reg+/-imm]	-> m(R,I)
 *
 */

int is_sparc_r(char c)
{
  return c == 'r' || c == 'g' || c == 'o' || c == 'l' || c == 'i';
}

char *rewrite_sparc_reg(char *arg)
{
  if (!strncmp(arg, "fp",2)) {	/* %i6 == %30 */
    delete(arg, 2);
    return insert(arg, "30");
  }
  if (!strncmp(arg, "sp",2)) {	/* %o6 == %14 */
    delete(arg, 2);
    return insert(arg, "14");
  }
  if (is_sparc_r(arg[0]) && (((arg[1] >= '0') && (arg[1] <= '7')) || (arg[1] == '('))) {
    switch (*arg) {
    case 'r':	delete(arg, 1); arg= insert(arg,  "0+"); break;
    case 'g':	delete(arg, 1); arg= insert(arg,  "0+"); break;
    case 'o':	delete(arg, 1); arg= insert(arg,  "8+"); break;
    case 'l':	delete(arg, 1); arg= insert(arg, "16+"); break;
    case 'i':	delete(arg, 1); arg= insert(arg, "24+"); break;
    default:
      error("THIS CANNOT HAPPEN");
    }
  }
  if (*arg == '(') return skipParens(arg);
  while (isdigit(*arg)) ++arg;
  return arg;
}

char *rewrite_sparc_mem(char *arg)
{
  /* need to suffix op with m (immediate index) or x (register index) */
  /* return arg -> ']' */
  if (arg[0] != '%') error("missing base register in memory operand");
  delete(arg, 1);
  arg= rewrite_sparc_reg(arg);
  if (*arg == ']') {
    /* no index: use immediate zero */
    oxp+= sprintf(oxp, "m");
    arg= insert(arg, ",0");
    return arg;
  }
  if (*arg == '+' && arg[1] == '%') {
    /* [%reg+%reg] */
    oxp+= sprintf(oxp, "x");
    *arg++= ',';
    delete(arg, 1);	/* % */
    return rewrite_sparc_reg(arg);
  }
  if (*arg == '+' || *arg == '-') {
    /* [%reg+/-offset] */
    oxp+= sprintf(oxp, "m");
    arg= insert(arg, ",");
    return skipParens(arg);
  }
  return arg;
}

char *rewrite_sparc_arg(char *arg)
{
  if (*arg == '\0') return arg;
  /* pseudo-ops %hi() and %lo() */
  if (!strncmp(arg, "%hi(", 4)) {
    *arg++= '_';
    *arg++= 'H';
    *arg++= 'I';
    goto immediate;
  }
  if (!strncmp(arg, "%lo(", 4)) {
    *arg++= '_';
    *arg++= 'L';
    *arg++= 'O';
    goto immediate;
  }
  /* register */
  if (*arg == '%') {
    oxp+= sprintf(oxp, "r");
    delete(arg, 1);
    arg= rewrite_sparc_reg(arg);
    if (*arg != '\0') error("junk after register operand");
    return arg;
  }
  /* memory */
  if (*arg == '[') {
    delete(arg, 1);
    arg= rewrite_sparc_mem(arg);
    if (*arg != ']') error("malformed memory operand");
    delete(arg, 1);
    if (*arg != '\0')  error("junk after memory operand");
    return arg;
  }
  /* immediate */
 immediate:
  oxp+= sprintf(oxp, "i");
  while (*arg != '\0') ++arg;
  return arg;
}

char *(sparc_insns[])= {
#include "insns-sparc.h"
  (char *)0
};

char *rewrite_sparc(int code)
{
  if (code == 0) {
    insn_tab= sparc_insns;
    return "sparc";
  }
  if (code == 1) {
    if (a1[0] != '%') error("malformed register expression");
    delete(a1, 1);
    return rewrite_sparc_reg(a1);
  }
  rewrite_sparc_arg(a1);
  rewrite_sparc_arg(a2);
  rewrite_sparc_arg(a3);
  return 0;
}

/*
 *	rewriting -- Others (as yet unimplemented)
 */

char *rewrite_null(int code)
{
  error("missing #cpu declaration");
  return 0;
}

rewriter rewrite_alpha= rewrite_null;
rewriter rewrite_hppa=  rewrite_null;
rewriter rewrite_mips=  rewrite_null;
rewriter rewrite_m68k=  rewrite_null;

rewriter rewrite= rewrite_null;

#if defined(PPC) || defined(_POWER) || defined(_IBMR2)
# define localRewrite rewrite_ppc
#elif defined(__sparc__)
# define localRewrite rewrite_sparc
#elif defined(__alpha__)
# define localRewrite rewrite_alpha
#elif defined(__hppa__)
# define localRewrite rewrite_hppa
#elif defined(__mips__)
# define localRewrite rewrite_mips
#elif defined(__mc68000__)
# define localRewrite rewrite_m68k
#elif defined(__i386__)
# define localRewrite rewrite_i386
#else
# define localRewrite rewrite_null
#endif


/*
 *	assembler statement parsing
 */

char *eatSpaces(char *in)
{
  while (isspace(*in)) ++in;
  if (*in == commentChar) {
    if (!isspace(in[1]))
      warning("POSSIBLE UNINTENTIONAL COMMENT"); /* sometimes serious if not caught */
    *in= '\0';
  }
  return in;
}

char *eatOpcode(char *in, char *opp)
{
  int c;
  in= eatSpaces(in);
  while (isopchar(c= *in++)) {
    if (c == ',') c= '_';	/* sparc */
    if (c == '.') c= '_';	/* ppc and pseudo */
    *opp++= c;
  }
  --in;
  upcase(op, opp - op);
  *opp= '\0';
  return eatSpaces(in);
}

char *eatArg(char *in, char *out)
{
  int nest= 0;
  int inString= 0;
  in= eatSpaces(in);
  for (;;) {
    char c= *in++;
    if (c == 0 || c == '\n') { --in; break; }
    else if (c == commentChar && nest == 0) { --in; break; }
    else if (c <= ' ' && !inString) continue;
    else if (c == '\"') inString= !inString;
    else if (c == '(' || c == '[') ++nest;
    else if (c == ')' || c == ']') --nest;
    else if (c == ',' && nest == 0) break;
    if (c == '.' && dodot)
      out+= sprintf(out, "asm_pc");
    else
      *out++= c;
  }
  if (nest != 0) error("mismatched parentheses");
  *out= '\0';
  return eatSpaces(in);
}

char *assemble(char *line)
{
  static char buf[256];
  static char arg[256];
  static char tmp[256];
  char *out= buf;
  int width= 0;
  int continued= 0;

  if (strlen(line) == 0) return line;
  line= eatSpaces(line);
  if (*line == escapeChar) {
    return line + 1;
  }
  if (line[strlen(line) - 1] == '\\') {
    continued= 1;
    line[strlen(line) - 1]= '\0';
  }
  line= eatSpaces(line);
  *out= '\0';
  if (line[0] == '\0') return (continued ? "\\" : "");
  while (2 == sscanf(line, "%["LABEL_CHARS"]%[:]%n", arg, tmp, &width)) {
    out+= sprintf(out, "  "ASM_DEF"(%s);", arg);
    line= eatSpaces(line + width);
  }
  /* pseudo-ops: .org and .label are intrinsic, all others are platdep */
  if ((!strncmp(line, ".org", 4)) && isspace(line[4])) {
    line= eatArg(line + 5, a1);
    if (*line != '\0') error("junk after origin expression");
    out+= sprintf(out, "  "ASM_ORG"(%s);", a1);
    if (continued) out+= sprintf(out, "\\");
    return buf;
  } else
  if ((!strncmp(line, ".label", 6)) && isspace(line[6])) {
    line= eatSpaces(line + 7);
    if (*line == '\0') error("missing label list");
    while ((*arg= '\0'),
	   sscanf(line, "%["LABEL_CHARS"]%n", arg, &width),
	   (*arg != '\0')) {
      out+= sprintf(out, "  "ASM_LBL"(%s);", arg);
      line= eatSpaces(line + width);
      if (*line == '\0') {
	if (continued) out+= sprintf(out, "\\");
	return buf;
      }
      if (*line != ',') error("junk in label list");
      line= eatSpaces(line + 1);
    }
    error("illegal label name");
  }
  /* opcode [operands...] */
  line= eatOpcode(line, op);
  line= eatArg(line, a1);
  line= eatArg(line, a2);
  line= eatArg(line, a3);
  line= eatArg(line, a4);
  line= eatArg(line, a5);
  if (op[0]) {
    *(oxp= ox)= '\0';
#ifdef REWRITE_PSEUDO_ARGS
    rewrite(2);
#else
    /* don't rewrite pseudo-ops */
    if (op[0] == '_') {
      if (a1[0]) *oxp++= 'o';
      if (a2[0]) *oxp++= 'o';
      if (a3[0]) *oxp++= 'o';
      if (a4[0]) *oxp++= 'o';
      if (a5[0]) *oxp++= 'o';
    } else {
      rewrite(2);
    }
#endif
    strcat(op, ox);
    if (check_insns && op[0] != '_') check_insn(op);
    out+= sprintf(out, "\t%s", op);
    out+= sprintf(out, "\t(");
    if (a1[0]) out+= sprintf(out,   "%s", a1);
    if (a2[0]) out+= sprintf(out, ", %s", a2);
    if (a3[0]) out+= sprintf(out, ", %s", a3);
    if (a4[0]) out+= sprintf(out, ", %s", a4);
    if (a5[0]) out+= sprintf(out, ", %s", a5);
    out+= sprintf(out, "); ");
  }
  if (continued) out+= sprintf(out, "\\");
  return buf;
}

/*
 *	shell
 */

void selectCPU(char *arch, rewriter rw)
{
  if (rw == rewrite_null) error("%s is not supported", arch);
  if (rewrite != rewrite_null && rewrite != rw)
    error("conflicting #cpu declarations");
  if (rw != localRewrite)
    warning("foreign cpu type selected");
  rewrite= rw;
}

void chooseCPU(char *arg)
{
  if
    (!strcasecmp(arg, "default"))	selectCPU("default", localRewrite);
  else if
    (!strcasecmp(arg, "ppc") ||
     !strcasecmp(arg, "power") ||
     !strcasecmp(arg, "powerpc") ||
     !strcasecmp(arg, "rs6000"))	selectCPU("PowerPC", rewrite_ppc);
  else if
    (!strcasecmp(arg, "sparc"))		selectCPU("Sparc",   rewrite_sparc);
  else if
    (!strcasecmp(arg, "alpha") ||
     !strcasecmp(arg, "axp"))		selectCPU("Alpha",   rewrite_alpha);
  else if
    (!strcasecmp(arg, "hppa"))		selectCPU("HP-PA",   rewrite_hppa);
  else if
    (!strcasecmp(arg, "mips"))		selectCPU("MIPS",    rewrite_mips);
  else if
    (!strcasecmp(arg, "68k") ||
     !strcasecmp(arg, "m68k") ||
     !strcasecmp(arg, "mc68k") ||
     !strcasecmp(arg, "68000") ||
     !strcasecmp(arg, "m68000") ||
     !strcasecmp(arg, "mc68000"))	selectCPU("M68K",    rewrite_m68k);
  else if
    (!strcasecmp(arg, "i386") || !strcasecmp(arg, "386") ||
     !strcasecmp(arg, "i486") || !strcasecmp(arg, "486") ||
     !strcasecmp(arg, "i586") || !strcasecmp(arg, "586") ||
     !strcasecmp(arg, "pentium") ||
     !strcasecmp(arg, "intel") ||
     !strcasecmp(arg, "ia32"))		selectCPU("i386",    rewrite_i386);
  else
    error("unknown architecture");
}

void process(char *line, FILE *out)
{
  char arg[256];
  if (*line == '\0') return;
  if (*line == '\n') {
    fprintf(out, line);
    return;
  }
  line[strlen(line) - 1]= '\0';	/* zap newline */
  if (!inAsm && (line[0] == '#')) {
    if (!strncmp(line, "#cpu", 4)) {
      if (sscanf(line, "#cpu %s", arg) != 1)
	error("missing architecture name");
      chooseCPU(arg);
      fprintf(out, "#include \""HEADER_PREFIX"%s.h\" /* #cpu %s */\n",
	      rewrite(0), arg);
      return;
    }
    if (!strncmp(line, "#quiet", 6)) {
      if (inAsm) quiet= 2; else quiet= 1;
      fprintf(out, "/* #quiet */\n");
      return;
    }
    if (!strncmp(line, "#nodot", 6)) {
      dodot= 0;
      fprintf(out, "/* #nodot */\n");
      return;
    }
    if (!strncmp(line, "#localpc", 8)) {
      fprintf(out, "#define _ASM_LOCALPC /* #localpc */\n");
      return;
    }
    if (sscanf(line, "#comment %s", arg)) {
      commentChar= *arg;
      fprintf(out, "/* #comment %c */\n", commentChar);
      return;
    }
    if (sscanf(line, "#escape %s", arg)) {
      escapeChar= *arg;
      fprintf(out, "/* #escape %c */\n", escapeChar);
      return;
    }
  }
  for (;;) {
    if (!inAsm) { 
      char *delim= 0, save= 0;
      if ((delim= strstr(line, REG_OPEN)) != 0) {
	char *delim2= 0;
	save= *delim;
	*delim= '\0';
	fprintf(out, "%s", line);
	*delim= save;
	line= delim + strlen(REG_OPEN);
	if ((delim2= strstr(line, REG_CLOSE)) == 0)
	  error("missing closing delimiter in register expression");
	save= *delim2;
	*delim2= '\0';
	line= eatArg(line, a1);
	*delim2= save;
	oxp= op;
	rewrite(1);
	fprintf(out, "(%s)", a1);
	if (line != strstr(line, REG_CLOSE)) error("bad register expression");
	line+= strlen(REG_CLOSE);
	continue;
      }
      if ((delim= strstr(line, ASM_1_OPEN))  != 0) {
	inAsm= 1;
      } else {
	if ((delim= strstr(line, ASM_2_OPEN)) != 0) {
	  inAsm= 2;
	} else {
	  fprintf(out, "%s\n", line);
	  return;
	}
      }
      save= *delim;
      *delim= '\0';
      fprintf(out, "%s  %s ", line,
	      ((inAsm == 2) ? ASM_2_BEGIN : ASM_1_BEGIN));
      asmFwdWarned= 0;
      line= delim + 2;
      *delim= save;
    }
    /* assembling */
    {
      char *delim= 0, save= 0;
      if ((delim= strstr(line, ASM_1_CLOSE)) != 0) {
	if (inAsm != 1) error("mismatched assembler delimiters");
      } else {
	if ((delim= strstr(line, ASM_2_CLOSE)) != 0) {
	  if (inAsm != 2) error("mismatched assembler delimiters");
	} else {
	  fprintf(out, "%s\n", assemble(line));
	  return;
	}
      }
      save= *delim;
      *delim= '\0';
      fprintf(out, "  %s%s", assemble(line),
	      ((inAsm == 1) ? ASM_1_END : ASM_2_END));
      line= delim+2;
      *delim= save;
      inAsm= 0;
    }
  }
}

void newOutput(FILE *out)
{
#if 0
  fprintf(out,
"/****************************************************************************\n"
"          THIS FILE WAS GENERATED AUTOMATICALLY -- DO NOT EDIT IT!\n"
"****************************************************************************/\n");
#endif
}

/*
 *	driver
 */

int main(int argc, char **argv)
{
  char *progName= *argv++;
  FILE *in= 0, *out= 0;

  {
    char *ptr= progName;
    do
      if (*ptr++ == '/') progName= ptr;
    while (*ptr);
  }

  if (argc < 2) usage(progName);

  while (--argc) {
    lineNo= 0;
    fileName= 0;

    if (!strcmp(*argv, "-o")) {
      char *outFile;
      if (out) fclose(out);
      if (!argc--) error("file name missing after \"-o\"");
      outFile= *++argv;
      if (!strcmp(outFile, "-"))
	out= 0;
      else {
	out= fopen(outFile, "w");
	if (!out) error("can't open output file: %s", outFile);
	newOutput(out);
      }
      ++argv;
      continue;
    }

    if (!strcmp(*argv, "-c")) {
      if (!argc--) error("character missing after \"-c\"");
      commentChar= **++argv;
      ++argv;
      continue;
    }

    if (!strcmp(*argv, "-e")) {
      if (!argc--) error("character missing after \"-v\"");
      escapeChar= **++argv;
      ++argv;
      continue;
    }

    if (!strcmp(*argv, "-q")) {
      quiet= 1;
      ++argv;
      continue;
    }

    if (!strcmp(*argv, "-n")) {
      check_insns= 0;
      ++argv;
      continue;
    }

    if (**argv == '-') usage(progName);

    fileName= *argv++;
    in= fopen(fileName, "r");
    if (in == 0) {
      fprintf(stderr, "%s: file not found: %s\n", progName, fileName);
      exit(1);
    }
    if (out == 0) {
      out= stdout;
      newOutput(out);
    }
#if 0
    fprintf(out, "# \"%s\" %d\n", fileName, 1);
#endif
    while (!feof(in)) {
      if (0 == fgets(line, sizeof(line) - 1, in)) break;
      ++lineNo;
      process(line, out);
    }
    fclose(in);
  }

  return 0;
}
