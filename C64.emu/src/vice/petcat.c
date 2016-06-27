/*
 * This file is part of Commodore 64 emulator
 *      and Program Development System.
 *
 *   Copyright (C) 1993-1995,1996, Jouko Valta
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * This program converts your SEQ files as well as expands tokenized
 * C64/128 BASIC programs into 7-bit ASCII text. Unprintable characters
 * can be shown as hexadecimal codes in parenthesis, via `-c' option.
 * It is also possible to convert programs from ascii listings into
 * tokenized basic v2.0, v3.5, v4.0, v7.0 or simon's basic programs. This
 * program also replaces certain control code names with the actual codes.
 *
 * A list of Toolkit Basic (published by Compute! Books) token codes
 * would be greatly appreciated. (-:
 *
 * Runs on UNIX or Atari ST.
 *
 * In shell:
 *  tr '\015A-Za-z\301-\332\\\|\[\{\]\}' '\012a-zA-ZA-Z\|\\\{\[\}\]'
 * or
 *  tr '\015A-Za-z\\\|\[\{\]\}' '\012a-zA-Z\|\\\{\[\}\]'
 *
 * Written by
 *   Jouko Valta <jopi@stekt.oulu.fi>
 *
 * With additional changes by
 *   Ettore Perazzoli <ettore@comm2000.it>
 *   Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
 *
 * Support for Final Cartridge III extensions to c64 2.0 basic
 *   Matti 'ccr' Hamalainen <ccr@tnsp.org>
 *
 * Support for many of the other extensions by
 *   Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Various fixes and enhancements by
 *   groepaz <groepaz@gmx.net>
 *   Ian Coog
 *
 */

/* #define DEBUG */

#include "vice.h"

#include "version.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "charset.h"            /* ctrl1, ctrl2, cbmkeys */
#include "cmdline.h"
#include "lib.h"
#include "network.h"
#include "types.h"
#include "util.h"
#include "vice-event.h"

#ifdef DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

#define PETCATVERSION   2.21
#define PETCATLEVEL     1

#define B_1              1
#define B_2              2
#define B_SUPEREXP       3
#define B_TURTLE         4

#define B_SIMON          5
#define B_SPEECH         6
#define B_ATBAS          7
#define B_4              8
#define B_4E             9      /* C64 extension, Expand only */

#define B_35            10
#define B_7             11
#define B_10            12
#define B_FC3           13
#define B_ULTRA         14
#define B_GRAPH         15
#define B_WS            16
#define B_MIGHTY        17
#define B_PEG           18
#define B_X             19
#define B_DRAGO         20
#define B_REU           21
#define B_BASL          22
#define B_71            23
#define B_MAGIC         24
#define B_EASY          25
#define B_BLARG         26
#define B_VIC4          27
#define B_VIC5          28
#define B_WSF           29
#define B_GB            30
#define B_BSX           31
#define B_SUPERBAS      32
#define B_EXPBAS64      33
#define B_SXC           34
#define B_WARSAW        35
#define B_EXPBAS20      36
#define B_SUPERGRA      37
#define B_KIPPER        38
#define B_BOB           39
#define B_EVE           40
#define B_TT64          41

/* Super Expander (VIC20) -- Tokens 0xCC - 0xDD */

const char *superexpkwcc[] = { 
    "key",   "graphic", "scnclr", "circle", "draw", "region", "color", "point",
    "sound", "char",    "paint",  "rpot",   "rpen", "rsnd",   "rcolr", "rgr",
    "rjoy",  "rdot"
};

/* Turtle Basic v1.0 (VIC20) by Craig Bruce -- Tokens 0xCC - 0xED */

const char *turtlekwcc[] = {
    "graphic", "old",    "turn",   "pen",    "draw",  "move", "point", "kill",
    "write",   "repeat", "screen", "doke",   "reloc", "fill", "rtime", "base",
    "pause",   "pop",    "color",  "merge",  "char",  "take", "sound", "vol",
    "put",     "place",  "cls",    "accept", "reset", "grab", "rdot",  "plr$",
    "deek",    "joy"
};

/* Simon's Basic (C64) -- Tokens 0x6400 - 0x647F */

const char *simonskw[] = {
    "",         "hires",    "plot",     "line",    "block",    "fchr",
    "fcol",     "fill",     "rec",      "rot",     "draw",     "char",
    "hi col",   "inv",      "frac",     "move",    "place",    "upb",
    "upw",      "leftw",    "leftb",    "downb",   "downw",    "rightb",
    "rightw",   "multi",    "colour",   "mmob",    "bflash",   "mob set",
    "music",    "flash",    "repeat",   "play",    ">>",       "centre",
    "envelope", "cgoto",    "wave",     "fetch",   "at(",      "until",
    ">>",       ">>",       "use",      ">>",      "global",   ">>",
    "reset",    "proc",     "call",     "exec",    "end proc", "exit",
    "end loop", "on key",   "disable",  "resume",  "loop",     "delay",
    ">>",       ">>",       ">>",       ">>",      "secure",   "disapa",
    "circle",   "on error", "no error", "local",   "rcomp",    "else",
    "retrace",  "trace",    "dir",      "page",    "dump",     "find",
    "option",   "auto",     "old",      "joy",     "mod",      "div",
    ">>",       "dup",      "inkey",    "inst",    "test",     "lin",
    "exor",     "insert",   "pot",      "penx",    ">>",       "peny",
    "sound",    "graphics", "design",   "rlocmob", "cmob",     "bckgnds",
    "pause",    "nrm",      "mob off",  "off",     "angl",     "arc",
    "cold",     "scrsv",    "scrld",    "text",    "cset",     "vol",
    "disk",     "hrdcpy",   "key",      "paint",   "low col",  "copy",
    "merge",    "renumber", "mem",      "detect",  "check",    "display",
    "err",      "out"
};

/* Speech Basic v2.7 (C64) - Tokens 0xCC - 0xE6 */

const char *speechkwcc[] = {
    "reset",  "basic",  "help", "key",   "himem",  "disk", "dir",    "bload",
    "bsave",  "map",    "mem",  "pause", "block",  "hear", "record", "play",
    "voldef", "coldef", "hex",  "dez",   "screen", "exec", "mon",    "<-",
    "from",   "speed",  "off"
};

/* @Basic (C64) by Andre Fachat -- Tokens 0xCC - 0xF6 */

const char *atbasickwcc[] = {
    "trace",    "delete",  "auto",   "old",     "dump",      "find",
    "renumber", "dload",   "dsave",  "dverify", "directory", "catalog",
    "scratch",  "collect", "rename", "copy",    "backup",    "disk",
    "header",   "append",  "merge",  "mload",   "mverify",   "msave",
    "key",      "basic",   "reset",  "exit",    "enter",     "doke",
    "set",      "help",    "screen", "lomem",   "himem",     "colour",
    "type",     "time",    "deek",   "hex$",    "bin$",      "off",
    "alarm"
};

/* Basic v4.0 (PET) -- TOKENS 0xCC - 0xDA / Basic v4.0 extension (C64) -- Tokens 0xCC - 0xE3 */
const char *petkwcc[] = {
    "concat",    "dopen",  "dclose", "record", "header",  "collect", "backup",
    "copy",      "append", "dsave",  "dload",  "catalog", "rename",  "scratch",
    "directory",

    /* Basic 4 Extension for C64 (0xdb - 0xe3) */
    "color",  "cold", "key", "dverify", "delete", "auto", "merge", "old",
    "monitor"
};

/* Final Cartridge III (C64) by Matti 'ccr' Hamalainen -- Tokens 0xCC - 0xE8 */

const char *fc3kw[] = {
    "off",   "auto",    "del",     "renum",  "?ERROR?", "find",   "old",
    "dload", "dverify", "dsave",   "append", "dappend", "dos",    "kill",
    "mon",   "pdir",    "plist",   "bar",    "desktop", "dump",   "array",
    "mem",   "trace",   "replace", "order",  "pack",    "unpack", "mread",
    "mwrite"
};

/* Ultrabasic-64 (C64) by Marco van den Heuvel - Tokens 0xCC - 0xFE */

const char *ultrabasic64kwcc[] = {
    "dot",    "draw",  "box",    "tic",    "copy", "sprite", "off",   "mode",
    "norm",   "graph", "dump",   "gread",  "char", "place",  "multi", "hires",
    "hex",    "bit",   "colors", "pixel",  "fill", "circle", "block", "sdata",
    "vol",    "gen",   "scoll",  "bcoll",  "joy",  "paddle", "pen",   "sound",
    "tune",   "tdata", "set",    "turnto", "turn", "tup",    "tdown", "tcolor",
    "turtle", "move",  "bye",    "rotate", "tpos", "ctr",    "sctr",  "[",
    "]",      "hard",  "exit"
};

/* Graphics basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xFE */

const char *graphicsbasickwcc[] = {
    "background", "border", "dir",   "disk",   "fill",      "key",     "circle",
    "procedure",  "dot",    "find",  "change", "ren",       "else",    "copy",
    "scroll",     "roll",   "box",   "scale",  "do",        "line",    "sprite",
    "color",      "hires",  "clear", "text",   "window",    "off",     "at",
    "shape",      "xysize", "speed", "from",   "setorigin", "animate", "multi",
    "eze",        "move",   "under", "edit",   "reset",     "xpos",    "gprint",
    "voice",      "adsr",   "wave",  "ne",     "volume",    "play",    "ypos",
    "sound",      "joy"
};

/* WS (WohnzimmerSoft) basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xFE */

const char *wsbasickwcc[] = {
    "copy",   "old",    "port",  "doke",  "vpoke",  "fill",   "error",
    "send",   "call",   "bit",   "dir",   "bload",  "bsave",  "find",
    "speed",  "pitch",  "say",   "fast",  "slow",   "talk",   "shutup",
    "stash",  "fetch",  "swap",  "off",   "screen", "device", "object",
    "vstash", "vfetch", "quiet", "color", "cls",    "curpos", "monitor",
    "subend", "do",     "loop",  "exit",  "deek",   "rsc",    "rsm",
    "dec",    "hex$",   "hi",   "lo",    "ds$",    "line",   "vpeek",
    "row",    "joy"
};

/* Mighty Basic (VIC20) -- Tokens 0xCC - 0xFE */

const char *mightykwcc[] = {
    "delete",  "old",     "renumber",  "help",   "header", "move",
    "trace",   "kill",    "dump",      "dsave",  "dload",  "dverify",
    "dresave", "scratch", "directory", "key",    "send",   "pop",
    "off",     "bsave",   "bload",     "find",   "auto",   "pprint",
    "accept",  "reset",   "else",      "color",  "take",   "pause",
    "base",    "copychr", "char",      "beep",   "cls",    "fill",
    "merge",   "sound",   "give",      "plist",  "put",    "volume",
    "rtime",   "msb",     "lsb",       "vector", "joy",    "dec",
    "hex$",    "grab",    "ds$"
};

/* Pegasus basic v4.0 (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xEC */

const char *pegbasickwcc[] = {
    "off",      "asc(",     "sin(",    "cos(",  "tan(",     "atn(",
    "deg(",     "rad(",     "frac(",   "mod(",  "round(",   "dec(",
    "bin(",     "deek(",    "instr(",  "joy(",  "pot(",     "screen(",
    "test(",    "using",    "ds$",     "hex$(", "bin$(",    "space$(",
    "ucase$(",  "string$(", "input$(", "time$", "spritex(", "spritey(",
    "turtlex(", "turtley(", "turtleang"
};

/* Xbasic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xEC */

const char *xbasickwcc[] = {
    "sprat",   "brdr",    "screen", "quit",     "sprmult", "move",  "sprite",
    "asprite", "dsprite", "sid",    "envelope", "gate",    "frq",   "wave",
    "vol",     "fcut",    "fmode",  "filter",   "frsn",    "cset",  "multi",
    "extnd",   "locate",  "center", "hires",    "line",    "hprnt", "plot",
    "text",    "clear",   "colr",   "stick",    "btn"
};

/* Drago basic v2.2 (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xD8 */

const char *dragobasickwcc[] = {
    "punkt",   "linia",  "rysuj", "param",  "kuntur", "anim", "kolor", "puwid",
    "ryselip", "koguma", "fiut",  "figura", "figuma"
};

/* REU-basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xDA */

const char *reubasickwcc[] = {
    "push", "pull", "flip", "rec", "stash", "fetch", "swap", "reu", "size",
    "dir",  "@",    "kill", "rom", "ram",   "move"
};

/* Basic Lightning (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xFE */

const char *baslkwcc[] = {
    "else",  "hex$",  "deek",     "true",    "import",  "cfn",   "size",
    "false", "ver$",  "lpx",      "lpy",     "common%", "crow",  "ccol",
    "atr",   "inc",   "num",      "row2",    "col2",    "spn2",  "hgt",
    "wid",   "row",   "col",      "spn",     "task",    "halt",  "repeat",
    "until", "while", "wend",     "cif",     "celse",   "cend",  "label",
    "doke",  "exit",  "allocate", "disable", "pull",    "dload", "dsave",
    "var",   "local", "procend",  "proc",    "casend",  "of",    "case",
    "rpt",   "setatr"
};

/* Magic Basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xFD */

const char *magickwcc[] = {
    "assembler", "auto",   "cdrive", "cat",     "dappend", "delete",  "dez",
    "dir",       "dload",  "dsave",  "dverify", "config",  "find",    " ",
    " ",         "help",   "hex",    "jump",    "llist",   "lprint",  "off",
    "old",       "renum",  "crun",   "send",    "status",  "hires",   "multi",
    "clear",     "plot",   "invert", "line",    "text",    "graphik", "page",
    "box",       "draw",   "mix",    "copy",    "circle",  "gsave",   "gload",
    "frame",     "hprint", "vprint", "block",   "fill",    " ",       "replace",
    "lrun"
};

/* Easy Basic (VIC20) -- Tokens 0xCC - 0xFE */

const char *easykwcc[] = {
    "delete", "old",     "renumber",  "dump",   "merge", "plot",
    "trace",  "kill",    "help",      "dload",  "dsave", "dverify",
    "append", "screen",  "directory", "key",    "send",  "pop",
    "off",    "pout",    "header",    "find",   "auto",  "pprint",
    "accept", "reset",   "scratch",   "color",  "take",  "pause",
    "base",   "copychr", "char",      "clk",    "cls",   "fill",
    "retime", "sound",   "poff",      "plist",  "put",   "volume",
    "joy",    "msb",     "lsb",       "vector", "rkey",  "dec",
    "hex$",   "grab",    "ds$"
};

/* Blarg (C64) by Marco van den Heuvel -- Tokens 0xE0 - 0xEA */

const char *blargkwe0[] = {
    "plot",   "line", "circle", "gron", "groff", "mode", "origin", "clear",
    "buffer", "swap", "color"
};

/* Basic 4.0 extension (VIC20) -- Tokens 0xCC - 0xDF */

const char *vic4kwcc[] = {
    "concat",  "dopen",   "dclose",    "record", "header", "collect",
    "backup",  "copy",    "append",    "dsave",  "dload",  "catalog",
    "rename",  "scratch", "directory", "ieee",   "serial", "parallel",
    "monitor", "modem"
};

/* Basic 5.0 extension (VIC20) -- Tokens 0xCC - 0xF1 */

const char *vic5kwcc[] = {
    "concat", "dopen",    "dclose",    "record",  "header",  "collect",
    "backup", "copy",     "append",    "dsave",   "dload",   "catalog",
    "rename", "scratch",  "directory", "dverify", "monitor", "repeat",
    "bell",   "commands", "renew",     "`",       "key",     "auto",
    "off",    "",         "merge",     "color",   "mem",     "enter",
    "delete", "find",     "number",    "else",    "call",    "graphic",
    "alpha",  "dmerge"
};

/* WS (WohnzimmerSoft) Basic final (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xFE */

const char *wsfbasickwcc[] = {
    "copy",   "bank",   "old",   "doke",  "display", "fill",   "error",
    "send",   "call",   "bit",   "dir",   "bload",   "bsave",  "find",
    "speed",  "pitch",  "say",   "fast",  "slow",    "talk",   "shutup",
    "stash",  "fetch",  "swap",  "off",   "mode",    "device", "object",
    "vstash", "vfetch", "latch", "color", "cls",     "curpos", "monitor",
    "subend", "do",     "loop",  "exit",  "deek",    "col",    "rsm",
    "dec",    "hex$",   "hi",    "lo",    "ds$",     "line",   "bnk",
    "ypos",   "joy"
};

/* Game Basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xE8 */

const char *gbkwcc[] = {
    "window", "bfile",   "upper",   "lower",   "cls",    "screen", "parse",
    "proc",   "else",    "scratch", "replace", "device", "dir",    "repeat",
    "until",  "disk",    "fetch#",  "put#",    "prompt", "pop",    "help",
    "exit",   "disable", "enter",   "reset",   "warm",   "num",    "type",
    "text$"
};

/* Basex (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xEA */

const char *bsxkwcc[] = {
    "append",   "auto",   "bar",    "circle", "clg",      "cls",   "csr",
    "delete",   "disk",   "draw",   "edge",   "envelope", "fill",  "key",
    "mob",      "mode",   "move",   "old",    "pic",      "dump",  "plot",
    "renumber", "repeat", "scroll", "sound",  "while",    "until", "voice",
    "ass",      "dis",    "mem"
};

/* Super Basic (C64) by Marco van den Heuvel -- Tokens 0xDB - 0xFE */

const char *superbaskwdb[] = {
    "volume",   "reset",     "mem",    "trace",   "basic",   "resume", "letter",
    "help",     "coke",      "ground", "matrix",  "dispose", "print@", "himem",
    "hardcopy", "inputform", "lock",   "swap",    "using",   "sec",    "else",
    "error",    "round",     "deek",   "string$", "point",   "instr",  "ceek",
    "min",      "max",       "varptr", "frac",    "odd",     "dec",    "hex$",
    "eval"
};

/* Expanded Basic (C64) by Marco van den Heuvel - Tokens 0xCC - 0xF5 */

const char *expbas64kwcc[] = {
    "hires",    "norm",     "graph",    "set",    "line",     "circle",
    "fill",     "mode",     "cls",      "text",   "color",    "gsave",
    "gload",    "inverse",  "frame",    "move",   "using",    "renumber",
    "delete",   "box",      "mobdef",   "sprite", "mobset",   "modsize",
    "mobcolor", "mobmulti", "mobmove",  "doke",   "allclose", "old",
    "auto",     "volume",   "envelope", "wave",   "play",     "case error",
    "resume",   "no error", "find",     "inkey",  "merge",    "hardcopy"
};

/* Super Expander Chip (C64) -- Tokens 0xFE00 - 0xFE1F */

const char *sxckwfe[] = {
    "key",    "color",  "graphic", "scnclr", "locate", "scale",  "box",
    "circle", "char",   "draw",    "gshape", "paint",  "sshape", "tune",
    "filter", "sprdef", "tempo",   "movspr", "sprcol", "sprite", "colint",
    "sprsav", "rbump",  "rclr",    "rdot",   "rgr",    "rjoy",   "rpen",
    "rpot",   "rspcol", "rsppos",  "rspr"
};

/* Warsaw Basic Keywords (C64) by Marco van den Heuvel -- Tokens 0xDB - 0xFE */

const char *warsawkwdb[] = {
    "hisave", "sline", "mem",    "trace",   "beep",    "resume", "letter",
    "help",   "*****", "ground", "revers",  "dispose", "print@", "himem",
    "*****",  "line",  "proc",   "axis",    "using",   "sec",    "else",
    "rror",   "round", "****",   "*******", "*****",   "*****",  "pound",
    "min",    "max",   "******", "frac",    "odd",     "***",    "heek",
    "eval"
};

/* Expanded Basic (VIC20) by Marco van den Heuvel -- Tokens 0xCC - 0xE3 */

const char *expbas20kwcc[] = {
    "reset", "sound", "slow(", "com",   "mem",    "stat(", "key",   "off",
    "col(",  "plot(", "pop(",  "chol(", "curol(", "beep(", "paus(", "msav",
    "reg(",  "dpek(", "pdl",   "joy",   "dpok",   "do",    "until", "old"
};

/* Data Becker Supergrafik 64 (C64) by Sven Droll -- Tokens 0xCC - 0xFD */

const char *supergrakw[] = {
    " ",      " ",       "   ",    " ",      " ",         " ",       " ",
    " ",      " ",       " ",      " ",      "directory", "spower",  "gcomb",
    "dtaset", "merge",   "renum",  "key",    "trans",     " ",       "tune",
    "sound",  "volume=", "filter", "sread",  "define",    "set",     "swait",
    "smode",  "gmode",   "gclear", "gmove",  "plot",      "draw",    "fill",
    "frame",  "invers",  "text",   "circle", "paddle",    "scale=",  "color=",
    "scol=",  "pcol=",   "gsave",  "gload",  "hcopy",     "ireturn", "if#",
    "paint"
};

/* Kipper Basic (C64) by Marco van den Heuvel -- Tokens 0xE1 - 0xF2 */

const char *kipperkwe1[] = {
    "ipcfg", "dhcp",      "ping",    "myip",     "netmask", "gateway",
    "dns",   "tftp",      "tfget",   "tfput",    "netcat",  "tcpconnect",
    "poll",  "tcplisten", "tcpsend", "tcpclose", "tcpblat", "mac"
};

/* Basic on Bails (C64) by Marco van den Heuvel -- Tokens 0xE1 - 0xF0 */

const char *bobkwe1[] = {
    "ipcfg", "dhcp",  "ping", "myip",  "netmask", "gateway", "dns",   "hook",
    "yield", "xsend", "!",    "httpd", "type",    "status",  "flush", "mac"
};

/* Eve Basic (C64) by Marco van den Heuvel -- Tokens 0xCC - 0xF9 */

const char *evekwcc[] = {
    "else",     "page",    "paper",  "ink",    "locate", "erase",    "graphic",
    "scale",    "pen",     "point",  "line",   "paint",  "write",    "draw",
    "image",    "sprite",  "sprpic", "sprcol", "sprloc", "sprmulti", "tone",
    "envelope", "wave",    "vol",    "filter", "dos",    "dvc",      "dir",
    "cat",      "record#", "swap",   "exit",   "do",     "loop",     "while",
    "until",    "cur",     "bin$",   "mak$",   "input$", "fmt$",     "infix$",
    "instr",    "ds$",     "ds",     "sd"
};

/* The Tool 64 (C64) by Marco van den Heuvel -- Tokens 0xDB - 0xF4 */

const char *tt64kwdb[] = {
    " ",       "sort",  "extract", "carget", " ",    " ",     "screen",
    "graphic", "text",  "auto",    "find",   "dump", "error", "renu",
    "delete",  "plot",  "point",   "draw",   "move", "color", "else",
    "display", "trace", "off",     "hcopy",  "joy"
};

typedef struct basic_list_s {
    BYTE version;
    BYTE num_tokens;
    BYTE max_token;
    WORD load_address;
    BYTE token_offset;
    BYTE token_start;
    const char **tokens;
    char *version_select;
    char *name;
} basic_list_t;

static basic_list_t basic_list[] = {
    { B_1,        75, 0xCB, 0x0801, 0, 0,    NULL, /* fix */    "1",         "PET Basic v1.0" },
    { B_2,        76, 0xDD, 0x0801, 0, 0,    NULL, /* fix */    "2",         "Basic v2.0" },
    { B_SUPEREXP, 18, 0xDD, 0x0401, 0, 0xCC, superexpkwcc,      "superexp",  "Basic v2.0 with Super Expander (VIC20)" },
    { B_TURTLE,   34, 0xED, 0x3701, 0, 0xCC, turtlekwcc,        "turtle",    "Basic v2.0 with Turtle Basic v1.0 (VIC20)" },
    { B_SIMON,   128, 0xCB, 0x0801, 1, 0,    simonskw,          "simon",     "Basic v2.0 with Simon's Basic (C64)" },
    { B_SPEECH,   27, 0xE6, 0x0801, 0, 0xCC, speechkwcc,        "speech",    "Basic v2.0 with Speech Basic v2.7 (C64)" },
    { B_ATBAS,    43, 0xF6, 0x0801, 0, 0xCC, atbasickwcc,       "a",         "Basic v2.0 with @Basic (C64)" },
    { B_4,        15, 0xDA, 0x0801, 0, 0xCC, petkwcc,           "40",        "Basic v4.0 (PET)" },
    { B_4E,       24, 0xE3, 0x0801, 0, 0xCC, petkwcc,           "4e",        "Basic v4.0 extension (C64)" },
    { B_35,      126, 0xCB, 0x1001, 0, 0,    NULL, /* fix */    "3",         "Basic v3.5 (C16)" },
    { B_7,        39, 0x26, 0x1c01, 2, 0,    NULL, /* fix */    "70",        "Basic v7.0 (C128)" },
    { B_10,       62, 0x3D, 0x2001, 2, 0,    NULL, /* fix */    "10",        "Basic v10.0 (C65/C64DX)" },
    { B_FC3,      29, 0xE8, 0x0801, 0, 0xCC, fc3kw,             "f",         "Basic v2.0 with Final Cartridge III (C64)" },
    { B_ULTRA,    51, 0xFE, 0x2c01, 0, 0xCC, ultrabasic64kwcc,  "ultra",     "Basic v2.0 with Ultrabasic-64 (C64)" },
    { B_GRAPH,    51, 0xFE, 0x1001, 0, 0xCC, graphicsbasickwcc, "graph",     "Basic v2.0 with Graphics basic (C64)" },
    { B_WS,       51, 0xFE, 0x0801, 0, 0xCC, wsbasickwcc,       "wsb",       "Basic v2.0 with WS basic (C64)" },
    { B_MIGHTY,   51, 0xFE, 0x3201, 0, 0xCC, mightykwcc,        "mighty",    "Basic v2.0 with Mighty Basic (VIC20)" },
    { B_PEG,      33, 0xEC, 0x0401, 0, 0xCC, pegbasickwcc,      "pegasus",   "Basic v2.0 with Pegasus basic v4.0 (C64)" },
    { B_X,        33, 0xEC, 0x0801, 0, 0xCC, xbasickwcc,        "xbasic",    "Basic v2.0 with Xbasic (C64)" },
    { B_DRAGO,    13, 0xD8, 0x0801, 0, 0xCC, dragobasickwcc,    "drago",     "Basic v2.0 with Drago basic v2.2 (C64)" },
    { B_REU,      14, 0xDA, 0x0801, 0, 0xCC, reubasickwcc,      "reu",       "Basic v2.0 with REU-basic (C64)" },
    { B_BASL,     51, 0xFE, 0x0801, 0, 0xCC, baslkwcc,          "lightning", "Basic v2.0 with Basic Lightning (C64)" },
    { B_71,       56, 0x39, 0x1c01, 2, 0,    NULL, /* fix */    "71",        "Basic v7.1 (C128)" },
    { B_MAGIC,    50, 0xFD, 0x0801, 0, 0xCC, magickwcc,         "magic",     "Basic v2.0 with Magic Basic (C64)" },
    { B_EASY,     51, 0xFE, 0x3001, 0, 0xCC, easykwcc,          "easy",      "Basic v2.0 with Easy Basic (VIC20)" },
    { B_BLARG,    11, 0xEA, 0x0801, 0, 0xE0, blargkwe0,         "blarg",     "Basic v2.0 with Blarg (C64)" },
    { B_VIC4,     20, 0xDF, 0x0801, 0, 0xCC, vic4kwcc,          "4v",        "Basic v4.0 (VIC20)" },
    { B_VIC5,     38, 0xF1, 0x0801, 0, 0xCC, vic5kwcc,          "5",         "Basic v5.0 (VIC20)" },
    { B_WSF,      51, 0xFE, 0x0801, 0, 0xCC, wsfbasickwcc,      "wsf",       "Basic v2.0 with WS basic final (C64)" },
    { B_GB,       29, 0xE8, 0x0801, 0, 0xCC, gbkwcc,            "game",      "Basic v2.0 with Game Basic (C64)" },
    { B_BSX,      31, 0xEA, 0x0401, 0, 0xCC, bsxkwcc,           "bsx",       "Basic v2.0 with Basex (C64)" },
    { B_SUPERBAS, 36, 0xFE, 0x0801, 0, 0xDB, superbaskwdb,      "superbas",  "Basic v2.0 with Super Basic (C64)" },
    { B_EXPBAS64, 42, 0xF5, 0x0801, 0, 0xCC, expbas64kwcc,      "exp64",     "Basic v2.0 with Expanded Basic (C64)" },
    { B_SXC,      32, 0x1F, 0x0801, 0, 0,    sxckwfe,           "sxc",       "Basic v2.0 with Super Expander Chip (C64)" },
    { B_WARSAW,   36, 0xFE, 0x0801, 0, 0xDB, warsawkwdb,        "warsaw",    "Basic v2.0 with Warsaw Basic (C64)" },
    { B_EXPBAS20, 24, 0xE3, 0x0801, 0, 0xCC, expbas20kwcc,      "exp20",     "Basic v2.0 with Expanded Basic (VIC20)" },
    { B_SUPERGRA, 50, 0xFE, 0x0801, 0, 0xCC, supergrakw,        "supergra",  "Basic v2.0 with Supergrafik 64 (C64)" },
    { B_KIPPER,   18, 0xF2, 0x0801, 0, 0xE1, kipperkwe1,        "k",         "Basic v2.0 with Kipper Basic (C64)" },
    { B_BOB,      16, 0xF0, 0x0801, 0, 0xE1, bobkwe1,           "bb",        "Basic v2.0 with Basic on Bails (C64)" },
    { B_EVE,      46, 0xF9, 0x0801, 0, 0xCC, evekwcc,           "eve",       "Basic v2.0 with Eve Basic (C64)" },
    { B_TT64,     26, 0xF4, 0x5b01, 0, 0xDB, tt64kwdb,          "tt64",      "Basic v2.0 with The Tool 64 (C64)" },
    { -1,         -1, -1,   -1,     0, 0,    NULL,              NULL,        NULL }
};

#define NUM_VERSIONS ((sizeof(basic_list) / sizeof(basic_list[0])) - 1)

/* Limits */

#define NUM_KWCE        11

#define MAX_KWCE        0x0A

#define KW_NONE         0xFE    /* flag unused token */

#define CODE_NONE       -1      /* flag unknown control code */

#define CLARIF_LP       '{'     /* control code left delimiter */
#define CLARIF_RP       '}'     /* control code right delimiter */

#define CLARIF_LP_ST    "{"     /* control code left delimiter, "string version" */
#define CLARIF_RP_ST    "}"     /* control code right delimiter, "string version" */

/* ------------------------------------------------------------------------- */

/* all numeric codes */
static const char *hexcodes[0x100] = {
    "$00", "$01", "$02", "$03", "$04", "$05", "$06", "$07", "$08", "$09", "$0a", "$0b", "$0c", "$0d", "$0e", "$0f",
    "$10", "$11", "$12", "$13", "$14", "$15", "$16", "$17", "$18", "$19", "$1a", "$1b", "$1c", "$1d", "$1e", "$1f",
    "$20", "$21", "$22", "$23", "$24", "$25", "$26", "$27", "$28", "$29", "$2a", "$2b", "$2c", "$2d", "$2e", "$2f",
    "$30", "$31", "$32", "$33", "$34", "$35", "$36", "$37", "$38", "$39", "$3a", "$3b", "$3c", "$3d", "$3e", "$3f",
    "$40", "$41", "$42", "$43", "$44", "$45", "$46", "$47", "$48", "$49", "$4a", "$4b", "$4c", "$4d", "$4e", "$4f",
    "$50", "$51", "$52", "$53", "$54", "$55", "$56", "$57", "$58", "$59", "$5a", "$5b", "$5c", "$5d", "$5e", "$5f",
    "$60", "$61", "$62", "$63", "$64", "$65", "$66", "$67", "$68", "$69", "$6a", "$6b", "$6c", "$6d", "$6e", "$6f",
    "$70", "$71", "$72", "$73", "$74", "$75", "$76", "$77", "$78", "$79", "$7a", "$7b", "$7c", "$7d", "$7e", "$7f",
    "$80", "$81", "$82", "$83", "$84", "$85", "$86", "$87", "$88", "$89", "$8a", "$8b", "$8c", "$8d", "$8e", "$8f",
    "$90", "$91", "$92", "$93", "$94", "$95", "$96", "$97", "$98", "$99", "$9a", "$9b", "$9c", "$9d", "$9e", "$9f",
    "$a0", "$a1", "$a2", "$a3", "$a4", "$a5", "$a6", "$a7", "$a8", "$a9", "$aa", "$ab", "$ac", "$ad", "$ae", "$af",
    "$b0", "$b1", "$b2", "$b3", "$b4", "$b5", "$b6", "$b7", "$b8", "$b9", "$ba", "$bb", "$bc", "$bd", "$be", "$bf",
    "$c0", "$c1", "$c2", "$c3", "$c4", "$c5", "$c6", "$c7", "$c8", "$c9", "$ca", "$cb", "$cc", "$cd", "$ce", "$cf",
    "$d0", "$d1", "$d2", "$d3", "$d4", "$d5", "$d6", "$d7", "$d8", "$d9", "$da", "$db", "$dc", "$dd", "$de", "$df",
    "$e0", "$e1", "$e2", "$e3", "$e4", "$e5", "$e6", "$e7", "$e8", "$e9", "$ea", "$eb", "$ec", "$ed", "$ee", "$ef",
    "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9", "$fa", "$fb", "$fc", "$fd", "$fe", "$ff",
};

/* ------------------------------------------------------------------------- */

#if 0
/* keys for charcodes 0x00 - 0x1f */
static const char *ctrlkeys[0x20] = {
    "",       "CTRL-A", "CTRL-B", "CTRL-C", "CTRL-D", "CTRL-E", "CTRL-F", "CTRL-G",
    "CTRL-H", "CTRL-I", "CTRL-J", "CTRL-K", "CTRL-L", "CTRL-M", "CTRL-N", "CTRL-O",
    "CTRL-P", "CTRL-Q", "CTRL-R", "CTRL-S", "CTRL-T", "CTRL-U", "CTRL-V", "CTRL-W",
    "CTRL-X", "CTRL-Y", "CTRL-Z", "",       "CTRL-3", "",       "CTRL-6", "CTRL-7"
};
#endif

/* keys for charcodes 0xa0-0xe0 */
static const char *cbmkeys[0x40] = {
    "SHIFT-SPACE", "CBM-K",       "CBM-I",   "CBM-T",   "CBM-@",   "CBM-G",   "CBM-+",   "CBM-M",
    "CBM-POUND",   "SHIFT-POUND", "CBM-N",   "CBM-Q",   "CBM-D",   "CBM-Z",   "CBM-S",   "CBM-P",
    "CBM-A",       "CBM-E",       "CBM-R",   "CBM-W",   "CBM-H",   "CBM-J",   "CBM-L",   "CBM-Y",
    "CBM-U",       "CBM-O",       "SHIFT-@", "CBM-F",   "CBM-C",   "CBM-X",   "CBM-V",   "CBM-B",
    "SHIFT-*",     "SHIFT-A",     "SHIFT-B", "SHIFT-C", "SHIFT-D", "SHIFT-E", "SHIFT-F", "SHIFT-G",
    "SHIFT-H",     "SHIFT-I",     "SHIFT-J", "SHIFT-K", "SHIFT-L", "SHIFT-M", "SHIFT-N", "SHIFT-O",
    "SHIFT-P",     "SHIFT-Q",     "SHIFT-R", "SHIFT-S", "SHIFT-T", "SHIFT-U", "SHIFT-V", "SHIFT-W",
    "SHIFT-X",     "SHIFT-Y",     "SHIFT-Z", "SHIFT-+", "CBM--",   "SHIFT--", "SHIFT-^", "CBM-*"
};

/* alternative keys for charcodes 0xa0-0xe0 */
static const char *a_cbmkeys[0x40] = {
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "",      "",
    "", "", "", "", "", "", "CBM-^", ""
};

/* ------------------------------------------------------------------------- */
/*
    the following are various tables containing control codes as printed in
    magazines and used by other programs.

    note that the codes should get reproduced *exactly* as they appear in the
    other application. if needed, new tables should be added.

FIXME: These are used by MikroBITTI:

These are the valid special substitions for TOK64:

{clear}             {home}              {right}         {left}              {up}                {down}
{reverse on}        {reverse off}       {black}         {white}             {red}               {cyan}
{purple}            {green}             {blue}          {yellow}            {orange}            {brown}
{pink}              {dark gray}         {gray}          {light green}       {light blue}        {light gray}
{f1}                {f2}                {f3}            {f4}                {f5}                {f6}
{f7}                {f8}                {space}         {pi}

codes originally used by 64er/Checksummer v3:

{DOWN}{UP}{CLR}{INST}{HOME}{DEL}{RIGHT}{LEFT}{SPACE}
{BLACK}{WHITE}{RED}{CYAN}{PURPLE}{GREEN}{BLUE}{YELLOW}{RVSON}{RVSOFF}
{ORANGE}{BROWN}{LIG.RED}{GREY 1}{GREY 2}{LIG.GREEN}{LIG.BLUE}{GREY 3}
{F1}{F2}{F3}{F4}{F5}{F6}{F7}{F8}{RETURN}

*/

/* 0x00 - 0x1f (petcat) */
static const char *ctrl1[0x20] = {
    "",              "CTRL-A",         "CTRL-B",     "stop",   "CTRL-D", "wht",    "CTRL-F",     "CTRL-G",
    "dish",          "ensh",           "\n",         "CTRL-K", "CTRL-L", "\n",     "swlc",       "CTRL-O",
    "CTRL-P",        "down",           "rvon",       "home",   "del",    "CTRL-U", "CTRL-V",     "CTRL-W",
    "CTRL-X",        "CTRL-Y",         "CTRL-Z",     "esc",    "red",    "rght",   "grn",        "blu"
};

/* 0x00 - 0x1f (FIXME: MikroBITTI) */
const char *a_ctrl1[0x20] = {
    "",              "",               "",           "",       "",       "WHT",    "",           "",
    "up/lo lock on", "up/lo lock off", "",           "",       "",       "return", "lower case", "",
    "",              "DOWN",           "RVS ON",     "HOME",   "delete", "",       "",           "",
    "",              "",               "",           "esc",    "RED",    "RIGHT",  "GRN",        "BLU"
};

/* 0x00 - 0x1f */
const char *b_ctrl1[0x20] = {
    "",              "",               "",           "",       "",       "",       "",           "",
    "",              "",               "",           "",       "",       "",       "",           "",
    "",              "",               "REVERSE ON", "",       "",       "",       "",           "",
    "",              "",               "",           "",       "",       "",       "",           ""
};

/* 0x00 - 0x1f (tok64) */
const char *c_ctrl1[0x20] = {
    "",              "",               "",           "",       "",       "white",  "",           "",
    "",              "",               "",           "",       "",       "",       "",           "",
    "",              "down",           "reverse on", "home",   "",       "",       "",           "",
    "",              "",               "",           "",       "red",    "right",  "green",      "blue"
};

/* 0x00 - 0x1f (64er/checksummer v3) */
const char *d_ctrl1[0x20] = {
    "",              "",               "",           "",       "",       "WHITE",  "",           "",
    "",              "",               "",           "",       "",       "RETURN", "",           "",
    "",              "DOWN",           "RVSON",      "HOME",   "DEL",    "",       "",           "",
    "",              "",               "",           "",       "RED",    "RIGHT",  "GREEN",      "BLUE"
};

/* ------------------------------------------------------------------------- */

/* 0x20 - 0x3f (petcat, tok64) */
const char *cbmchars[0x20] = {
    "space", "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", ""
};

/* 0x20 - 0x3f (64er/Checksummer v3) */
const char *a_cbmchars[0x20] = {
    "SPACE", "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", "",
    "",      "", "", "", "", "", "", ""
};

/* ------------------------------------------------------------------------- */

/* 0x80 - 0x9f (petcat) */
static const char *ctrl2[0x20] = {
    "",     "orng",         "",            "",           "",       "f1",           "f3",         "f5",
    "f7",   "f2",           "f4",          "f6",         "f8",     "sret",         "swuc",       "",
    "blk",  "up",           "rvof",        "clr",        "inst",   "brn",          "lred",       "gry1",
    "gry2", "lgrn",         "lblu",        "gry3",       "pur",    "left",         "yel",        "cyn"
};

/* 0x80 - 0x9f (FIXME: MikroBITTI) */
const char *a_ctrl2[0x20] = {
    "",      "orange",      "",            "",           "",       "F1",           "F3",         "F5",
    "F7",    "F2",          "F4",          "F6",         "F8",     "shift return", "upper case", "",
    "BLK",   "UP",          "RVS OFF",     "CLR",        "insert", "BROWN",        "LT.RED",     "GREY1",
    "GREY2", "lt green",    "LT.BLUE",     "GREY3",      "PUR",    "LEFT",         "YEL",        "cyn"
};

/* 0x80 - 0x9f (tok64) */
const char *b_ctrl2[0x20] = {
    "",      "orange",      "",            "",           "",        "f1",          "f3",         "r5",
    "f7",    "f2",          "f4",          "f6",         "f8",      "",            "",           "",
    "black", "up",          "reverse off", "clear",      "",        "brown",       "pink",       "dark gray",
    "gray",  "light green", "light blue",  "light gray", "purple",  "left",        "yellow",     "cyan",
};
/* 0x80 - 0x9f (64er/Checksummer v3) */
const char *c_ctrl2[0x20] = {
    "",      "ORANGE",      "",            "",           "",        "F1",          "F3",         "F5",
    "F7",    "F2",          "F4",          "F6",         "F8",      "",            "",           "",
    "BLACK", "UP",          "RVSOFF",      "CLR",        "INST",    "BROWN",       "LIG.RED",    "GREY 1",
    "GREY 2","LIG.GREEN",   "LIG.BLUE",    "GREY 3",     "PURPLE",  "LEFT",        "YELLOW",     "CYAN",
};

/* ------------------------------------------------------------------------- */

/*
 * Two BASIC tokens which need some special care
 */
#define TOKEN_REM  (0x8F - 0x80)
#define TOKEN_DATA (0x83 - 0x80)

/*
 * CBM Basic Keywords
 */

const char *keyword[] = {
    /* Common Keywords, 80 - cb */
    "end",    "for",    "next", "data", "input#",  "input",  "dim",    "read",
    "let",    "goto",   "run",  "if",   "restore", "gosub",  "return", "rem",
    "stop",   "on",     "wait", "load", "save",    "verify", "def",    "poke",
    "print#", "print",  "cont", "list", "clr",     "cmd",    "sys",    "open",
    "close",  "get",    "new",  "tab(", "to",      "fn",     "spc(",   "then",
    "not",    "step",   "+",    "-",    "*",       "/",      "^",      "and",
    "or",     ">",      "=",    "<",    "sgn",     "int",    "abs",    "usr",
    "fre",    "pos",    "sqr",  "rnd",  "log",     "exp",    "cos",    "sin",
    "tan",    "atn",    "peek", "len",  "str$",    "val",    "asc",    "chr$",
    "left$",  "right$", "mid$", "go",
    /*
     * The following codes (0xcc- 0xfe) are for 3.5, 7.0, and 10.0 only.
     * On 10.0 gshape, sshape, and draw are replaced with paste, cut, and line
     * respectively. */
    "rgr", "rlcr", "rlum" /* 0xce -- v7 prefix */, "joy",

    "rdot",     "dec",    "hex$",    "err$",    "instr",  "else",   "resume",    "trap",
    "tron",     "troff",  "sound",   "vol",     "auto",   "pudef",  "graphic",   "paint",
    "char",     "box",    "circle",  "gshape",  "sshape", "draw",   "locate",    "color",
    "scnclr",   "scale",  "help",    "do",      "loop",   "exit",   "directory", "dsave",
    "dload",    "header", "scratch", "collect", "copy",   "rename", "backup",    "delete",
    "renumber", "key",    "monitor", "using",   "until",  "while",

    /* 0xfe -- prefix */ "", "~" /* '~' is ASCII for 'pi' */
};


/*
 * 7.0, 7.1 and 10.0 only.
 * On 10.0 stash, fetch, and swap are replaced with dma.
 */

const char *kwce[] = {
    "",    "",        "pot",     "bump", "pen", "rsppos", "rsprite", "rspcolor",
    "xor", "rwindow", "pointer"
};


const char *kwfe[] = {
    "",         "",      "bank",     "filter", "play",    "tempo",  "movspr", "sprite",
    "sprcolor", "rreg",  "envelope", "sleep",  "catalog", "dopen",  "append", "dclose",
    "bsave",    "bload", "record",   "concat", "dverify", "dclear", "sprsav", "collision",
    "begin",    "bend",  "window",   "boot",   "width",   "sprdef", "quit",   "stash",
    "",         "fetch", "",         "swap",   "off",     "fast",   "slow",

    /* Basic 10.0 only (fe27 - fe3d) */
    "type",    "bverify", "ectory",     "erase", "find",       "change",  "set",       "screen",
    "polygon", "ellipse", "viewport",   "gcopy", "pen",        "palette", "dmode",     "dpat",
    "pic",     "genlock", "foreground", "",      "background", "border",  "highlight"
};


/* Basic 7.1 extension */

const char *kwfe71[] = {
    "",         "",      "bank",     "filter", "play",    "tempo",  "movspr", "sprite",
    "sprcolor", "rreg",  "envelope", "sleep",  "catalog", "dopen",  "append", "dclose",
    "bsave",    "bload", "record",   "concat", "dverify", "dclear", "sprsav", "collision",
    "begin",    "bend",  "window",   "boot",   "width",   "sprdef", "quit",   "stash",
    "",         "fetch", "",         "swap",   "off",     "fast",   "slow",

    /* Basic 7.1 extension only (fe27 - fe37) */
    "cwind", "sscrn", "lscrn", "hide",  "show", "sfont", "lfont", "view",
    "fcopy", "esave", "send",  "check", "esc",  "old",   "find",  "dump",
    "merge"
};

/* ------------------------------------------------------------------------- */

static void usage(char *progname);
static int parse_version(char *str);
static void list_keywords(int version);
static void pet_2_asc (int ctrls);
static void asc_2_pet (int ctrls);
static void _p_toascii(int c, int ctrls, int quote);
static int p_expand(int version, int addr, int ctrls);
static void p_tokenize(int version, unsigned int addr, int ctrls);
static unsigned char sstrcmp(unsigned char *line, const char **wordlist, int token, int maxitems);
static int sstrcmp_codes(unsigned char *line, const char **wordlist, int token, int maxitems);

/* ------------------------------------------------------------------------- */

static FILE *source, *dest;
static unsigned int kwlen = 0;
static int codesnocase = 0; /* flag, =1 if controlcodes should be interpreted case insensitive */
static int quotedcodes = 0; /* flag, =1 if non alphanumeric characters inside quotes should always be converted to controlcodes */
static int dec = 0;         /* flag, =1 if output control codes in decimal */
static int verbose = 0;     /* flag, =1 for verbose output */

static const unsigned char MagicHeaderP00[8] = "C64File\0";

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    char *progname, *outfilename = NULL;
    int c = 0;

    long offset = 0;
    int wr_mode = 0, version = B_7;         /* best defaults */
    int load_addr = 0, ctrls = -1, hdr = -1, show_words = 0;
    int fil = 0, outf = 0, overwrt = 0, textmode = 0;
    int flg = 0;                            /* files on stdin */

    archdep_init(&argc, argv);

    /* Parse arguments */
    progname = argv[0];
    while (--argc && ((*++argv)[0] == '-')) {
        if (!strcmp(argv[0], "--")) {
            --argc;
            ++argv;
            break;
        }
        if (!strcmp(argv[0], "-v")) {
            verbose = 1;
            continue;
        }

        if (!strcmp(argv[0], "-l")) {           /* load address */
            if (argc > 1 && sscanf(argv[1], "%x", &load_addr) == 1) {
                --argc; ++argv;
                continue;
            }
            /* Fall to error */
        }

        if (!strcmp(argv[0], "-ic")) {
            codesnocase = 1;
            continue;
        }

        if (!strcmp(argv[0], "-qc")) {
            quotedcodes = 1;
            continue;
        }

        if (!strcmp(argv[0], "-c")) {
            ctrls = 1;
            continue;
        } else {
            if (!strcmp(argv[0], "-nc")) {
                ctrls = 0;
                continue;
            }
        }

        if (!strcmp(argv[0], "-d")) {
            dec = 1;
            continue;
        }

        if (!strcmp(argv[0], "-h")) {
            hdr = 1;
            continue;
        } else if (!strcmp(argv[0], "-nh")) {
            hdr = 0;
            continue;
        } else if (!strcmp(argv[0], "-f")) {      /* force overwrite */
            ++overwrt;
            continue;
        } else if (!strcmp(argv[0], "-o")) {
            if (argc > 1) {
                outfilename = argv[1];
                ++outf;
                --argc; ++argv;
                continue;
            }
            fprintf (stderr, "\nOutput filename missing\n");
            /* Fall to error */
        }

        /* reading offset */
        if (!strcmp(argv[0], "-skip") || !strcmp(argv[0], "-offset")) {
            if (argc > 1 && sscanf(argv[1], "%lx", &offset) == 1) {
                --argc; ++argv;
                continue;
            }
            /* Fall to error */
        } else if (!strcmp(argv[0], "-text")) {   /* force text mode */
            ++textmode;
            continue;
        } else if (!strcmp(argv[0], "-help") || !strncmp(argv[0], "-?", 2)) {  /* version ID */
            /* Fall to error for Usage */

            /* Basic version */
        } else if (!strncmp(argv[0], "-w", 2) && !wr_mode) {
            version = parse_version((strlen(argv[0]) > 2 ? &argv[0][2] : NULL));
            ++wr_mode;
            continue;
        } else if (!strncmp(argv[0], "-k", 2) && !wr_mode) {
            version = parse_version((strlen(argv[0]) > 2 ? &argv[0][2] : NULL));
            ++show_words;
            continue;
        } else if ((version = parse_version(&argv[0][1])) >= 0) {
            continue;
        }

        usage(progname);
        exit(1);
    }

/******************************************************************************
 * Check parameters
 */

    if (argc) {
        fil++;
    }

    if (hdr == -1) {
        hdr = outf ? 0 : 1;
    }

    if (version == B_10) {
        keyword[0x63] = "paste";
        keyword[0x64] = "cut";
        keyword[0x65] = "line";
        keyword[0x6e] = "dir";
        kwfe[0x1f] = "dma";
        kwfe[0x21] = "dma";
        kwfe[0x23] = "dma";
    }

    if (show_words) {
        list_keywords(version);
        return (0);
    }

    if (ctrls < 0) {
        ctrls = (textmode ? 0 : 1);     /*default ON for prgs, OFF for text */
    }

    if (!load_addr) {
        load_addr = basic_list[version - 1].load_address;
    }

    if (verbose) {
        if (wr_mode) {
            if (!textmode) {
                fprintf(stderr, "\nLoad address %04x\n", load_addr);
                if ((load_addr & 255) != 1) {
                    fprintf (stderr, "Warning: odd load address (are you sure?)\n");
                }
            }
            fprintf(stderr, "Control code set: %s\n\n", (ctrls ? "enabled" : "disabled"));
        }
    }

    /*
     * Loop all files
     */

    do {
        int plen = 0;
        flg = 0;        /* stdin loop flag */


        /*
         * Try to figure out whether input file is in P00 format or not.
         * If the header is found, the real filaname is feched and any other
         * offset specified is overruled.
         * This is particularly difficult on <stdin>, as only _one_ character
         * of pushback is guaranteed on all cases. So, the lost bytes "C64File"
         * are recreated from the magic header while printing the (text) file.
         */

        if (!fil) {
            const unsigned char *p;

            source = stdin;

            for (plen = 0, p = MagicHeaderP00; plen < 8 && (c = getc(source)) != EOF && (unsigned)c == *p; ++plen, ++p) {}

            if (plen == 8) {
                /* skip the rest of header */
                for (plen = 18; plen > 0 && getc(source) != EOF; --plen) {}
            } else {
                /*printf("P00 failed at location %d.\n", plen);*/
                ungetc(c, source);
            }
        } else {
            if ((source = fopen(argv[0], "rb")) == NULL) {
                fprintf(stderr, "\n%s: Can't open file %s\n", progname, argv[0]);
                exit(1);
            }
        }


        if (!outf) {
            dest = stdout;
        } else {
            if ((dest = fopen(outfilename, "wb")) == NULL) {
                fprintf(stderr, "\n%s: Can't open output file %s\n", progname, outfilename);
                exit(1);
            }
        }


        if (wr_mode) {
            if (textmode) {
                asc_2_pet(ctrls);
            } else {
                p_tokenize(version, load_addr, ctrls);
            }
        } else {
            if (hdr) { /* name as comment when using petcat name.prg > name.txt */
                fprintf(dest, "\n\n;%s ", (fil ? argv[0] : "<stdin>"));
            }

            /*
             * Use TEXT mode if the offset doesn't equal BASIC load addresses
             * and the first bytes to be read do not contain load address.
             * Explicitly selected textmode overrules these conditions.
             */

            if (textmode || ((offset & 255) != 1 && ((c = getc(source)) != EOF && ungetc(c, source) != EOF && c && c != 1))) {
                /* Print the bytes lost in header check */
                if (plen > 0 && plen < 8) {
                    for (c = 0; c < plen; ++c) {
                        fputc (MagicHeaderP00[(int)c], dest);
                    }
                }

                pet_2_asc(ctrls);
            } else {
                load_addr = (getc(source) & 0xff);
                load_addr |= (getc(source) & 0xff) << 8;
                if (hdr) {
                    fprintf(dest, "==%04x==\n", load_addr);
                }

                if (p_expand(version, load_addr, ctrls)) {
                    fprintf(dest, "\n;*** Machine language part skipped. ***\n");
                } else {   /* End of BASIC on stdin. Is there more ? */
                    if (!fil && (c = getc(source)) != EOF && ungetc(c, source) != EOF && c) {
                        ++flg;
                        ++hdr;
                    }
                }
            }

            if (hdr) {
                fputc('\n', dest);
            }
        }


        if (fil) {
            fclose(source);
        }
        if (outf) {
            fclose(dest);
        }
    } while (flg || (fil && --argc && ++argv));           /* next file */
    return(0);
}

/* ------------------------------------------------------------------------- */
void usage(char *progname)
{
    int i;

    fprintf(stdout,
            "\n\t%s V%4.2f PL %d -- Basic list/crunch utility.\n\tPart of "PACKAGE " "VERSION "\n",
            progname, (float)PETCATVERSION, PETCATLEVEL );

    fprintf(stdout,
            "\nUsage: %7s  [-c | -nc]  [-h | -nh]  [-text | -<version> | -w<version>]"
            "\n\t\t[-skip <bytes>] [-l <hex>]  [--] [file list]\n\t\t[-k[<version>]]\n",
            progname);

    fprintf(stdout, "\n"
            "   -help -?\tOutput this help screen here\n"
            "   -v\t\tverbose output\n"
            "   -c\t\tcontrols (interpret also control codes) <default if textmode>\n"
            "   -nc\t\tno controls (suppress control codes in printout)\n"
            "   \t\t<default if non-textmode>\n"
            "   -ic\t\tinterpret control codes case-insensitive\n"
            "   -qc\t\tconvert all non alphanumeric characters inside quotes into controlcodes\n"
            "   -d\t\toutput raw codes in decimal\n"
            "   -h\t\twrite header <default if output is stdout>\n"
            "   -nh\t\tno header <default if output is a file>\n"
            "   -skip <n>\tSkip <n> bytes in the beginning of input file. Ignored on P00.\n"
            "   -text\tForce text mode\n"
            "   -<version>\tuse keywords for <version> instead of the v7.0 ones\n"
            "   -w<version>\ttokenize using keywords on specified Basic version.\n"
            "   -k<version>\tlist all keywords for the specified Basic version\n"
            "   -k\t\tlist all Basic versions available.\n"
            "   -l\t\tSpecify load address for program (in hex, no leading chars!).\n"
            "   -o <name>\tSpecify the output file name\n"
            "   -f\t\tForce overwritten the output file\n"
            "   \t\tThe default depends on the BASIC version.\n");

    fprintf(stdout, "\n\tVersions:\n");
    for (i = 0; basic_list[i].version_select; ++i) {
            fprintf(stdout, "\t%s\t%s\n", basic_list[i].version_select, basic_list[i].name);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "\tUsage examples:\n"
            "\tpetcat -2 -o outputfile.txt -- inputfile.prg\n"
            "\t\tDe-tokenize, convert inputfile.prg to a text file\n"
            "\t\tin outputfile.txt, using BASIC V2 only\n"
            "\tpetcat -wsimon -o outputfile.prg -- inputfile.txt\n"
            "\t\tTokenize, convert inputfile.txt to a PRG file\n"
            "\t\tin outputfile.prg, using Simon's BASIC\n"
            "\tpetcat -text -o outputfile.txt -- inputfile.seq\n"
            "\t\tConvert inputfile.seq to a Ascii text file\n"
            "\t\tin outputfile.txt.\n"
            "\tpetcat -text -w2 -o outputfile.seq -- inputfile.txt\n"
            "\t\tConvert inputfile.txt to a Petscii text SEQ file\n"
            "\t\tin outputfile.seq.\n");
}

/* ------------------------------------------------------------------------- */
/* Parse given version name and return its code, or -1 if not recognized. */

static int parse_version(char *str)
{
    int i;

    if (str == NULL || !*str) {
        return 0;
    }

    for (i = 0; basic_list[i].version_select; ++i) {
        if (!strncasecmp(str, basic_list[i].version_select, strlen(basic_list[i].version_select))) {
            return i + 1;
        }
    }

    fprintf(stderr, "\nUnimplemented version '%s'\n", str);

    return -1;
}

static void list_keywords(int version)
{
    int n, max;

    if (version <= 0 || version > NUM_VERSIONS) {
        printf("\n  The following versions are supported on  %s V%4.2f\n\n", "petcat", (float)PETCATVERSION );

        for (n = 0; basic_list[n].name; n++) {
            printf("\t%s\n", basic_list[n].name);
        }
        printf("\n");
        return;
    }

    printf("\n  Available Keywords on %s\n\n", basic_list[version - 1].name);

    if (version == B_1) {
        max = basic_list[B_1 - 1].num_tokens;
    } else if (version == B_35 || version == B_7 || version == B_71 || version == B_10) {
        max = basic_list[B_35 - 1].num_tokens;
    } else {
        max = basic_list[B_2 - 1].num_tokens;
    }

    for (n = 0; n < max; n++) {
        if (version == B_35 || n != 0x4e) {      /* Skip prefix marker */
            printf("%s\t", keyword[n] /*, n | 0x80*/);
        }
    }
    printf("%s\n", keyword[127]);


    if (version == B_7 || version == B_71 || version == B_10 || version == B_SXC) {
        for (n = basic_list[version - 1].token_offset; n < basic_list[version - 1].num_tokens; n++) {
            printf("%s\t", kwfe[n] /*, 0xfe, n*/);
        }

        if (version != B_SXC) {
            for (n = basic_list[version - 1].token_offset; n < NUM_KWCE; n++) {
                printf("%s\t", kwce[n] /*, 0xce, n*/);
            }
        }
    } else {
        switch (version) {
            case B_EXPBAS64:
            case B_SUPERBAS:
            case B_BLARG:
            case B_EASY:
            case B_MAGIC:
            case B_MIGHTY:
            case B_TURTLE:
            case B_SUPEREXP:
            case B_BSX:
            case B_GB:
            case B_VIC5:
            case B_VIC4:
            case B_4:
            case B_4E:
            case B_WARSAW:
            case B_EXPBAS20:
            case B_WSF:
            case B_BASL:
            case B_REU:
            case B_DRAGO:
            case B_X:
            case B_PEG:
            case B_WS:
            case B_GRAPH:
            case B_ULTRA:
            case B_FC3:
            case B_SPEECH:
            case B_ATBAS:
            case B_SUPERGRA:
            case B_KIPPER:
            case B_BOB:
            case B_EVE:
            case B_SIMON:
            case B_TT64:
                for (n = basic_list[version - 1].token_offset; n < basic_list[version - 1].num_tokens; n++) {
                    printf("%s\t", basic_list[version -1].tokens[n] /*, n + 0xcc*/);
                }
                break;
        }  /* switch */
    }

    printf("\n\n");
}


/* ------------------------------------------------------------------------- */

/*
 * Conversion Routines
 */

/* used in text mode */
static void pet_2_asc(int ctrls)
{
    int c;

    while ((c = getc(source)) != EOF) {
        _p_toascii(c, ctrls, 0);           /* convert character */
    }      /* line */
}

/*******************************************************************************
    translate petscii code into an ascii representation

    notes:
    - petscii codes 0x61-0x7f and (*) 0xc1-0xdf produce the same screencodes
    - petscii codes (*) 0xa1-0xbe and 0xe1-0xfe produce the same screencodes
    - petscii codes (*) 0xff, 0x7e and 0xde (PI) produces the same screencode
    
 ******************************************************************************/
static void out_ctrl(unsigned char c)
{
    if (dec) {
        fprintf(dest, CLARIF_LP_ST "%03d" CLARIF_RP_ST, c);
    } else {
        fprintf(dest, CLARIF_LP_ST "$%02x" CLARIF_RP_ST, c);
    }
}

static void _p_fputc(int c, int p, int quote)
{
   if (quote && quotedcodes) {
        /* if enabled, output all quoted non alphanumeric characters as control codes */
        if (!(((c >= 'a') && (c <= 'A')) ||
              ((c >= 'A') && (c <= 'Z')) ||
              ((c >= '0') && (c <= '9')) ||
              (c == '"') /* needed so the leading quote does NOT get converted into a control code */
           )) {
            out_ctrl((unsigned char)(p & 0xff));
            return;
        }
    }
    fputc(c, dest);
}

static void _p_toascii(int c, int ctrls, int quote)
{
    /* fprintf(stderr, "<%02x:%d>", c, ctrls); */
    switch (c) {
        case 0x00: /* 00 for SEQ */
        case 0x0a:
            if (!ctrls) {
                _p_fputc('\n', c, quote);
            } else {
                out_ctrl((unsigned char)(c & 0xff));
            }
            break;
        case 0x0d: /* CBM carriage return */
            _p_fputc('\n', c, quote);
            break;
        case 0x40:
            _p_fputc('@', c, quote);
            break;
        case 0x5b:
            _p_fputc('[', c, quote);
            break;
        case 0x5c:
            _p_fputc('\\', c, quote);
            break;
        case 0x5d:
            _p_fputc(']', c, quote);
            break;
        case 0x5e:
            _p_fputc('^', c, quote);
            break;
        case 0x5f: /* left arrow */
            _p_fputc('_', c, quote);
            break;
        case 0x60: /* produces the same screencode as $c0! */
            out_ctrl((unsigned char)(c & 0xff));
            break;

        case 0x7b: /* produces the same screencode as $db! */
        case 0x7c: /* produces the same screencode as $dc! */
        case 0x7d: /* produces the same screencode as $dd! */
        case 0x7e: /* PI produces the same screencode as $de! */
        case 0x7f: /* produces the same screencode as $df! */
            out_ctrl((unsigned char)(c & 0xff)); /* shift+arrow up */
            break;

        case 0xc0:
            fprintf(dest, CLARIF_LP_ST "SHIFT-*" CLARIF_RP_ST);
            break;
        case 0xdb: /* (*) produces the same screencode as $7b! */
            fprintf(dest, CLARIF_LP_ST "SHIFT-+" CLARIF_RP_ST);
            break;
        case 0xdc: /* (*) produces the same screencode as $7c! */
            fprintf(dest, CLARIF_LP_ST "CBM--" CLARIF_RP_ST); /* Conflicts with Scandinavian Chars */
            break;
        case 0xdd: /* (*) produces the same screencode as $7d! */
            fprintf(dest, CLARIF_LP_ST "SHIFT--" CLARIF_RP_ST);
            break;
        case 0xde: /* PI produces the same screencode as $7e and $ff! */
            out_ctrl((unsigned char)(c & 0xff));
            break;
        case 0xdf: /* (*) produces the same screencode as $7f! */
            fprintf(dest, CLARIF_LP_ST "CBM-*" CLARIF_RP_ST);
            break;

        case 0xa0: /* shifted Space */
        case 0xe0: /* produces the same screencode as $a0! */
            if (!ctrls) {
                _p_fputc(' ', c, quote);
            } else {
                out_ctrl((unsigned char)(c & 0xff));
            }
            break;
        case 0xff: /* (*) PI produces the same screencode as $7e and $de! */
            _p_fputc(0x7e, c, quote); /*  '~' is ASCII for 'pi' */
            break;

        default:
            switch (c & 0xe0) {
                case 0x40:                /* 41 - 5F (no duplicated set exists) */
                    _p_fputc(c ^ 0x20, c, quote);
                    break;
                case 0x60:                /* 61 - 7F (produces same screencodes as C1...) */
                    if (ctrls) {
                        out_ctrl((unsigned char)(c & 0xff));
                    } else {
                        _p_fputc(c ^ 0x20, c, quote);
                    }
                    break;
                case 0xa0:                /* (primary set) A1 - BF (produces same screencodes as E1...) */
                    fprintf(dest, CLARIF_LP_ST "%s" CLARIF_RP_ST, cbmkeys[c & 0x1f]);
                    break;
                case 0xe0:                /* E1 - FE (produces same screencodes as A1...) */
                    out_ctrl((unsigned char)(c & 0xff));
                    break;
                case 0xc0:                /* (primary set) C0 - DF (produces same screencodes as 61...) */
                    _p_fputc(c ^ 0x80, c, quote);
                    break;

                default:
                    if ((c > 0x1f) && isprint(c)) {
                        _p_fputc(c, c, quote);
                    } else if (ctrls) {
                        if ((c < 0x20) && *ctrl1[c]) {
                            fprintf(dest, CLARIF_LP_ST "%s" CLARIF_RP_ST, ctrl1[c]);
                        } else if ((c > 0x7f) && (c < 0xa0) && *ctrl2[c & 0x1f]) {
                            fprintf(dest, CLARIF_LP_ST "%s" CLARIF_RP_ST, ctrl2[c & 0x1f]);
                        } else {
                            out_ctrl((unsigned char)(c & 0xff));
                        }
                    }  /* ctrls */
            }  /* switch */
    }  /* switch */
}

static int _a_topetscii(int c, int ctrls)
{
    if (c == '\n') {
        return 0x0d;
    } else if (c == 0x7e) {              /*  '~' is ASCII for 'pi' */
        return 0xff;
    } else if ((c >= 0x5b) && (c <= 0x5f)) { /* iAN: '_' -> left arrow, no char value change */
        return c;
    } else if ((c >= 0x60) && (c <= 0x7e)) {
        return c ^ 0x20;
    } else if ((c >= 'A') && (c <= 'Z')) {
        return c | 0x80;
    }
    return c;
}

/* read a decimal integer from string. we do it in a seperate function because
 * of the ugly GEMDOS hack */
static int scan_integer(const char *line, unsigned int *num, unsigned int *digits)
{
#ifdef GEMDOS
    *digits = 0;
    if (sscanf(line, "%d", num) == 1) {
        while (isspace(*line) || isdigit(*line)) {
            line++;
            (*digits)++;
        }
        return *digits;
    }
#else
    if (sscanf(line, "%d%n", num, digits) == 1) {
        return *digits;
    }
#endif
    return 0;
}

/* ------------------------------------------------------------------------- */
/*
 * convert basic (and petscii) to ascii (text)
 *
 * This routine starts from the beginning of Basic, and not from the
 * load address included on program files. That way it can list from
 * RAM dump.
 */

static int p_expand(int version, int addr, int ctrls)
{
    static char line[4];
    unsigned int c;
    int quote, spnum, directory = 0;
    int sysflg = 0;

    /*
     * It seems to be common mistake not to terminate BASIC properly
     * before the machine language part, so we don't check for the
     * low byte of line link here.
     * Line link and line number are read separately to leave possible
     * next file on stdin intact.
     */

    while ((fread(line, 1, 2, source) == 2) && (line[1]) && fread(line + 2, 1, 2, source) == 2) {
        quote = 0;
        fprintf(dest, " %4d ", (spnum = (line[2] & 0xff) + ((line[3] & 0xff) << 8)));

        if (directory) {
            if (spnum >= 100) {
                spnum = 0;
            } else if (spnum >= 10) {
                spnum = 1;
            } else {
                spnum = 2;
            }
        }

        /* prevent list protection from terminating listing */

        while ((c = getc(source)) != EOF && !c) {
        }

        if (c == 0x12 && !line[2] && !line[3]) {  /* 00 00 12 22 */
            directory++;
        }

        do {
            if (c == 0x22) {
                quote ^= c;
            }

            /*
             * Simon's basic. Any flag for this is not needed since it is
             * mutually exclusive with all other implemented modes.
             */

            if (!quote && (c == 0x64)) {
                if ((c = getc(source)) < 0x80) {
                    fprintf(dest, "%s", basic_list[version - 1].tokens[c]);
                    continue;
                } else {
                    fprintf(dest, "($64)");     /* it wasn't prefix */
                }
            }

            /* basic 2.0, 7.0 & 10.0 and extensions */

            if (!quote && c > 0x7f) {
                /* check for keywords common to all versions, include pi */
                if (c <= basic_list[B_1 - 1].max_token || c == 0xff) {
                    fprintf(dest, "%s", keyword[c & 0x7f]);

                    if (c == 0x9E) {
                        ++sysflg;
                    }
                    continue;
                }
                if (version != B_35 && version != B_FC3) {
                    if (c == 0xce) {            /* 'rlum' on V3.5*/
                        if ((c = getc(source)) <= MAX_KWCE) {
                            fprintf(dest, "%s", kwce[c]);
                        } else {
                            fprintf(dest, "($ce%02x)", c);
                        }
                        continue;
                    } else if (c == 0xfe) {
                        if (version == B_SXC) {
                            if ((c = getc(source)) <= basic_list[B_SXC - 1].max_token) {
                                fprintf(dest, "%s", basic_list[version - 1].tokens[c]);
                            } else {
                                fprintf(dest, "($fe%02x)", c);
                            }
                        } else {
                            if ((c = getc(source)) <= basic_list[B_10 - 1].max_token) {
                                fprintf(dest, "%s", (version == B_71) ? kwfe71[c] : kwfe[c]);
                            } else {
                                fprintf(dest, "($fe%02x)", c);
                            }
                        }
                        continue;
                    }
                }
                switch (version) {
                    case B_2:
                    case B_EASY:
                    case B_MAGIC:
                    case B_MIGHTY:        /* VIC Mightyb basic */
                    case B_TURTLE:        /* VIC extension as well */
                    case B_EXPBAS20:
                    case B_SUPEREXP:         /* VIC extension */
                    case B_EXPBAS64:
                    case B_BSX:
                    case B_GB:
                    case B_WSF:
                    case B_VIC5:
                    case B_VIC4:
                    case B_4:             /* PET V4.0 */
                    case B_4E:            /* V4.0 extension */
                    case B_BASL:
                    case B_REU:
                    case B_DRAGO:
                    case B_X:
                    case B_PEG:
                    case B_WS:
                    case B_GRAPH:
                    case B_ULTRA:         /* C64 Ultrabasic-64 */
                    case B_FC3:           /* C64 FC3 */
                    case B_ATBAS:         /* C64 Atbasic */
                    case B_SUPERGRA:
                    case B_SPEECH:        /* C64 Speech basic */
                    case B_BLARG:
                    case B_SUPERBAS:
                    case B_WARSAW:
                    case B_KIPPER:
                    case B_BOB:
                    case B_EVE:
                    case B_TT64:
                        if (c >= basic_list[version - 1].token_start && c <= basic_list[version - 1].max_token) {
                            fprintf(dest, "%s", basic_list[version - 1].tokens[c - basic_list[version - 1].token_start]);
                        }
                        break;

                    default:              /* C128 */
                        fprintf(dest, "%s", keyword[c & 0x7f]);
                }
                continue;
            } /* quote */

            if (directory && spnum && c == 0x20) {
                spnum--;          /* eat spaces to adjust directory lines */
                continue;
            }

            _p_toascii((int)c, ctrls, quote);  /* convert character */
        } while ((c = getc(source)) != EOF && c);
        fprintf(dest, "\n");
    }      /* line */

    DBG(("\n c %02x  EOF %d  *line %d  sysflg %d\n", c, feof(source), *line, sysflg));

    return (!feof(source) && (*line | line[1]) && sysflg);
}

/* ------------------------------------------------------------------------- */
/* convert ascii (basic) to tokenized basic (and petscii) */

#define MAX_INLINE_LEN  (256 * 8)
#define MAX_OUTLINE_LEN 256

static void p_tokenize(int version, unsigned int addr, int ctrls)
{
    static char line[MAX_INLINE_LEN + 1];
    static char tokenizedline[MAX_OUTLINE_LEN + 1];
    unsigned char *p1, *p2, quote;
    int c;
    unsigned char rem_data_mode, rem_data_endchar = '\0';
    unsigned int len = 0, match;
    unsigned int linum = 10;

    /* put start address to output file */
    fprintf(dest, "%c%c", (addr & 255), ((addr >> 8) & 255));

    /* Copies from p2 to p1 */

    while ((p2 = (unsigned char *)fgets(line, MAX_INLINE_LEN, source)) != NULL) {
        /* skip comment line when starting with ";" */
        if (*line == ';') {
            continue;
        }

        memset(tokenizedline, 0, MAX_OUTLINE_LEN);
        p1 = (unsigned char *)tokenizedline;

        p2 += scan_integer(line, &linum, &len); /* read decimal from "line" into "linum" */

        DBG(("line: %d [%s]\n", linum, line));

        quote = 0;
        rem_data_mode = 0;
        while (isspace(*p2)) {
            p2++;
        }

        while (*p2) {
            if (*p2 == 0x22) {
                quote ^= *p2;
            }
            if (*p2 == 0x0a || *p2 == 0x0d) {
                break;
            }

            match = 0;
            if (quote) {
                /*
                 * control code evaluation
                 * only strings that appear inside quotes are
                 * interpreted -- they should not be used elsewhere
                 */

                if (ctrls && (*p2 == CLARIF_LP)) {
                    unsigned char *p;
                    p = p2;

                    DBG(("controlcode start: %c\n", *p2));

                    /* repetition count */
                    len = 1;
                    if (scan_integer((char *)++p, &len, &kwlen) > 0) {
                        p += kwlen;

                        /* if we are already at the closing brace, then the previous
                           value wasnt the repeat count but an actual decimal charactercode */
                        if (*p == CLARIF_RP) {
                            *p1++ = len; /* put charcode into output */
                            p2 = p + 1; /* skip the closing brace in input */
                            DBG(("controlcode was a decimal character code: {%d}\n", len));
                            continue;
                        }

                        DBG(("controlcode repeat count: len:%d kwlen:%d\n", len, kwlen));

                        if (*p == ' ') {
                            ++p;
                        }
                    }

                    DBG(("controlcode test: %s\n", p));

                    if (
                        (
                            ((c = sstrcmp_codes(p, hexcodes, 0, 0x100)) != CODE_NONE) || /* 0x00-0xff */

                            ((c = sstrcmp_codes(p, ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                            ((c = sstrcmp_codes(p, a_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                            ((c = sstrcmp_codes(p, b_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                            ((c = sstrcmp_codes(p, c_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                            ((c = sstrcmp_codes(p, d_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */

                            ((((c = sstrcmp_codes(p, cbmchars, 0, 0x20)) != CODE_NONE) || /* 0x20-0x3f */
                              ((c = sstrcmp_codes(p, a_cbmchars, 0, 0x20)) != CODE_NONE) /* 0x20-0x3f */
                              ) && (c += 0x20)) ||

                            ((((c = sstrcmp_codes(p, ctrl2, 0, 0x20)) != CODE_NONE) ||
                              ((c = sstrcmp_codes(p, a_ctrl2, 0, 0x20)) != CODE_NONE) ||
                              ((c = sstrcmp_codes(p, b_ctrl2, 0, 0x20)) != CODE_NONE) ||
                              ((c = sstrcmp_codes(p, c_ctrl2, 0, 0x20)) != CODE_NONE)
                             ) && (c += 0x80)) ||

                            ((((c = sstrcmp_codes(p, cbmkeys, 0, 0x40)) != CODE_NONE) ||
                              ((c = sstrcmp_codes(p, a_cbmkeys, 0, 0x40)) != CODE_NONE)
                              ) && (c += 0xA0))


                        )
                        ) {

                        DBG(("controlcode test 2: '%c' '%s' '%d'\n", p[kwlen], p, kwlen));

                        if (p[kwlen] == '*') {
                            /* repetition count */
                            p += (kwlen);

                            DBG(("controlcode test rpt: %s\n", p));

                            len = 1;

                            if (scan_integer((char *)++p, &len, &kwlen) > 0) {
                                p += kwlen;
                                DBG(("controlcode repeat count: len:%d kwlen:%d\n", len, kwlen));
                                kwlen = 0;
                            }
                        }

                        DBG(("controlcode test 3: '%c' '%s' '%d'\n", p[0], p, kwlen));

                        if (p[kwlen] == CLARIF_RP) {
                            for (; len-- > 0; ) {
                                *p1++ = c;
                            }
                            p2 = p + (++kwlen);

                            DBG(("controlcode continue\n"));
                            continue;
                        }
                    }

                    fprintf(stderr, "error: line %d - unknown control code: %s\n", linum, p);
                    exit(-1);
                }
/*	    DBG(("controlcode end\n")); */
            } else if (rem_data_mode) {
                /* if we have already encountered a REM or a DATA,
                   simply copy the char */

                /* DO NOTHING! As we do not set "match", the if (!match) will be true,
                 * and this part will copy the char over to the new buffer */
            } else if (isalpha(*p2) || strchr("+-*/^>=<", *p2)) {
                /* FE and CE prefixes are checked first */
                if (version == B_7 || version == B_71 || version == B_10 || version == B_SXC) {
                    switch (version) {
                        case B_10:
                        case B_71:
                        case B_7:
                            if ((c = sstrcmp(p2, kwfe, basic_list[version - 1].token_offset, basic_list[version - 1].num_tokens)) != KW_NONE) {
                                *p1++ = 0xfe;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            } else if ((c = sstrcmp(p2, kwce, basic_list[version - 1].token_offset, NUM_KWCE)) != KW_NONE) {
                                *p1++ = 0xce;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                        case B_SXC:
                            if ((c = sstrcmp(p2, basic_list[version - 1].tokens, basic_list[version - 1].token_offset, basic_list[version - 1].num_tokens)) != KW_NONE) {
                                *p1++ = 0xfe;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                    }
                }

                /* Common Keywords
                 * Note:  ~ (pi) is tested later */

                if (!match) {
                    int max;

                    if (version == B_1) {
                        max = basic_list[B_1 - 1].num_tokens;
                    } else if ((version == B_35) || (version == B_7) || (version == B_71) ||
                               (version == B_10) || (version == B_SXC)) {
                        max = basic_list[B_35 - 1].num_tokens;
                    } else {
                        max = basic_list[B_2 - 1].num_tokens;
                    }

                    if ((c = sstrcmp(p2, keyword, 0, max)) != KW_NONE) {
                        if ((version == B_35) || (c != 0x4e)) {  /* Skip prefix */
                            *p1++ = c | 0x80;
                            p2 += kwlen;
                            match++;

                            /* Check if the keyword is a REM or a DATA */
                            switch (c) {
                                case TOKEN_DATA:
                                    rem_data_mode = 1;
                                    rem_data_endchar = ':';
                                    break;

                                case TOKEN_REM:
                                    rem_data_mode = 1;
                                    rem_data_endchar = '\0';
                                    break;
                            }
                        }
                    }
                }

                if (!match) {
                    switch (version) {
                        case B_EXPBAS64:
                        case B_BSX:
                        case B_GB:
                        case B_WSF:
                        case B_VIC5:
                        case B_VIC4:
                        case B_4:
                        case B_4E:
                        case B_MAGIC:
                        case B_MIGHTY:
                        case B_BASL:
                        case B_REU:
                        case B_DRAGO:
                        case B_X:
                        case B_PEG:
                        case B_WS:
                        case B_GRAPH:
                        case B_ULTRA:
                        case B_FC3:
                        case B_ATBAS:
                        case B_SPEECH:
                        case B_EASY:
                        case B_TURTLE:
                        case B_SUPEREXP:
                        case B_EXPBAS20:
                        case B_SUPERGRA:
                        case B_BLARG:
                        case B_KIPPER:
                        case B_BOB:
                        case B_EVE:
                        case B_TT64:
                        case B_WARSAW:
                        case B_SUPERBAS:
                            if ((c = sstrcmp(p2, basic_list[version - 1].tokens, basic_list[version - 1].token_offset, basic_list[version - 1].num_tokens)) != KW_NONE) {
                                *p1++ = c + basic_list[version - 1].token_start;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_SIMON:
                            if ((c = sstrcmp(p2, basic_list[version - 1].tokens, basic_list[version - 1].token_offset, basic_list[version - 1].num_tokens)) != KW_NONE) {
                                *p1++ = 0x64;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                    } /* switch */
                }
            } /* !quote */

            if (!match) {
                /* convert character */
                *p1++ = _a_topetscii(*p2 & 0xff, ctrls);

                /* check if the REM/DATA mode has to be stopped: */
                if (*p2 == rem_data_endchar) {
                    rem_data_mode = 0;
                }

                ++p2;
            } /* match */
        } /* while */

        DBG(("output line start: %s\n", line));
        /*  DBG(("output line petscii: %s\n", tokenizedline)); */

        *p1 = 0;
        if ((len = (int)strlen(tokenizedline)) > 0) {
            addr += (len + 5);
            fprintf(dest, "%c%c%c%c%s%c", addr & 255, (addr >> 8) & 255,
                    linum & 255, (linum >> 8) & 255, tokenizedline, '\0');

            linum += 2; /* auto line numbering by default */
        }

        DBG(("output line end\n"));
    } /* while */

    fprintf(dest, "%c%c", 0, 0);        /* program end marker */
}

/* ------------------------------------------------------------------------- */
/* convert ascii (text) to petscii */
static void asc_2_pet(int ctrls)
{
    static unsigned char line[MAX_INLINE_LEN + 1];
    int c, d;
    unsigned int len = 0;

    /* Copies from p2 to p1 */
    while ((c = fgetc(source)) != EOF) {
        /* control code evaluation */
        if (ctrls && (c == CLARIF_LP)) {
            unsigned char *p;
            int pos;
            pos = ftell(source);
            if (fread(line, 1, 0x20, source) < 1) {
                break;
            }
            p = line;

            DBG(("asc_2_pet controlcode start: %c\n", c));

            /* repetition count */
            len = 1;
            if (scan_integer((char *)p, &len, &kwlen) > 0) {
                p += kwlen;
                /* if we are already at the closing brace, then the previous
                    value wasnt the repeat count but an actual decimal charactercode */
                if (*p == CLARIF_RP) {
                    fputc(len, dest); /* output character */
                    fseek(source, pos + kwlen + 1, SEEK_SET);
                    continue;
                }

                DBG(("asc_2_pet controlcode repeat count: len:%d kwlen:%d\n", len, kwlen));

                if (*p == ' ') {
                    ++p;
                }
            }

            DBG(("asc_2_pet controlcode test: \"%s\"\n", p));

            if (
                (
                    ((c = sstrcmp_codes(p, hexcodes, 0, 0x100)) != CODE_NONE) || /* 0x00-0xff */

                    ((c = sstrcmp_codes(p, ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                    ((c = sstrcmp_codes(p, a_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                    ((c = sstrcmp_codes(p, b_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                    ((c = sstrcmp_codes(p, c_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */
                    ((c = sstrcmp_codes(p, d_ctrl1, 0, 0x20)) != CODE_NONE) || /* 0x00-0x1f */

                    ((((c = sstrcmp_codes(p, cbmchars, 0, 0x20)) != CODE_NONE) || /* 0x20-0x3f */
                      ((c = sstrcmp_codes(p, a_cbmchars, 0, 0x20)) != CODE_NONE) /* 0x20-0x3f */
                        ) && (c += 0x20)) ||

                    ((((c = sstrcmp_codes(p, ctrl2, 0, 0x20)) != CODE_NONE) ||
                      ((c = sstrcmp_codes(p, a_ctrl2, 0, 0x20)) != CODE_NONE) ||
                      ((c = sstrcmp_codes(p, b_ctrl2, 0, 0x20)) != CODE_NONE) ||
                      ((c = sstrcmp_codes(p, c_ctrl2, 0, 0x20)) != CODE_NONE)
                     ) && (c += 0x80)) ||

                    ((((c = sstrcmp_codes(p, cbmkeys, 0, 0x40)) != CODE_NONE) ||
                        ((c = sstrcmp_codes(p, a_cbmkeys, 0, 0x40)) != CODE_NONE)
                        ) && (c += 0xA0))


                )
                ) {

                DBG(("asc_2_pet controlcode test 2: '%c' '%s' '%d'\n", p[kwlen], p, kwlen));

                if (p[kwlen] == '*') {
                    /* repetition count */
                    p += (kwlen);

                    DBG(("asc_2_pet controlcode test rpt: %s\n", p));

                    len = 1;
                    if (scan_integer((char *)++p, &len, &kwlen) > 0) {
                        p += kwlen;
                        DBG(("asc_2_pet controlcode repeat count: len:%d kwlen:%d\n", len, kwlen));
                        kwlen = 0;
                    }
                }

                DBG(("asc_2_pet controlcode test 3: '%c' '%s' '%d'\n", p[0], p, kwlen));

                if (p[kwlen] == CLARIF_RP) {
                    for (; len-- > 0; ) {
                        fputc(c, dest);
                    }
                    DBG(("asc_2_pet controlcode continue\n"));

                    fseek(source, pos + kwlen + 1, SEEK_SET);
                    continue;
                }
            }

            fprintf(stderr, "error: unknown control code: %s\n", p);
            exit(-1);
        }

        DBG(("asc_2_pet convert character (%02x)\n", c));

        /* convert character */
        d = _a_topetscii(c, ctrls);
        fputc(d, dest);
    }
}

/*
     look up a controlcode

     sets kwlen
*/
static int sstrcmp_codes(unsigned char *line, const char **wordlist, int token, int maxitems)
{
    int j = 0;
    const char *p, *q;

    kwlen = 1;
    /* search for keyword */
    for (; token < maxitems; token++) {
        DBG(("compare '%s' vs  '%s' - %d %d\n", wordlist[token], line, j, kwlen));

        if (codesnocase) {
            for (p = wordlist[token], q = (char *)line, j = 0;
                 *p && *q && (util_tolower(*p) == util_tolower(*q));
                 p++, q++, j++) {}
        } else {
            for (p = wordlist[token], q = (char *)line, j = 0;
                 *p && *q && (*p == *q);
                 p++, q++, j++) {}
        }

        /* found a control code */
        if (j && (!*p) && ((*q == '}') || (*q == '*'))) {
            kwlen = j;
            DBG(("found '%s' %2x\n", wordlist[token], token));
            return token;
        }
    } /* for */ 

    return (CODE_NONE);
}

/*
     look up a keyword

     sets kwlen
*/
static unsigned char sstrcmp(unsigned char *line, const char **wordlist, int token, int maxitems)
{
    int j;
    const char *p, *q;

    kwlen = 1;
    /* search for keyword */
    for (; token < maxitems; token++) {
        for (p = wordlist[token], q = (char *)line, j = 0;
             *p && *q && *p == *q; p++, q++, j++) {}

        /* DBG(("compare %s %s - %d %d\n", wordlist[token], line, j, kwlen));*/

        /* found an exact or abbreviated keyword */
        if (j && (!*p || (*p && (*p ^ *q) == 0x20 && j++))) {
            kwlen = j;
            /* DBG(("found %s %2x\n", wordlist[token], token));*/
            return token;
        }
    } /* for */

    return (KW_NONE);
}

/* ------------------------------------------------------------------------- */
/* dummy functions

   FIXME: these really shouldnt be needed here and are a sign of bad modular
          design elsewhere
 */
const char machine_name[] = "PETCAT";
const char *machine_get_name(void)
{
    return machine_name;
}

int cmdline_register_options(const cmdline_option_t *c)
{
    return 0;
}

int network_connected(void)
{
    return 0;
}

int network_get_mode(void)
{
    return NETWORK_IDLE;
}

void network_event_record(unsigned int type, void *data, unsigned int size)
{
}

void event_record_in_list(event_list_state_t *list, unsigned int type, void *data, unsigned int size)
{
}

void archdep_ui_init(int argc, char *argv[])
{
}

void ui_error_string(const char *text) /* win32 needs this */
{
}

void ui_error(const char *format, ...) /* SDL on mingw32 needs this */
{
}
