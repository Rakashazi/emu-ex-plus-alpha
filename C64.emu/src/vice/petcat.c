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

#define PETCATVERSION   2.20
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

/* Limits */

#define NUM_B_1         75      /* Basic 1.0 */

#define NUM_COMM        76      /* common for all versions 2.0 ... 10.0 */
#define NUM_SECC        18      /* VIC-20 extension */
#define NUM_TUCC        34      /* VIC-20 Turtle Basic extension */

#define NUM_V4CC        15      /* PET 4.0 */
#define NUM_4ECC        24      /* 4.0 extension (C64) */
#define NUM_VIC4        20      /* 4.0 extension (VIC20) */
#define NUM_VIC5        38      /* 5.0 extension (VIC20) */
#define NUM_SPECC       27      /* Speech Basic */
#define NUM_ATBCC       43      /* Atbasic */

#define NUM_FC3CC       29      /* Final Cartridge III */

#define NUM_KWCE        11
#define NUM_V7FE        39
#define NUM_V71FE       56
#define NUM_V10FE       62
#define NUM_SXCFE       32

#define NUM_ULTRCC      51      /* Ultrabasic-64 */
#define NUM_GRAPHCC     51      /* graphics basic (c64) */
#define NUM_WSCC        51      /* WS basic (c64) */
#define NUM_MIGHTYCC    51      /* Mighty basic (vic20) */
#define NUM_PEGCC       33      /* Pegasus basic 4.0 (c64) */
#define NUM_XCC         33      /* Xbasic (c64) */
#define NUM_DRAGOCC     13      /* Drago basic 2.2 (c64) */
#define NUM_REUCC       14      /* REU-basic (c64) */
#define NUM_BASLCC      51      /* Basic Lightning (c64) */
#define NUM_MAGICCC     50      /* Magic Basic (c64) */
#define NUM_EASYCC      51      /* Easy Basic (vic20) */
#define NUM_WSFCC       51      /* WS basic final (c64) */
#define NUM_GBCC        29
#define NUM_BSXCC       31      /* Basex (c64) */
#define NUM_EXPBAS64CC  42      /* Expanded Basic (c64) */
#define NUM_EXPBAS20CC  24      /* Expanded Basic (vic20) */

#define NUM_BLARGE0     11      /* Blarg (c64) */

#define NUM_SUPERBASDB  36      /* Superbasic (c64) */
#define NUM_WARSAWDB    36      /* Warsaw Basic (c64) */

#define NUM_SUPERGRACC  50      /* Supergrafik 64 (c64) */

#define MAX_COMM        0xCB    /* common for all versions */
#define MAX_SECC        0xDD    /* VIC-20 extension */
#define MAX_TUCC        0xED    /* VIC-20 Turtle Basic extension */

#define MAX_V4CC        0xDA    /* PET 4.0 */
#define MAX_4ECC        0xE3    /* 4.0 extension (C64) */
#define MAX_VIC4        0xDF    /* 4.0 extension (VIC20) */
#define MAX_VIC5        0xF1    /* 5.0 extension (VIC20) */
#define MAX_SPECC       0xE6    /* Speech Basic */
#define MAX_ATBCC       0xF6    /* Atbasic */

#define MAX_FC3CC       0xE8    /* Final Cartridge III */

#define MAX_ULTRCC      0xFE    /* Ultrabasic-64 */
#define MAX_GRAPHCC     0xFE    /* graphics basic (c64) */
#define MAX_WSCC        0xFE    /* WS basic (c64) */
#define MAX_MIGHTYCC    0xFE    /* Mighty basic (vic20) */
#define MAX_PEGCC       0xEC    /* Pegasus basic 4.0 (c64) */
#define MAX_XCC         0xEC    /* Xbasic (c64) */
#define MAX_DRAGOCC     0xD8    /* Drago basic 2.2 (c64) */
#define MAX_REUCC       0xDA    /* REU-basic (c64) */
#define MAX_BASLCC      0xFE    /* Basic Lightning (c64) */
#define MAX_MAGICCC     0xFD    /* Magic Basic (c64) */
#define MAX_EASYCC      0xFE    /* Easy Basic (vic20) */
#define MAX_WSFCC       0xFE    /* WS basic final (c64) */
#define MAX_GBCC        0xE8    /* Game Basic (c64) */
#define MAX_BSXCC       0xEA    /* Basex (c64) */
#define MAX_EXPBAS64CC  0xF5    /* Expanded Basic (c64) */
#define MAX_EXPBAS20CC  0xE3    /* Expanded Basic (vic20) */

#define MAX_BLARGE0     0xEA    /* Blarg (c64) */

#define MAX_SUPERBASDB  0xFE    /* Superbasic (c64) */
#define MAX_WARSAWDB    0xFE    /* Warsaw Basic (c64) */

#define MAX_KWCE        0x0A
#define MAX_V7FE        0x26
#define MAX_V71FE       0x39
#define MAX_V10FE       0x3D
#define MAX_SXCFE       0x1F

#define MAX_SUPERGRACC  0xFE    /* Supergrafik 64 (c64) */

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

#define NUM_VERSIONS  35

const char *VersNames[] = {
    "Basic 1.0",
    "Basic 2.0",
    "Basic 2.0 with Super Expander",
    "Basic 2.0 with Turtle Basic v1.0",

    "Basic 2.0 with Simon's Basic",
    "Basic 2.0 with Speech Basic v2.7",
    "Basic 2.0 with @Basic",

    "Basic 4.0",
    "Basic 4.0 extension for C64",
    "Basic 3.5",
    "Basic 7.0",
    "Basic 10.0",

    "Basic 2.0 with Final Cartridge III",
    "Basic 2.0 with Ultrabasic-64",
    "Basic 2.0 with graphics basic",
    "Basic 2.0 with WS basic",
    "Basic 2.0 with Mighty basic",
    "Basic 2.0 with Pegasus basic 4.0",
    "Basic 2.0 with Xbasic",
    "Basic 2.0 with Drago basic 2.2",
    "Basic 2.0 with REU-basic",
    "Basic 2.0 with Basic Lightning",

    "Basic 7.1 extension",

    "Basic 2.0 with Magic Basic",
    "Basic 2.0 with Easy Basic",
    "Basic 2.0 with Blarg",
    "Basic 4.0 extension for VIC20",
    "Basic 5.0 extension for VIC20",
    "Basic 2.0 with WS basic final",
    "Basic 2.0 with Game Basic",
    "Basic 2.0 with Basex",
    "Basic 2.0 with super basic",
    "Basic 2.0 with expanded basic 20"
    "Basic 2.0 with expanded basic 64",
    "Basic 2.0 with super expander chip",
    "Basic 2.0 with Warsaw Basic",
    "Basic 2.0 with Supergrafik 64"
    ""
};

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

const char *superexpkwcc[] = { /* VIC Super Expander commands 0xcc - 0xdd */
    "key",   "graphic", "scnclr", "circle", "draw", "region", "color", "point",
    "sound", "char",    "paint",  "rpot",   "rpen", "rsnd",   "rcolr", "rgr",
    "rjoy",  "rdot"
};


const char *petkwcc[] = {       /* PET Basic 4.0 0xcc - 0xda */
    "concat", "dopen", "dclose", "record",  "header", "collect", "backup",    "copy",
    "append", "dsave", "dload",  "catalog", "rename", "scratch", "directory",

    /* Basic 4 Extension for C64 (0xdb - 0xe3) */
    "color",  "cold", "key", "dverify", "delete", "auto", "merge", "old",
    "monitor"
};


/* ------------------------------------------------------------------------- */

/*
 * Third party products for VIC-20
 */

/* Turtle Basic v1.0 Keywords  -- Craig Bruce */

const char *turtlekwcc[] = {
    "graphic", "old",    "turn",   "pen",    "draw",  "move", "point", "kill",
    "write",   "repeat", "screen", "doke",   "reloc", "fill", "rtime", "base",
    "pause",   "pop",    "color",  "merge",  "char",  "take", "sound", "vol",
    "put",     "place",  "cls",    "accept", "reset", "grab", "rdot",  "plr$",
    "deek",    "joy"
};


/* Easy Basic Keywords */

const char *easykwcc[] = {
    "delete", "old",    "renumber", "dump",    "merge",  "plot",   "trace",     "kill",
    "help",   "dload",  "dsave",    "dverify", "append", "screen", "directory", "key",
    "send",   "pop",    "off",      "pout",    "header", "find",   "auto",      "pprint",
    "accept", "reset",  "scratch",  "color",   "take",   "pause",  "base",      "copychr",
    "char",   "clk",    "cls",      "fill",    "retime", "sound",  "poff",      "plist",
    "put",    "volume", "joy",      "msb",     "lsb",    "vector", "rkey",      "dec",
    "hex$",   "grab",   "ds$"
};


/* Mighty basic Keywords */

const char *mightykwcc[] = {
    "delete", "old",    "renumber", "help",    "header",  "move",    "trace",     "kill",
    "dump",   "dsave",  "dload",    "dverify", "dresave", "scratch", "directory", "key",
    "send",   "pop",    "off",      "bsave",   "bload",   "find",    "auto",      "pprint",
    "accept", "reset",  "else",     "color",   "take",    "pause",   "base",      "copychr",
    "char",   "beep",   "cls",      "fill",    "merge",   "sound",   "give",      "plist",
    "put",    "volume", "rtime",    "msb",     "lsb",     "vector",  "joy",       "dec",
    "hex$",   "grab",   "ds$"
};


/* Basic 4.0 extension Keywords */

const char *vic4kwcc[] = {
    "concat", "dopen",    "dclose",  "record",  "header", "collect", "backup",    "copy",
    "append", "dsave",    "dload",   "catalog", "rename", "scratch", "directory", "ieee",
    "serial", "parallel", "monitor", "modem"
};


/* Basic 5.0 extension Keywords */

const char *vic5kwcc[] = {
    "concat",  "dopen",  "dclose", "record",   "header", "collect", "backup",    "copy",
    "append",  "dsave",  "dload",  "catalog",  "rename", "scratch", "directory", "dverify",
    "monitor", "repeat", "bell",   "commands", "renew",  "`",       "key",       "auto",
    "off",     "",       "merge",  "color",    "mem",    "enter",   "delete",    "find",
    "number",  "else",   "call",   "graphic",  "alpha",  "dmerge"
};


/*
 * Third party products for C=64
 */


/* Speech Basic v2.7  Keywords (Tokens CC - E6) */

const char *speechkwcc[] = {
    "reset",  "basic",  "help", "key",   "himem",  "disk", "dir",    "bload",
    "bsave",  "map",    "mem",  "pause", "block",  "hear", "record", "play",
    "voldef", "coldef", "hex",  "dez",   "screen", "exec", "mon",    "<-",
    "from",   "speed",  "off"
};


/* Blarg Keywords (Tokens E0 - EA) -- Marco van den Heuvel */

const char *blargkwe0[] = {
    "plot",   "line", "circle", "gron", "groff", "mode", "origin", "clear",
    "buffer", "swap", "color"
};


/* Super Basic Keywords (Tokens DB - FE) -- Marco van den Heuvel */

const char *superbaskwdb[] = {
    "volume",  "reset",  "mem",    "trace",   "basic",  "resume", "letter",   "help",
    "coke",    "ground", "matrix", "dispose", "print@", "himem",  "hardcopy", "inputform",
    "lock",    "swap",   "using",  "sec",     "else",   "error",  "round",    "deek",
    "string$", "point",  "instr",  "ceek",    "min",    "max",    "varptr",   "frac",
    "odd",     "dec",    "hex$",   "eval"
};


/* Warsaw Basic Keywords (TOKENS DB - FE) -- Marco van den Heuvel */

const char *warsawkwdb[] = {
    "hisave",  "sline",  "mem",    "trace",   "beep",   "resume", "letter", "help",
    "*****",   "ground", "revers", "dispose", "print@", "himem",  "*****",  "line",
    "proc",    "axis",   "using",  "sec",     "else",   "error",  "round",  "****",
    "*******", "*****",  "*****",  "pound",   "min",    "max",    "******", "frac",
    "odd",     "***",    "heek",   "eval"
};


/* @Basic (Atbasic) Keywords (Tokens CC - F6) -- Andre Fachat */

const char *atbasickwcc[] = {
    "trace",  "delete",  "auto",      "old",     "dump",    "find",    "renumber", "dload",
    "dsave",  "dverify", "directory", "catalog", "scratch", "collect", "rename",   "copy",
    "backup", "disk",    "header",    "append",  "merge",   "mload",   "mverify",  "msave",
    "key",    "basic",   "reset",     "exit",    "enter",   "doke",    "set",      "help",
    "screen", "lomem",   "himem",     "colour",  "type",    "time",    "deek",     "hex$",
    "bin$",   "off",     "alarm"
};


/* Ultrabasic-64 Keywords (Tokens CC - FE) -- Marco van den Heuvel */

const char *ultrabasic64kwcc[] = {
    "dot",    "draw",  "box",    "tic",    "copy", "sprite", "off",   "mode",
    "norm",   "graph", "dump",   "gread",  "char", "place",  "multi", "hires",
    "hex",    "bit",   "colors", "pixel",  "fill", "circle", "block", "sdata",
    "vol",    "gen",   "scoll",  "bcoll",  "joy",  "paddle", "pen",   "sound",
    "tune",   "tdata", "set",    "turnto", "turn", "tup",    "tdown", "tcolor",
    "turtle", "move",  "bye",    "rotate", "tpos", "ctr",    "sctr",  "[",
    "]",      "hard",  "exit"
};


/* Graphics basic (c64) Keywords (Tokens CC - FE) -- Marco van den Heuvel */

const char *graphicsbasickwcc[] = {
    "background", "border",  "dir",    "disk", "fill",   "key",    "circle", "procedure",
    "dot",        "find",    "change", "ren",  "else",   "copy",   "scroll", "roll",
    "box",        "scale",   "do",     "line", "sprite", "color",  "hires",  "clear",
    "text",       "window",  "off",    "at",   "shape",  "xysize", "speed",  "from",
    "setorigin",  "animate", "multi",  "eze",  "move",   "under",  "edit",   "reset",
    "xpos",       "gprint",  "voice",  "adsr", "wave",   "ne",     "volume", "play",
    "ypos",       "sound",   "joy"
};


/* WS (WohnzimmerSoft) basic (c64) Keywords (Tokens CC - FE) -- Marco van den Heuvel */

const char *wsbasickwcc[] = {
    "copy",  "old",    "port",    "doke",   "vpoke",  "fill",   "error", "send",
    "call",  "bit",    "dir",     "bload",  "bsave",  "find",   "speed", "pitch",
    "say",   "fast",   "slow",    "talk",   "shutup", "stash",  "fetch", "swap",
    "off",   "screen", "device",  "object", "vstash", "vfetch", "quiet", "color",
    "cls",   "curpos", "monitor", "subend", "do",     "loop",   "exit",  "deek",
    "rsc",   "rsm",    "dec",     "hex$",   "hi",     "lo",     "ds$",   "line",
    "vpeek", "row",    "joy"
};


/* WS (WohnzimmerSoft) basic final (c64) Keywords (Tokens CC - FE) -- Marco van den Heuvel */

const char *wsfbasickwcc[] = {
    "copy", "bank",   "old",     "doke",   "display", "fill",   "error", "send",
    "call", "bit",    "dir",     "bload",  "bsave",   "find",   "speed", "pitch",
    "say",  "fast",   "slow",    "talk",   "shutup",  "stash",  "fetch", "swap",
    "off",  "mode",   "device",  "object", "vstash",  "vfetch", "latch", "color",
    "cls",  "curpos", "monitor", "subend", "do",      "loop",   "exit",  "deek",
    "col",  "rsm",    "dec",     "hex$",   "hi",      "lo",     "ds$",   "line",
    "bnk",  "ypos",   "joy"
};


/* Pegasus basic 4.0 (c64) Keywords (Tokens CC - EC) -- Marco van den Heuvel */

const char *pegbasickwcc[] = {
    "off",      "asc(",     "sin(",    "cos(",  "tan(",     "atn(",     "deg(",     "rad(",
    "frac(",    "mod(",     "round(",  "dec(",  "bin(",     "deek(",    "instr(",   "joy(",
    "pot(",     "screen(",  "test(",   "using", "ds$",      "hex$(",    "bin$(",    "space$(",
    "ucase$(",  "string$(", "input$(", "time$", "spritex(", "spritey(", "turtlex(", "turtley(",
    "turtleang"
};


/* Xbasic (c64) Keywords (Tokens CC - EC) -- Marco van den Heuvel */

const char *xbasickwcc[] = {
    "sprat",   "brdr",   "screen",   "quit", "sprmult", "move",  "sprite", "asprite",
    "dsprite", "sid",    "envelope", "gate", "frq",     "wave",  "vol",    "fcut",
    "fmode",   "filter", "frsn",     "cset", "multi",   "extnd", "locate", "center",
    "hires",   "line",   "hprnt",    "plot", "text",    "clear", "colr",   "stick",
    "btn"
};


/* Drago basic 2.2 (c64) Keywords (Tokens CC - D8) -- Marco van den Heuvel */

const char *dragobasickwcc[] = {
    "punkt",   "linia",  "rysuj", "param",  "kuntur", "anim", "kolor", "puwid",
    "ryselip", "koguma", "fiut",  "figura", "figuma"
};


/* REU-basic (c64) Keywords (Tokens CC - DA) -- Marco van den Heuvel */

const char *reubasickwcc[] = {
    "push", "pull", "flip", "rec",  "stash", "fetch", "swap", "reu",
    "size", "dir",  "@",    "kill", "rom",   "ram",   "move"
};


/* Basic Lightning (c64) Keywords (Tokens CC - FE) -- Marco van den Heuvel */

const char *baslkwcc[] = {
    "else",  "hex$",  "deek",  "true",    "import",  "cfn",      "size",    "false",
    "ver$",  "lpx",   "lpy",   "common%", "crow",    "ccol",     "atr",     "inc",
    "num",   "row2",  "col2",  "spn2",    "hgt",     "wid",      "row",     "col",
    "spn",   "task",  "halt",  "repeat",  "until",   "while",    "wend",    "cif",
    "celse", "cend",  "label", "doke",    "exit",    "allocate", "disable", "pull",
    "dload", "dsave", "var",   "local",   "procend", "proc",     "casend",  "of",
    "case",  "rpt",   "setatr"
};


/* Magic Basic (c64) Keywords (Tokens CC - FD) -- Marco van den Heuvel */

const char *magickwcc[] = {
    "assembler", "auto",    "cdrive",  "cat",    "dappend", "delete", "dez",    "dir",
    "dload",     "dsave",   "dverify", "config", "find",    " ",      " ",      "help",
    "hex",       "jump",    "llist",   "lprint", "off",     "old",    "renum",  "crun",
    "send",      "status",  "hires",   "multi",  "clear",   "plot",   "invert", "line",
    "text",      "graphik", "page",    "box",    "draw",    "mix",    "copy",   "circle",
    "gsave",     "gload",   "frame",   "hprint", "vprint",  "block",  "fill",   " ",
    "replace",   "lrun"
};


/* Game Basic (c64) Keywords (Tokens CC - E8) -- Marco van den Heuvel */

const char *gbkwcc[] = {
    "window", "bfile",   "upper",   "lower",  "cls",  "screen", "parse",   "proc",
    "else",   "scratch", "replace", "device", "dir",  "repeat", "until",   "disk",
    "fetch#", "put#",    "prompt",  "pop",    "help", "exit",   "disable", "enter",
    "reset",  "warm",    "num",     "type",   "text$"
};


/* Basex (c64) Keywords (Tokens CC - EA) -- Marco van den Heuvel */

const char *bsxkwcc[] = {
    "append", "auto",  "bar",   "circle",   "clg",  "cls",      "csr",    "delete",
    "disk",   "draw",  "edge",  "envelope", "fill", "key",      "mob",    "mode",
    "move",   "old",   "pic",   "dump",     "plot", "renumber", "repeat", "scroll",
    "sound",  "while", "until", "voice",    "ass",  "dis",      "mem"
};

/* Expanded Basic (c64) Keywords (Tokens CC - F5) -- Marco van den Heuvel */

const char *expbas64kwcc[] = {
    "hires",    "norm",     "graph",   "set",        "line",     "circle",   "fill",   "mode",
    "cls",      "text",     "color",   "gsave",      "gload",    "inverse",  "frame",  "move",
    "using",    "renumber", "delete",  "box",        "mobdef",   "sprite",   "mobset", "modsize",
    "mobcolor", "mobmulti", "mobmove", "doke",       "allclose", "old",      "auto",   "volume",
    "envelope", "wave",     "play",    "case error", "resume",   "no error", "find",   "inkey",
    "merge",    "hardcopy"
};

/* Expanded Basic (vic20) Keywords (Tokens CC - E3) -- Marco van den Heuvel */

const char *expbas20kwcc[] = {
    "reset", "sound", "slow(", "com",   "mem",    "stat(", "key",   "off",
    "col(",  "plot(", "pop(",  "chol(", "curol(", "beep(", "paus(", "msav",
    "reg(",  "dpek(", "pdl",   "joy",   "dpok",   "do",    "until", "old"
};

/* Simon's Basic Keywords */

const char *simonskw[] = {
    "",        "hires",    "plot",   "line",     "block",    "fchr",    "fcol",     "fill",
    "rec",     "rot",      "draw",   "char",     "hi col",   "inv",     "frac",     "move",
    "place",   "upb",      "upw",    "leftw",    "leftb",    "downb",   "downw",    "rightb",
    "rightw",  "multi",    "colour", "mmob",     "bflash",   "mob set", "music",    "flash",
    "repeat",  "play",     ">>",     "centre",   "envelope", "cgoto",   "wave",     "fetch",
    "at(",     "until",    ">>",     ">>",       "use",      ">>",      "global",   ">>",
    "reset",   "proc",     "call",   "exec",     "end proc", "exit",    "end loop", "on key",
    "disable", "resume",   "loop",   "delay",    ">>",       ">>",      ">>",       ">>",
    "secure",  "disapa",   "circle", "on error", "no error", "local",   "rcomp",    "else",
    "retrace", "trace",    "dir",    "page",     "dump",     "find",    "option",   "auto",
    "old",     "joy",      "mod",    "div",      ">>",       "dup",     "inkey",    "inst",
    "test",    "lin",      "exor",   "insert",   "pot",      "penx",    ">>",       "peny",
    "sound",   "graphics", "design", "rlocmob",  "cmob",     "bckgnds", "pause",    "nrm",
    "mob off", "off",      "angl",   "arc",      "cold",     "scrsv",   "scrld",    "text",
    "cset",    "vol",      "disk",   "hrdcpy",   "key",      "paint",   "low col",  "copy",
    "merge",   "renumber", "mem",    "detect",   "check",    "display", "err",      "out"
};


/* Final Cartridge III Keywords -- Matti 'ccr' Hamalainen */

const char *fc3kw[] = {
    "off",     "auto",  "del",     "renum",   "?ERROR?", "find", "old",   "dload",
    "dverify", "dsave", "append",  "dappend", "dos",     "kill", "mon",   "pdir",
    "plist",   "bar",   "desktop", "dump",    "array",   "mem",  "trace", "replace",
    "order",   "pack",  "unpack",  "mread",   "mwrite"
};


/* Super Expander Chip extension */

const char *sxckwfe[] = {
    "key",   "color",  "graphic", "scnclr", "locate", "scale",  "box",    "circle",
    "char",  "draw",   "gshape",  "paint",  "sshape", "tune",   "filter", "sprdef",
    "tempo", "movspr", "sprcol",  "sprite", "colint", "sprsav", "rbump",  "rclr",
    "rdot",  "rgr",    "rjoy",    "rpen",   "rpot",   "rspcol", "rsppos", "rspr"
};


/* Data Becker Supergrafik 64 (C64) Keywords (Tokens CC - FD) -- Sven Droll */

const char *supergrakw[] = {
    " ",      " ",      "   ",   " ",         " ",      " ",     " ",       " ",
    " ",      " ",      " ",     "directory", "spower", "gcomb", "dtaset",  "merge",
    "renum",  "key",    "trans", " ",         "tune",   "sound", "volume=", "filter",
    "sread",  "define", "set",   "swait",     "smode",  "gmode", "gclear",  "gmove",
    "plot",   "draw",   "fill",  "frame",     "invers", "text",  "circle",  "paddle",
    "scale=", "color=", "scol=", "pcol=",     "gsave",  "gload", "hcopy",   "ireturn",
    "if#",    "paint"
};


/* ------------------------------------------------------------------------- */

static void usage(char *progname);
static int parse_version(char *str);
static void list_keywords(int version);
static void pet_2_asc (int ctrls);
static void asc_2_pet (int ctrls);
static void _p_toascii(int c, int ctrls);
static int p_expand(int version, int addr, int ctrls);
static void p_tokenize(int version, unsigned int addr, int ctrls);
static unsigned char sstrcmp(unsigned char *line, const char **wordlist, int token, int maxitems);
static int sstrcmp_codes(unsigned char *line, const char **wordlist, int token, int maxitems);

/* ------------------------------------------------------------------------- */

static FILE *source, *dest;
static unsigned int kwlen = 0;
static int codesnocase = 0; /* flag, =1 if controlcodes should be interpreted case insensitive */
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
        switch (version) {
            case B_PEG:
            case B_SUPEREXP:
            case B_BSX:
                load_addr = 0x0401;
                break;
            case B_TURTLE:
                load_addr = 0x3701;
                break;
            case B_GRAPH:
            case B_35:
                load_addr = 0x1001;
                break;
            case B_71:
            case B_7:
                load_addr = 0x1c01;
                break;
            case B_10:
                load_addr = 0x2001;
                break;
            case B_ULTRA:
                load_addr = 0x2c01;
                break;
            case B_EASY:
                load_addr = 0x3001;
                break;
            case B_MIGHTY:
                load_addr = 0x3201;
                break;

            default:
                load_addr = 0x0801;
        }
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

    fprintf(stdout, "\n\tVersions:\n"
            "\t1\tPET Basic V1.0\n"
            "\t2\tBasic v2.0\n"
            "\tsuperexp\tBasic v2.0 with Super Expander (VIC20)\n"
            "\tturtle\tBasic v2.0 with Turtle Basic by Craig Bruce (VIC20)\n"
            "\tmighty\tBasic v2.0 with Mighty Basic by Craig Bruce (VIC20)\n"
            "\ta\tBasic v2.0 with AtBasic (C64)\n"
            "\tsimon\tBasic v2.0 with Simon's Basic extension (C64)\n"
            "\tspeech\tBasic v2.0 with Speech Basic v2.7 (C64)\n"
            "\tF\tBasic v2.0 with Final Cartridge III (C64)\n"
            "\tultra\tBasic v2.0 with Ultrabasic-64 (C64)\n"
            "\tgraph\tBasic v2.0 with Graphics basic (C64)\n"
            "\tWSB\tBasic v2.0 with WS basic (C64)\n"
            "\tWSBF\tBasic v2.0 with WS basic final (C64)\n"
            "\tPegasus\tBasic v2.0 with Pegasus basic 4.0 (C64)\n"
            "\tXbasic\tBasic v2.0 with Xbasic (C64)\n"
            "\tDrago\tBasic v2.0 with Drago basic 2.2 (C64)\n"
            "\tREU\tBasic v2.0 with REU-basic (C64)\n"
            "\tLightning\tBasic v2.0 with Basic Lightning (C64)\n"
            "\tmagic\tBasic v2.0 with Magic Basic (C64)\n"
            "\teasy\tBasic v2.0 with Easy Basic (VIC20)\n"
            "\tblarg\tBasic v2.0 with Blarg (C64)\n"
            "\tGame\tBasic v2.0 with Game Basic (C64)\n"
            "\tBSX\tBasic v2.0 with Basex (C64)\n"
            "\tsuperbas\tBasic v2.0 with Super Basic (C64)\n"
            "\texp20\tBasic 2.0 with Expanded Basic (VIC20)\n"
            "\texp64\tBasic 2.0 with Expanded Basic (C64)\n"
            "\tsxc\tBasic 2.0 with Super Expander Chip (C64)\n"
            "\twarsaw\tBasic 2.0 with Warsaw Basic (C64)\n"
            "\t4v\tBasic 2.0 with Basic 4.0 extensions (VIC20)\n"
            "\t4 -w4e\tPET Basic v4.0 program (PET/C64)\n"
            "\t5\tBasic 2.0 with Basic 5.0 extensions (VIC20)\n"
            "\t3\tBasic v3.5 program (C16)\n"
            "\t70\tBasic v7.0 program (C128)\n"
            "\t71\tBasic v7.1 program (C128)\n"
            "\t10\tBasic v10.0 program (C64DX)\n"
            "\tsupergra\tBasic v2.0 with Supergrafik 64 (C64)\n\n");

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
    int version = -1;

    if (str == NULL || !*str) {
        return 0;
    }

    switch (util_toupper(*str)) {
        case '1':         /* Basic 1.0 and Basic 10.0 */
            if (str[1] == '0') {
                version = B_10;
            } else if (!str[1]) {
                version = B_1;
            }
            break;

        case '2':
            version = B_2;
            break;

        case '3':
            version = B_35; /* 3.5 */
            break;

        case '4':
            version = ((util_toupper(str[1]) == 'E') ? B_4E : ((util_toupper(str[1]) == 'v') ? B_VIC4 : B_4)); /* Basic 4.0 */
            break;

        case '5':
            version = B_VIC5; /* 5.0 */
            break;

        case '7':
            switch (str[1]) {
                case '0':
                    version = B_7;
                    break;
                case '1':
                    version = B_71;
                    break;
                default:
                    fprintf(stderr, "Please, select one of the following: 70, 71\n");
            }
            break;

        case 'A':
            version = B_ATBAS;
            break;

        case 'B':
            switch (util_toupper(str[1])) {
                case 'L':
                    version = B_BLARG;
                    break;
                case 'S':
                    version = B_BSX;
                    break;
                default:
                    fprintf(stderr, "Please, select one of the following: blarg, bsx\n");
            }
            break;

        case 'D':
            version = B_DRAGO;
            break;

        case 'E':
            switch (util_toupper(str[1])) {
                case 'A':
                    version = B_EASY;
                    break;
                case 'X':
                    if (util_toupper(str[2]) != 'P') {
                        fprintf(stderr, "Please, select one of the following: exp20, exp64\n");
                    } else {
                        switch (util_toupper(str[3])) {
                            case '2':
                                version = B_EXPBAS20;
                                break;
                            case '6':
                                version = B_EXPBAS64;
                                break;
                            default:
                                fprintf(stderr, "Please, select one of the following: exp20, exp64\n");
                        }
                    }
                    break;
            }
            break;

        case 'F':
            version = B_FC3;
            break;

        case 'G':
            switch (util_toupper(str[1])) {
                case 'R':
                    version = B_GRAPH;
                    break;
                case 'A':
                    version = B_GB;
                    break;
                default:
                    fprintf (stderr, "Please, select one of the following: graphics, game\n");
            }
            break;

        case 'L':
            version = B_BASL;
            break;

        case 'M':
            switch (util_toupper(str[1])) {
                case 'I':
                    version = B_MIGHTY;
                    break;
                case 'A':
                    version = B_MAGIC;
                    break;
                default:
                    fprintf (stderr, "Please, select one of the following: magic, mighty\n");
            }
            break;

        case 'P':
            version = B_PEG;
            break;

        case 'R':
            version = B_REU;
            break;

        case 'S':
            switch (util_toupper(str[1])) {
                case 'U':
                    if (util_toupper(str[2]) != 'P' || util_toupper(str[3]) != 'E' || util_toupper(str[4]) != 'R') {
                        fprintf(stderr, "Please, select one of the following: superbas, superexp, supergra\n");
                    } else {
                        switch (util_toupper(str[5])) {
                            case 'B':
                                version = B_SUPERBAS;
                                break;
                            case 'E':
                                version = B_SUPEREXP;
                                break;
                            case 'G':
                                version = B_SUPERGRA;
                                break;
                            default:
                                fprintf(stderr, "Please, select one of the following: superbas, superexp, supergra\n");
                        }
                    }
                    break;
                case 'B':
                    version = B_SUPERBAS;
                    break;
                case 'E':
                    version = B_SUPEREXP;
                    break;
                case 'I':
                    version = B_SIMON;
                    break;
                case 'P':
                    version = B_SPEECH;
                    break;
                case 'X':
                    version = B_SXC;
                    break;
                default:
                    fprintf (stderr, "Please, select one of the following: superbas, superexp, simon, speech, sxc\n");
            }
            break;

        case 'T':
            version = B_TURTLE;
            break;

        case 'U':
            version = B_ULTRA;
            break;

        case 'W':
            switch (util_toupper(str[1])) {
                case 'A':
                    version = B_WARSAW;
                    break;
                case 'S':
                    if (util_toupper(str[2]) != 'B') {
                        fprintf(stderr, "Please, select one of the following: WSB, WSBF\n");
                    } else {
                        if (util_toupper(str[3]) == 'F') {
                            version = B_WSF;
                        } else {
                            version = B_WS;
                        }
                    }
                    break;
                default:
                    fprintf(stderr, "Please, select one of the following: Warsaw, WSB, WSBF\n");
                    break;
            }

        case 'X':
            version = B_X;
            break;

        default:
            fprintf (stderr, "\nUnimplemented version '%s'\n", str);
            version = -1;
    }

    return (version);
}

static void list_keywords(int version)
{
    int n, max;

    if (version <= 0 || version > NUM_VERSIONS) {
        printf("\n  The following versions are supported on  %s V%4.2f\n\n", "petcat", (float)PETCATVERSION );

        for (n = 0; n < NUM_VERSIONS; n++) {
            printf("\t%s\n", VersNames[n]);
        }
        printf("\n");
        return;
    }

    printf("\n  Available Keywords on %s\n\n", VersNames[version - 1]);

    if (version == B_1) {
        max = NUM_B_1;
    } else if (version == B_35 || version == B_7 || version == B_71 || version == B_10) {
        max = 0x7E;
    } else {
        max = NUM_COMM;
    }

    for (n = 0; n < max; n++) {
        if (version == B_35 || n != 0x4e) {      /* Skip prefix marker */
            printf("%s\t", keyword[n] /*, n | 0x80*/);
        }
    }
    printf("%s\n", keyword[127]);


    if (version == B_7 || version == B_71 || version == B_10 || version == B_SXC) {
        switch (version) {
            case B_10:
                for (n = 2; n < NUM_V10FE; n++) {
                    printf("%s\t", kwfe[n] /*, 0xfe, n*/);
                }
                break;
            case B_71:
                for (n = 2; n < NUM_V71FE; n++) {
                    printf("%s\t", kwfe71[n] /*, 0xfe, n*/);
                }
                break;
            default:
            case B_7:
                for (n = 2; n < NUM_V7FE; n++) {
                    printf("%s\t", kwfe[n] /*, 0xfe, n*/);
                }
                break;
            case B_SXC:
                for (n = 0; n < NUM_SXCFE; n++) {
                    printf("%s\t", sxckwfe[n] /*, 0xfe, n*/);
                }
                break;
        }

        if (version != B_SXC) {
            for (n = 2; n < NUM_KWCE; n++) {
                printf("%s\t", kwce[n] /*, 0xce, n*/);
            }
        }
    } else {
        switch (version) {
            case B_SUPEREXP:
                for (n = 0; n < NUM_SECC; n++) {
                    printf("%s\t", superexpkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_TURTLE:
                for (n = 0; n < NUM_TUCC; n++) {
                    printf("%s\t", turtlekwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_MIGHTY:
                for (n = 0; n < NUM_MIGHTYCC; n++) {
                    printf("%s\t", mightykwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_MAGIC:
                for (n = 0; n < NUM_MAGICCC; n++) {
                    printf("%s\t", magickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_EASY:
                for (n = 0; n < NUM_EASYCC; n++) {
                    printf("%s\t", easykwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_BLARG:
                for (n = 0; n < NUM_BLARGE0; n++) {
                    printf("%s\t", blargkwe0[n] /*, n + 0xe0*/);
                }
                break;

            case B_SUPERBAS:
                for (n = 0; n < NUM_SUPERBASDB; n++) {
                    printf("%s\t", superbaskwdb[n] /*, n + 0xdb*/);
                }
                break;

            case B_WARSAW:
                for (n = 0; n < NUM_WARSAWDB; n++) {
                    printf("%s\t", warsawkwdb[n] /*, n + 0xdb*/);
                }
                break;

            case B_EXPBAS20:
                for (n = 0; n < NUM_EXPBAS20CC; n++) {
                    printf("%s\t", expbas20kwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_EXPBAS64:
                for (n = 0; n < NUM_EXPBAS64CC; n++) {
                    printf("%s\t", expbas64kwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_GB:
                for (n = 0; n < NUM_GBCC; n++) {
                    printf("%s\t", gbkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_BSX:
                for (n = 0; n < NUM_BSXCC; n++) {
                    printf("%s\t", bsxkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_4:
            case B_4E:
                for (n = 0; n < ((version == B_4) ? NUM_V4CC : NUM_4ECC); n++) {
                    printf("%s\t", petkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_VIC4:
                for (n = 0; n < NUM_VIC4; n++) {
                    printf("%s\t", vic4kwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_VIC5:
                for (n = 0; n < NUM_VIC5; n++) {
                    printf("%s\t", vic5kwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_SIMON:
                for (n = 1; n < 0x80; n++) {
                    printf("%s\t", simonskw[n] /*, 0x64, n*/);
                }
                break;

            case B_SPEECH:
                for (n = 0; n < NUM_SPECC; n++) {
                    printf("%s\t", speechkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_ATBAS:
                for (n = 0; n < NUM_ATBCC; n++) {
                    printf("%s\t", atbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_ULTRA:
                for (n = 0; n < NUM_ULTRCC; n++) {
                    printf("%s\t", ultrabasic64kwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_GRAPH:
                for (n = 0; n < NUM_GRAPHCC; n++) {
                    printf("%s\t", graphicsbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_WS:
                for (n = 0; n < NUM_WSCC; n++) {
                    printf("%s\t", wsbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_WSF:
                for (n = 0; n < NUM_WSFCC; n++) {
                    printf("%s\t", wsfbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_PEG:
                for (n = 0; n < NUM_PEGCC; n++) {
                    printf("%s\t", pegbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_X:
                for (n = 0; n < NUM_XCC; n++) {
                    printf("%s\t", xbasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_DRAGO:
                for (n = 0; n < NUM_DRAGOCC; n++) {
                    printf("%s\t", dragobasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_REU:
                for (n = 0; n < NUM_REUCC; n++) {
                    printf("%s\t", reubasickwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_FC3:
                for (n = 0; n < NUM_FC3CC; n++) {
                    printf("%s\t", fc3kw[n] /*, n + 0xcc*/);
                }
                break;

            case B_BASL:
                for (n = 0; n < NUM_BASLCC; n++) {
                    printf("%s\t", baslkwcc[n] /*, n + 0xcc*/);
                }
                break;

            case B_SUPERGRA:
                for (n = 0; n < NUM_SUPERGRACC; n++) {
                    printf("%s\t", supergrakw[n] /*, n + 0xcc*/);
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

static void pet_2_asc(int ctrls)
{
    int c;

    while ((c = getc(source)) != EOF) {
        _p_toascii(c, ctrls);           /* convert character */
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

static void _p_toascii(int c, int ctrls)
{
    /* fprintf(stderr, "<%02x:%d>", c, ctrls); */
    switch (c) {
        case 0x00: /* 00 for SEQ */
        case 0x0a:
            if (!ctrls) {
                fputc('\n', dest);
            } else {
                out_ctrl((unsigned char)(c & 0xff));
            }
            break;
        case 0x0d: /* CBM carriage return */
            fputc('\n', dest);
            break;
        case 0x40:
            fputc('@', dest);
            break;
        case 0x5b:
            fputc('[', dest);
            break;
        case 0x5c:
            fputc('\\', dest);
            break;
        case 0x5d:
            fputc(']', dest);
            break;
        case 0x5e:
            fputc('^', dest);
            break;
        case 0x5f: /* left arrow */
            fputc('_', dest);
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
                fputc(' ', dest);
            } else {
                out_ctrl((unsigned char)(c & 0xff));
            }
            break;
        case 0xff: /* (*) PI produces the same screencode as $7e and $de! */
            fputc(0x7e, dest); /*  '~' is ASCII for 'pi' */
            break;

        default:
            switch (c & 0xe0) {
                case 0x40:                /* 41 - 5F */
                    fputc(c ^ 0x20, dest);
                    break;
                case 0x60:                /* 61 - 7F (produces same screencodes as C1...) */
                    if (ctrls) {
                        out_ctrl((unsigned char)(c & 0xff));
                    } else {
                        fputc(c ^ 0x20, dest);
                    }
                    break;
                case 0xa0:                /* (*) A1 - BF (produces same screencodes as E1...) */
                    fprintf(dest, CLARIF_LP_ST "%s" CLARIF_RP_ST, cbmkeys[c & 0x1f]);
                    break;
                case 0xe0:                /* E1 - FE (produces same screencodes as A1...) */
                    out_ctrl((unsigned char)(c & 0xff));
                    break;
                case 0xc0:                /* (*) C0 - DF (produces same screencodes as 61...) */
                    fputc(c ^ 0x80, dest);
                    break;

                default:
                    if ((c > 0x1f) && isprint(c)) {
                        fputc(c, dest);
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
                    fprintf(dest, "%s", simonskw[c]);
                    continue;
                } else {
                    fprintf(dest, "($64)");     /* it wasn't prefix */
                }
            }

            /* basic 2.0, 7.0 & 10.0 and extensions */

            if (!quote && c > 0x7f) {
                /* check for keywords common to all versions, include pi */
                if (c <= MAX_COMM || c == 0xff) {
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
                            if ((c = getc(source)) <= MAX_SXCFE) {
                                fprintf(dest, "%s", sxckwfe[c]);
                            } else {
                                fprintf(dest, "($fe%02x)", c);
                            }
                        } else {
                            if ((c = getc(source)) <= MAX_V10FE) {
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
                    case B_SUPEREXP:         /* VIC extension */
                        if (c >= 0xcc && c <= MAX_SECC) {
                            fprintf(dest, "%s", superexpkwcc[c - 0xcc]);
                        }
                        break;

                    case B_TURTLE:        /* VIC extension as well */
                        if (c >= 0xcc && c <= MAX_TUCC) {
                            fprintf(dest, "%s", turtlekwcc[c - 0xcc]);
                        }
                        break;

                    case B_MIGHTY:        /* VIC Mightyb basic */
                        if (c >= 0xcc && c <= MAX_MIGHTYCC) {
                            fprintf(dest, "%s", mightykwcc[c - 0xcc]);
                        }
                        break;

                    case B_MAGIC:
                        if (c >= 0xcc && c <= MAX_MAGICCC) {
                            fprintf(dest, "%s", magickwcc[c - 0xcc]);
                        }
                        break;

                    case B_EASY:
                        if (c >= 0xcc && c <= MAX_EASYCC) {
                            fprintf(dest, "%s", easykwcc[c - 0xcc]);
                        }
                        break;

                    case B_BLARG:
                        if (c >= 0xe0 && c <= MAX_BLARGE0) {
                            fprintf(dest, "%s", blargkwe0[c - 0xe0]);
                        }
                        break;

                    case B_SUPERBAS:
                        if (c >= 0xdb && c <= MAX_SUPERBASDB) {
                            fprintf(dest, "%s", superbaskwdb[c - 0xdb]);
                        }
                        break;

                    case B_WARSAW:
                        if (c >= 0xdb && c <= MAX_WARSAWDB) {
                            fprintf(dest, "%s", warsawkwdb[c - 0xdb]);
                        }
                        break;

                    case B_EXPBAS20:
                        if (c >= 0xcc && c <= MAX_EXPBAS20CC) {
                            fprintf(dest, "%s", expbas20kwcc[c - 0xcc]);
                        }
                        break;

                    case B_EXPBAS64:
                        if (c >= 0xcc && c <= MAX_EXPBAS64CC) {
                            fprintf(dest, "%s", expbas64kwcc[c - 0xcc]);
                        }
                        break;

                    case B_GB:
                        if (c >= 0xcc && c <= MAX_GBCC) {
                            fprintf(dest, "%s", gbkwcc[c - 0xcc]);
                        }
                        break;

                    case B_BSX:
                        if (c >= 0xcc && c <= MAX_BSXCC) {
                            fprintf(dest, "%s", bsxkwcc[c - 0xcc]);
                        }
                        break;

                    case B_SPEECH:        /* C64 Speech basic */
                        if (c >= 0xcc && c <= MAX_SPECC) {
                            fprintf(dest, "%s", speechkwcc[c - 0xcc]);
                        }
                        break;

                    case B_ATBAS:         /* C64 Atbasic */
                        if (c >= 0xcc && c <= MAX_ATBCC) {
                            fprintf(dest, "%s", atbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_FC3:           /* C64 FC3 */
                        if (c >= 0xcc && c <= MAX_FC3CC) {
                            fprintf(dest, "%s", fc3kw[c - 0xcc]);
                        }
                        break;

                    case B_ULTRA:         /* C64 Ultrabasic-64 */
                        if (c >= 0xcc && c <= MAX_ULTRCC) {
                            fprintf(dest, "%s", ultrabasic64kwcc[c - 0xcc]);
                        }
                        break;

                    case B_GRAPH:
                        if (c >= 0xcc && c <= MAX_GRAPHCC) {
                            fprintf(dest, "%s", graphicsbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_WS:
                        if (c >= 0xcc && c <= MAX_WSCC) {
                            fprintf(dest, "%s", wsbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_WSF:
                        if (c >= 0xcc && c <= MAX_WSFCC) {
                            fprintf(dest, "%s", wsfbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_PEG:
                        if (c >= 0xcc && c <= MAX_PEGCC) {
                            fprintf(dest, "%s", pegbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_X:
                        if (c >= 0xcc && c <= MAX_XCC) {
                            fprintf(dest, "%s", xbasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_DRAGO:
                        if (c >= 0xcc && c <= MAX_DRAGOCC) {
                            fprintf(dest, "%s", dragobasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_REU:
                        if (c >= 0xcc && c <= MAX_REUCC) {
                            fprintf(dest, "%s", reubasickwcc[c - 0xcc]);
                        }
                        break;

                    case B_BASL:
                        if (c >= 0xcc && c <= MAX_BASLCC) {
                            fprintf(dest, "%s", baslkwcc[c - 0xcc]);
                        }
                        break;

                    case B_4:             /* PET V4.0 */
                    case B_4E:            /* V4.0 extension */
                        if (c >= 0xcc && c <= MAX_4ECC) {
                            fprintf(dest, "%s", petkwcc[c - 0xcc]);
                        }
                        break;

                    case B_VIC4:
                        if (c >= 0xcc && c <= MAX_VIC4) {
                            fprintf(dest, "%s", vic4kwcc[c - 0xcc]);
                        }
                        break;

                    case B_VIC5:
                        if (c >= 0xcc && c <= MAX_VIC5) {
                            fprintf(dest, "%s", vic5kwcc[c - 0xcc]);
                        }
                        break;

                    case B_SUPERGRA:
                        if (c >= 0xcc && c <= MAX_SUPERGRACC) {
                            fprintf(dest, "%s", supergrakw[c - 0xcc]);
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

            _p_toascii((int)c, ctrls);  /* convert character */
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
                            if ((c = sstrcmp(p2, kwfe, 2, NUM_V10FE)) != KW_NONE) {
                                *p1++ = 0xfe;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            } else if ((c = sstrcmp(p2, kwce, 2, NUM_KWCE)) != KW_NONE) {
                                *p1++ = 0xce;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                        case B_71:
                            if ((c = sstrcmp(p2, kwfe, 2, NUM_V71FE)) != KW_NONE) {
                                *p1++ = 0xfe;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            } else if ((c = sstrcmp(p2, kwce, 2, NUM_KWCE)) != KW_NONE) {
                                *p1++ = 0xce;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                        case B_7:
                            if ((c = sstrcmp(p2, kwfe, 2, NUM_V7FE)) != KW_NONE) {
                                *p1++ = 0xfe;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            } else if ((c = sstrcmp(p2, kwce, 2, NUM_KWCE)) != KW_NONE) {
                                *p1++ = 0xce;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;
                        case B_SXC:
                            if ((c = sstrcmp(p2, sxckwfe, 2, NUM_SXCFE)) != KW_NONE) {
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
                        max = NUM_B_1;
                    } else if ((version == B_35) || (version == B_7) || (version == B_71) ||
                               (version == B_10) || (version == B_SXC)) {
                        max = 0x7E;
                    } else {
                        max = NUM_COMM;
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
                        case B_SUPEREXP:
                            if ((c = sstrcmp(p2, superexpkwcc, 0, NUM_SECC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_TURTLE:
                            if ((c = sstrcmp(p2, turtlekwcc, 0, NUM_TUCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_EASY:
                            if ((c = sstrcmp(p2, easykwcc, 0, NUM_EASYCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_BLARG:
                            if ((c = sstrcmp(p2, blargkwe0, 0, NUM_BLARGE0)) != KW_NONE) {
                                *p1++ = c + 0xe0;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_SUPERBAS:
                            if ((c = sstrcmp(p2, superbaskwdb, 0, NUM_SUPERBASDB)) != KW_NONE) {
                                *p1++ = c + 0xdb;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_WARSAW:
                            if ((c = sstrcmp(p2, warsawkwdb, 0, NUM_WARSAWDB)) != KW_NONE) {
                                *p1++ = c + 0xdb;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_EXPBAS20:
                            if ((c = sstrcmp(p2, expbas20kwcc, 0, NUM_EXPBAS20CC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_EXPBAS64:
                            if ((c = sstrcmp(p2, expbas64kwcc, 0, NUM_EXPBAS64CC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_GB:
                            if ((c = sstrcmp(p2, gbkwcc, 0, NUM_GBCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p1 += kwlen;
                                match++;
                            }
                            break;

                        case B_BSX:
                            if ((c = sstrcmp(p2, bsxkwcc, 0, NUM_BSXCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p1 += kwlen;
                                match++;
                            }
                            break;

                        case B_MIGHTY:
                            if ((c = sstrcmp(p2, mightykwcc, 0, NUM_MIGHTYCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_MAGIC:
                            if ((c = sstrcmp(p2, magickwcc, 0, NUM_MAGICCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_4:
                        case B_4E:
                            if ((c = sstrcmp(p2, petkwcc, 0, ((version == B_4) ? NUM_V4CC : NUM_4ECC))) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_VIC4:
                            if ((c = sstrcmp(p2, vic4kwcc, 0, NUM_VIC4)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_VIC5:
                            if ((c = sstrcmp(p2, vic5kwcc, 0, NUM_VIC5)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_SIMON:
                            if ((c = sstrcmp(p2, simonskw, 1, 0x80)) != KW_NONE) {
                                *p1++ = 0x64;
                                *p1++ = c;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_SPEECH:
                            if ((c = sstrcmp(p2, speechkwcc, 0, NUM_SPECC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_ATBAS:
                            if ((c = sstrcmp(p2, atbasickwcc, 0, NUM_ATBCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_FC3:
                            if ((c = sstrcmp(p2, fc3kw, 0, NUM_FC3CC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_ULTRA:
                            if ((c = sstrcmp(p2, ultrabasic64kwcc, 0, NUM_ULTRCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_GRAPH:
                            if ((c = sstrcmp(p2, graphicsbasickwcc, 0, NUM_GRAPHCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_WS:
                            if ((c = sstrcmp(p2, wsbasickwcc, 0, NUM_WSCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_WSF:
                            if ((c = sstrcmp(p2, wsfbasickwcc, 0, NUM_WSCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_PEG:
                            if ((c = sstrcmp(p2, pegbasickwcc, 0, NUM_PEGCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_X:
                            if ((c = sstrcmp(p2, xbasickwcc, 0, NUM_XCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_DRAGO:
                            if ((c = sstrcmp(p2, dragobasickwcc, 0, NUM_DRAGOCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_REU:
                            if ((c = sstrcmp(p2, reubasickwcc, 0, NUM_REUCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_BASL:
                            if ((c = sstrcmp(p2, baslkwcc, 0, NUM_BASLCC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
                                p2 += kwlen;
                                match++;
                            }
                            break;

                        case B_SUPERGRA:
                            if ((c = sstrcmp(p2, supergrakw, 0, NUM_SUPERGRACC)) != KW_NONE) {
                                *p1++ = c + 0xcc;
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
