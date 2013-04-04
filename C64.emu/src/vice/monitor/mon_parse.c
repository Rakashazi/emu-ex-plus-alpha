/* A Bison parser, made by GNU Bison 2.4.2.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2006, 2009-2010 Free Software
   Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "mon_parse.y"

/* -*- C -*-
 *
 * mon_parse.y - Parser for the VICE built-in monitor.
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

#include "vice.h"

#ifndef MINIXVMD
#ifdef __GNUC__
#undef alloca
#ifndef ANDROID_COMPILE
#define        alloca(n)       __builtin_alloca (n)
#endif
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else  /* Not HAVE_ALLOCA_H.  */
#if !defined(_AIX) && !defined(WINCE)
#ifndef _MSC_VER
extern char *alloca();
#else
#define alloca(n)   _alloca(n)
#endif  /* MSVC */
#endif /* Not AIX and not WINCE.  */
#endif /* HAVE_ALLOCA_H.  */
#endif /* GCC.  */
#endif /* MINIXVMD */

/* SunOS 4.x specific stuff */
#if defined(sun) || defined(__sun)
#  if !defined(__SVR4) && !defined(__svr4__)
#    ifdef __sparc__
#      define YYFREE
#    endif
#  endif
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "asm.h"
#include "console.h"
#include "lib.h"
#include "machine.h"
#include "mon_breakpoint.h"
#include "mon_command.h"
#include "mon_disassemble.h"
#include "mon_drive.h"
#include "mon_file.h"
#include "mon_memory.h"
#include "mon_util.h"
#include "montypes.h"
#include "resources.h"
#include "types.h"
#include "uimon.h"


#define join_ints(x,y) (LO16_TO_HI16(x)|y)
#define separate_int1(x) (HI16_TO_LO16(x))
#define separate_int2(x) (LO16(x))

static int yyerror(char *s);
static int temp;
static int resolve_datatype(unsigned guess_type, const char *num);
static int resolve_range(enum t_memspace memspace, MON_ADDR range[2],
                         const char *num);

#ifdef __IBMC__
static void __yy_memcpy (char *to, char *from, int count);
#endif

/* Defined in the lexer */
extern int new_cmd, opt_asm;
extern void free_buffer(void);
extern void make_buffer(char *str);
extern int yylex(void);
extern int cur_len, last_len;

#define ERR_ILLEGAL_INPUT 1     /* Generic error as returned by yacc.  */
#define ERR_RANGE_BAD_START 2
#define ERR_RANGE_BAD_END 3
#define ERR_BAD_CMD 4
#define ERR_EXPECT_CHECKNUM 5
#define ERR_EXPECT_END_CMD 6
#define ERR_MISSING_CLOSE_PAREN 7
#define ERR_INCOMPLETE_COMPARE_OP 8
#define ERR_EXPECT_FILENAME 9
#define ERR_ADDR_TOO_BIG 10
#define ERR_IMM_TOO_BIG 11
#define ERR_EXPECT_STRING 12
#define ERR_UNDEFINED_LABEL 13
#define ERR_EXPECT_DEVICE_NUM 14
#define ERR_EXPECT_ADDRESS 15

#define BAD_ADDR (new_addr(e_invalid_space, 0))
#define CHECK_ADDR(x) ((x) == addr_mask(x))

#define YYDEBUG 1



/* Line 189 of yacc.c  */
#line 201 "mon_parse.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     H_NUMBER = 258,
     D_NUMBER = 259,
     O_NUMBER = 260,
     B_NUMBER = 261,
     CONVERT_OP = 262,
     B_DATA = 263,
     H_RANGE_GUESS = 264,
     D_NUMBER_GUESS = 265,
     O_NUMBER_GUESS = 266,
     B_NUMBER_GUESS = 267,
     BAD_CMD = 268,
     MEM_OP = 269,
     IF = 270,
     MEM_COMP = 271,
     MEM_DISK8 = 272,
     MEM_DISK9 = 273,
     MEM_DISK10 = 274,
     MEM_DISK11 = 275,
     EQUALS = 276,
     TRAIL = 277,
     CMD_SEP = 278,
     LABEL_ASGN_COMMENT = 279,
     CMD_SIDEFX = 280,
     CMD_RETURN = 281,
     CMD_BLOCK_READ = 282,
     CMD_BLOCK_WRITE = 283,
     CMD_UP = 284,
     CMD_DOWN = 285,
     CMD_LOAD = 286,
     CMD_SAVE = 287,
     CMD_VERIFY = 288,
     CMD_IGNORE = 289,
     CMD_HUNT = 290,
     CMD_FILL = 291,
     CMD_MOVE = 292,
     CMD_GOTO = 293,
     CMD_REGISTERS = 294,
     CMD_READSPACE = 295,
     CMD_WRITESPACE = 296,
     CMD_RADIX = 297,
     CMD_MEM_DISPLAY = 298,
     CMD_BREAK = 299,
     CMD_TRACE = 300,
     CMD_IO = 301,
     CMD_BRMON = 302,
     CMD_COMPARE = 303,
     CMD_DUMP = 304,
     CMD_UNDUMP = 305,
     CMD_EXIT = 306,
     CMD_DELETE = 307,
     CMD_CONDITION = 308,
     CMD_COMMAND = 309,
     CMD_ASSEMBLE = 310,
     CMD_DISASSEMBLE = 311,
     CMD_NEXT = 312,
     CMD_STEP = 313,
     CMD_PRINT = 314,
     CMD_DEVICE = 315,
     CMD_HELP = 316,
     CMD_WATCH = 317,
     CMD_DISK = 318,
     CMD_SYSTEM = 319,
     CMD_QUIT = 320,
     CMD_CHDIR = 321,
     CMD_BANK = 322,
     CMD_LOAD_LABELS = 323,
     CMD_SAVE_LABELS = 324,
     CMD_ADD_LABEL = 325,
     CMD_DEL_LABEL = 326,
     CMD_SHOW_LABELS = 327,
     CMD_RECORD = 328,
     CMD_MON_STOP = 329,
     CMD_PLAYBACK = 330,
     CMD_CHAR_DISPLAY = 331,
     CMD_SPRITE_DISPLAY = 332,
     CMD_TEXT_DISPLAY = 333,
     CMD_SCREENCODE_DISPLAY = 334,
     CMD_ENTER_DATA = 335,
     CMD_ENTER_BIN_DATA = 336,
     CMD_KEYBUF = 337,
     CMD_BLOAD = 338,
     CMD_BSAVE = 339,
     CMD_SCREEN = 340,
     CMD_UNTIL = 341,
     CMD_CPU = 342,
     CMD_YYDEBUG = 343,
     CMD_BACKTRACE = 344,
     CMD_SCREENSHOT = 345,
     CMD_PWD = 346,
     CMD_DIR = 347,
     CMD_RESOURCE_GET = 348,
     CMD_RESOURCE_SET = 349,
     CMD_LOAD_RESOURCES = 350,
     CMD_SAVE_RESOURCES = 351,
     CMD_ATTACH = 352,
     CMD_DETACH = 353,
     CMD_MON_RESET = 354,
     CMD_TAPECTRL = 355,
     CMD_CARTFREEZE = 356,
     CMD_CPUHISTORY = 357,
     CMD_MEMMAPZAP = 358,
     CMD_MEMMAPSHOW = 359,
     CMD_MEMMAPSAVE = 360,
     CMD_COMMENT = 361,
     CMD_LIST = 362,
     CMD_STOPWATCH = 363,
     RESET = 364,
     CMD_EXPORT = 365,
     CMD_AUTOSTART = 366,
     CMD_AUTOLOAD = 367,
     CMD_LABEL_ASGN = 368,
     L_PAREN = 369,
     R_PAREN = 370,
     ARG_IMMEDIATE = 371,
     REG_A = 372,
     REG_X = 373,
     REG_Y = 374,
     COMMA = 375,
     INST_SEP = 376,
     L_BRACKET = 377,
     R_BRACKET = 378,
     LESS_THAN = 379,
     REG_U = 380,
     REG_S = 381,
     REG_PC = 382,
     REG_PCR = 383,
     REG_B = 384,
     REG_C = 385,
     REG_D = 386,
     REG_E = 387,
     REG_H = 388,
     REG_L = 389,
     REG_AF = 390,
     REG_BC = 391,
     REG_DE = 392,
     REG_HL = 393,
     REG_IX = 394,
     REG_IY = 395,
     REG_SP = 396,
     REG_IXH = 397,
     REG_IXL = 398,
     REG_IYH = 399,
     REG_IYL = 400,
     PLUS = 401,
     MINUS = 402,
     STRING = 403,
     FILENAME = 404,
     R_O_L = 405,
     OPCODE = 406,
     LABEL = 407,
     BANKNAME = 408,
     CPUTYPE = 409,
     MON_REGISTER = 410,
     COMPARE_OP = 411,
     RADIX_TYPE = 412,
     INPUT_SPEC = 413,
     CMD_CHECKPT_ON = 414,
     CMD_CHECKPT_OFF = 415,
     TOGGLE = 416,
     MASK = 417
   };
#endif
/* Tokens.  */
#define H_NUMBER 258
#define D_NUMBER 259
#define O_NUMBER 260
#define B_NUMBER 261
#define CONVERT_OP 262
#define B_DATA 263
#define H_RANGE_GUESS 264
#define D_NUMBER_GUESS 265
#define O_NUMBER_GUESS 266
#define B_NUMBER_GUESS 267
#define BAD_CMD 268
#define MEM_OP 269
#define IF 270
#define MEM_COMP 271
#define MEM_DISK8 272
#define MEM_DISK9 273
#define MEM_DISK10 274
#define MEM_DISK11 275
#define EQUALS 276
#define TRAIL 277
#define CMD_SEP 278
#define LABEL_ASGN_COMMENT 279
#define CMD_SIDEFX 280
#define CMD_RETURN 281
#define CMD_BLOCK_READ 282
#define CMD_BLOCK_WRITE 283
#define CMD_UP 284
#define CMD_DOWN 285
#define CMD_LOAD 286
#define CMD_SAVE 287
#define CMD_VERIFY 288
#define CMD_IGNORE 289
#define CMD_HUNT 290
#define CMD_FILL 291
#define CMD_MOVE 292
#define CMD_GOTO 293
#define CMD_REGISTERS 294
#define CMD_READSPACE 295
#define CMD_WRITESPACE 296
#define CMD_RADIX 297
#define CMD_MEM_DISPLAY 298
#define CMD_BREAK 299
#define CMD_TRACE 300
#define CMD_IO 301
#define CMD_BRMON 302
#define CMD_COMPARE 303
#define CMD_DUMP 304
#define CMD_UNDUMP 305
#define CMD_EXIT 306
#define CMD_DELETE 307
#define CMD_CONDITION 308
#define CMD_COMMAND 309
#define CMD_ASSEMBLE 310
#define CMD_DISASSEMBLE 311
#define CMD_NEXT 312
#define CMD_STEP 313
#define CMD_PRINT 314
#define CMD_DEVICE 315
#define CMD_HELP 316
#define CMD_WATCH 317
#define CMD_DISK 318
#define CMD_SYSTEM 319
#define CMD_QUIT 320
#define CMD_CHDIR 321
#define CMD_BANK 322
#define CMD_LOAD_LABELS 323
#define CMD_SAVE_LABELS 324
#define CMD_ADD_LABEL 325
#define CMD_DEL_LABEL 326
#define CMD_SHOW_LABELS 327
#define CMD_RECORD 328
#define CMD_MON_STOP 329
#define CMD_PLAYBACK 330
#define CMD_CHAR_DISPLAY 331
#define CMD_SPRITE_DISPLAY 332
#define CMD_TEXT_DISPLAY 333
#define CMD_SCREENCODE_DISPLAY 334
#define CMD_ENTER_DATA 335
#define CMD_ENTER_BIN_DATA 336
#define CMD_KEYBUF 337
#define CMD_BLOAD 338
#define CMD_BSAVE 339
#define CMD_SCREEN 340
#define CMD_UNTIL 341
#define CMD_CPU 342
#define CMD_YYDEBUG 343
#define CMD_BACKTRACE 344
#define CMD_SCREENSHOT 345
#define CMD_PWD 346
#define CMD_DIR 347
#define CMD_RESOURCE_GET 348
#define CMD_RESOURCE_SET 349
#define CMD_LOAD_RESOURCES 350
#define CMD_SAVE_RESOURCES 351
#define CMD_ATTACH 352
#define CMD_DETACH 353
#define CMD_MON_RESET 354
#define CMD_TAPECTRL 355
#define CMD_CARTFREEZE 356
#define CMD_CPUHISTORY 357
#define CMD_MEMMAPZAP 358
#define CMD_MEMMAPSHOW 359
#define CMD_MEMMAPSAVE 360
#define CMD_COMMENT 361
#define CMD_LIST 362
#define CMD_STOPWATCH 363
#define RESET 364
#define CMD_EXPORT 365
#define CMD_AUTOSTART 366
#define CMD_AUTOLOAD 367
#define CMD_LABEL_ASGN 368
#define L_PAREN 369
#define R_PAREN 370
#define ARG_IMMEDIATE 371
#define REG_A 372
#define REG_X 373
#define REG_Y 374
#define COMMA 375
#define INST_SEP 376
#define L_BRACKET 377
#define R_BRACKET 378
#define LESS_THAN 379
#define REG_U 380
#define REG_S 381
#define REG_PC 382
#define REG_PCR 383
#define REG_B 384
#define REG_C 385
#define REG_D 386
#define REG_E 387
#define REG_H 388
#define REG_L 389
#define REG_AF 390
#define REG_BC 391
#define REG_DE 392
#define REG_HL 393
#define REG_IX 394
#define REG_IY 395
#define REG_SP 396
#define REG_IXH 397
#define REG_IXL 398
#define REG_IYH 399
#define REG_IYL 400
#define PLUS 401
#define MINUS 402
#define STRING 403
#define FILENAME 404
#define R_O_L 405
#define OPCODE 406
#define LABEL 407
#define BANKNAME 408
#define CPUTYPE 409
#define MON_REGISTER 410
#define COMPARE_OP 411
#define RADIX_TYPE 412
#define INPUT_SPEC 413
#define CMD_CHECKPT_ON 414
#define CMD_CHECKPT_OFF 415
#define TOGGLE 416
#define MASK 417




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 129 "mon_parse.y"

    MON_ADDR a;
    MON_ADDR range[2];
    int i;
    REG_ID reg;
    CONDITIONAL cond_op;
    cond_node_t *cond_node;
    RADIXTYPE rt;
    ACTION action;
    char *str;
    asm_mode_addr_info_t mode;



/* Line 214 of yacc.c  */
#line 576 "mon_parse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 588 "mon_parse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  307
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1740

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  169
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  55
/* YYNRULES -- Number of rules.  */
#define YYNRULES  309
/* YYNRULES -- Number of states.  */
#define YYNSTATES  618

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   417

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     167,   168,   165,   163,     2,   164,     2,   166,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    10,    12,    15,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
      41,    43,    45,    47,    50,    54,    58,    64,    68,    71,
      74,    78,    81,    85,    88,    93,    96,   100,   104,   107,
     112,   115,   120,   123,   128,   131,   136,   139,   141,   144,
     148,   152,   158,   162,   168,   172,   178,   182,   188,   192,
     195,   200,   206,   207,   213,   217,   221,   224,   230,   236,
     242,   248,   254,   258,   261,   265,   268,   272,   275,   279,
     282,   286,   289,   292,   295,   300,   306,   312,   318,   321,
     325,   328,   334,   337,   343,   346,   350,   354,   358,   364,
     368,   371,   377,   383,   388,   392,   395,   399,   402,   406,
     409,   412,   415,   419,   423,   426,   430,   434,   438,   442,
     446,   449,   453,   456,   460,   466,   470,   475,   479,   483,
     486,   491,   496,   499,   503,   507,   510,   516,   522,   528,
     532,   537,   543,   548,   554,   559,   565,   571,   574,   578,
     583,   587,   591,   597,   601,   607,   611,   614,   618,   623,
     626,   629,   631,   633,   634,   636,   638,   640,   642,   645,
     647,   649,   650,   652,   655,   659,   661,   665,   667,   669,
     671,   673,   677,   679,   683,   686,   687,   689,   693,   695,
     697,   698,   700,   702,   704,   706,   708,   710,   712,   716,
     720,   724,   728,   732,   736,   738,   741,   742,   746,   750,
     754,   758,   760,   762,   764,   768,   770,   772,   774,   777,
     779,   781,   783,   785,   787,   789,   791,   793,   795,   797,
     799,   801,   803,   805,   807,   809,   811,   813,   817,   821,
     824,   827,   829,   831,   834,   836,   840,   844,   848,   852,
     856,   862,   870,   876,   880,   884,   888,   892,   896,   900,
     906,   912,   918,   924,   925,   927,   929,   931,   933,   935,
     937,   939,   941,   943,   945,   947,   949,   951,   953,   955,
     957,   959,   961,   964,   968,   972,   977,   981,   986,   989,
     993,   997,  1001,  1005,  1011,  1017,  1024,  1030,  1037,  1042,
    1048,  1054,  1060,  1066,  1070,  1076,  1078,  1080,  1082,  1084
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     170,     0,    -1,   171,    -1,   219,    22,    -1,    22,    -1,
     173,    -1,   171,   173,    -1,    23,    -1,    22,    -1,     1,
      -1,   174,    -1,   176,    -1,   179,    -1,   177,    -1,   180,
      -1,   181,    -1,   182,    -1,   183,    -1,   184,    -1,   185,
      -1,   186,    -1,   187,    -1,    13,    -1,    67,   172,    -1,
      67,   203,   172,    -1,    67,   153,   172,    -1,    67,   203,
     202,   153,   172,    -1,    38,   201,   172,    -1,    38,   172,
      -1,    46,   172,    -1,    46,   201,   172,    -1,    87,   172,
      -1,    87,   154,   172,    -1,   102,   172,    -1,   102,   202,
     206,   172,    -1,    26,   172,    -1,    49,   190,   172,    -1,
      50,   190,   172,    -1,    58,   172,    -1,    58,   202,   206,
     172,    -1,    57,   172,    -1,    57,   202,   206,   172,    -1,
      29,   172,    -1,    29,   202,   206,   172,    -1,    30,   172,
      -1,    30,   202,   206,   172,    -1,    85,   172,    -1,   175,
      -1,    39,   172,    -1,    39,   203,   172,    -1,    39,   195,
     172,    -1,    68,   203,   202,   190,   172,    -1,    68,   190,
     172,    -1,    69,   203,   202,   190,   172,    -1,    69,   190,
     172,    -1,    70,   201,   202,   152,   172,    -1,    71,   152,
     172,    -1,    71,   203,   202,   152,   172,    -1,    72,   203,
     172,    -1,    72,   172,    -1,   113,    21,   201,   172,    -1,
     113,    21,   201,    24,   172,    -1,    -1,    55,   201,   178,
     220,   172,    -1,    55,   201,   172,    -1,    56,   198,   172,
      -1,    56,   172,    -1,    37,   199,   202,   201,   172,    -1,
      48,   199,   202,   201,   172,    -1,    36,   199,   202,   210,
     172,    -1,    35,   199,   202,   212,   172,    -1,    43,   157,
     202,   198,   172,    -1,    43,   198,   172,    -1,    43,   172,
      -1,    76,   198,   172,    -1,    76,   172,    -1,    77,   198,
     172,    -1,    77,   172,    -1,    78,   198,   172,    -1,    78,
     172,    -1,    79,   198,   172,    -1,    79,   172,    -1,   103,
     172,    -1,   104,   172,    -1,   104,   202,   206,   172,    -1,
     104,   202,   206,   198,   172,    -1,   105,   190,   202,   206,
     172,    -1,    44,   193,   198,   207,   172,    -1,    44,   172,
      -1,    86,   198,   172,    -1,    86,   172,    -1,    62,   193,
     198,   207,   172,    -1,    62,   172,    -1,    45,   193,   198,
     207,   172,    -1,    45,   172,    -1,   159,   197,   172,    -1,
     160,   197,   172,    -1,    34,   197,   172,    -1,    34,   197,
     202,   206,   172,    -1,    52,   197,   172,    -1,    52,   172,
      -1,    53,   197,    15,   208,   172,    -1,    54,   197,   202,
     148,   172,    -1,    54,   197,     1,   172,    -1,    25,   161,
     172,    -1,    25,   172,    -1,    42,   157,   172,    -1,    42,
     172,    -1,    60,   203,   172,    -1,   110,   172,    -1,    65,
     172,    -1,    51,   172,    -1,    63,   188,   172,    -1,    59,
     206,   172,    -1,    61,   172,    -1,    61,   188,   172,    -1,
      64,   188,   172,    -1,     7,   206,   172,    -1,    66,   188,
     172,    -1,    82,   188,   172,    -1,    89,   172,    -1,    92,
     189,   172,    -1,    91,   172,    -1,    90,   190,   172,    -1,
      90,   190,   202,   206,   172,    -1,    93,   148,   172,    -1,
      94,   148,   148,   172,    -1,    95,   190,   172,    -1,    96,
     190,   172,    -1,    99,   172,    -1,    99,   202,   206,   172,
      -1,   100,   202,   206,   172,    -1,   101,   172,    -1,   106,
     189,   172,    -1,   108,   109,   172,    -1,   108,   172,    -1,
      31,   190,   191,   200,   172,    -1,    83,   190,   191,   200,
     172,    -1,    32,   190,   191,   199,   172,    -1,    32,   190,
       1,    -1,    32,   190,   191,     1,    -1,    84,   190,   191,
     199,   172,    -1,    84,   190,   191,     1,    -1,    33,   190,
     191,   201,   172,    -1,    33,   190,   191,     1,    -1,    27,
     206,   206,   200,   172,    -1,    28,   206,   206,   201,   172,
      -1,   107,   172,    -1,   107,   191,   172,    -1,    97,   190,
     206,   172,    -1,    98,   206,   172,    -1,   111,   190,   172,
      -1,   111,   190,   202,   217,   172,    -1,   112,   190,   172,
      -1,   112,   190,   202,   217,   172,    -1,    73,   190,   172,
      -1,    74,   172,    -1,    75,   190,   172,    -1,    80,   201,
     210,   172,    -1,    81,   172,    -1,    88,   172,    -1,   150,
      -1,   150,    -1,    -1,   149,    -1,     1,    -1,   206,    -1,
       1,    -1,   192,    14,    -1,    14,    -1,   192,    -1,    -1,
     155,    -1,   203,   155,    -1,   195,   120,   196,    -1,   196,
      -1,   194,    21,   217,    -1,   215,    -1,     1,    -1,   199,
      -1,   201,    -1,   201,   202,   201,    -1,     9,    -1,   203,
     202,     9,    -1,   202,   201,    -1,    -1,   204,    -1,   203,
     202,   204,    -1,   152,    -1,   120,    -1,    -1,    16,    -1,
      17,    -1,    18,    -1,    19,    -1,    20,    -1,   205,    -1,
     217,    -1,   206,   163,   206,    -1,   206,   164,   206,    -1,
     206,   165,   206,    -1,   206,   166,   206,    -1,   167,   206,
     168,    -1,   167,   206,     1,    -1,   214,    -1,    15,   208,
      -1,    -1,   208,   156,   208,    -1,   208,   156,     1,    -1,
     114,   208,   115,    -1,   114,   208,     1,    -1,   209,    -1,
     194,    -1,   217,    -1,   210,   202,   211,    -1,   211,    -1,
     217,    -1,   148,    -1,   212,   213,    -1,   213,    -1,   217,
      -1,   162,    -1,   148,    -1,   217,    -1,   194,    -1,     4,
      -1,    12,    -1,    11,    -1,    10,    -1,    12,    -1,    11,
      -1,    10,    -1,     3,    -1,     4,    -1,     5,    -1,     6,
      -1,   216,    -1,   218,   121,   219,    -1,   219,   121,   219,
      -1,   219,   121,    -1,   151,   221,    -1,   219,    -1,   218,
      -1,   116,   217,    -1,   217,    -1,   217,   120,   118,    -1,
     217,   120,   119,    -1,   217,   120,   126,    -1,   217,   120,
     217,    -1,   114,   217,   115,    -1,   114,   217,   120,   118,
     115,    -1,   114,   217,   120,   126,   115,   120,   119,    -1,
     114,   217,   115,   120,   119,    -1,   114,   136,   115,    -1,
     114,   137,   115,    -1,   114,   138,   115,    -1,   114,   139,
     115,    -1,   114,   140,   115,    -1,   114,   141,   115,    -1,
     114,   217,   115,   120,   117,    -1,   114,   217,   115,   120,
     138,    -1,   114,   217,   115,   120,   139,    -1,   114,   217,
     115,   120,   140,    -1,    -1,   117,    -1,   129,    -1,   130,
      -1,   131,    -1,   132,    -1,   133,    -1,   142,    -1,   144,
      -1,   134,    -1,   143,    -1,   145,    -1,   135,    -1,   136,
      -1,   137,    -1,   138,    -1,   139,    -1,   140,    -1,   141,
      -1,   124,   217,    -1,   217,   120,   223,    -1,   120,   222,
     146,    -1,   120,   222,   146,   146,    -1,   120,   147,   222,
      -1,   120,   147,   147,   222,    -1,   120,   222,    -1,   129,
     120,   222,    -1,   117,   120,   222,    -1,   131,   120,   222,
      -1,   217,   120,   127,    -1,   122,   217,   120,   222,   123,
      -1,   122,   120,   222,   146,   123,    -1,   122,   120,   222,
     146,   146,   123,    -1,   122,   120,   147,   222,   123,    -1,
     122,   120,   147,   147,   222,   123,    -1,   122,   120,   222,
     123,    -1,   122,   129,   120,   222,   123,    -1,   122,   117,
     120,   222,   123,    -1,   122,   131,   120,   222,   123,    -1,
     122,   217,   120,   127,   123,    -1,   122,   217,   123,    -1,
     122,   217,   123,   120,   119,    -1,   118,    -1,   119,    -1,
     223,    -1,   126,    -1,   125,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   193,   193,   194,   195,   198,   199,   202,   203,   204,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   222,   224,   226,   228,   230,   232,   234,
     236,   238,   240,   242,   244,   246,   248,   250,   252,   254,
     256,   258,   260,   262,   264,   266,   268,   270,   273,   275,
     277,   280,   285,   290,   292,   294,   296,   298,   300,   302,
     304,   308,   315,   314,   317,   319,   321,   325,   327,   329,
     331,   333,   335,   337,   339,   341,   343,   345,   347,   349,
     351,   353,   355,   357,   359,   361,   363,   367,   376,   379,
     383,   386,   395,   398,   407,   412,   414,   416,   418,   420,
     422,   424,   426,   428,   432,   434,   439,   441,   459,   461,
     463,   465,   469,   471,   473,   475,   477,   479,   481,   483,
     485,   487,   489,   491,   493,   495,   497,   499,   501,   503,
     505,   507,   509,   511,   513,   515,   519,   521,   523,   525,
     527,   529,   531,   533,   535,   537,   539,   541,   543,   545,
     547,   549,   551,   553,   555,   559,   561,   563,   567,   569,
     573,   577,   580,   581,   584,   585,   588,   589,   592,   593,
     596,   597,   600,   601,   604,   605,   608,   612,   613,   616,
     617,   620,   621,   623,   627,   628,   631,   636,   641,   651,
     652,   655,   656,   657,   658,   659,   662,   664,   666,   667,
     668,   669,   670,   671,   672,   675,   676,   678,   683,   685,
     687,   689,   693,   699,   707,   708,   711,   712,   715,   716,
     719,   720,   721,   724,   725,   728,   729,   730,   731,   734,
     735,   736,   739,   740,   741,   742,   743,   746,   747,   748,
     751,   761,   762,   765,   772,   783,   794,   802,   821,   827,
     835,   843,   845,   847,   848,   849,   850,   851,   852,   853,
     855,   857,   859,   861,   862,   863,   864,   865,   866,   867,
     868,   869,   870,   871,   872,   873,   874,   875,   876,   877,
     878,   879,   881,   882,   897,   901,   905,   909,   913,   917,
     921,   925,   929,   941,   956,   960,   964,   968,   972,   976,
     980,   984,   988,  1000,  1005,  1013,  1014,  1015,  1016,  1020
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "H_NUMBER", "D_NUMBER", "O_NUMBER",
  "B_NUMBER", "CONVERT_OP", "B_DATA", "H_RANGE_GUESS", "D_NUMBER_GUESS",
  "O_NUMBER_GUESS", "B_NUMBER_GUESS", "BAD_CMD", "MEM_OP", "IF",
  "MEM_COMP", "MEM_DISK8", "MEM_DISK9", "MEM_DISK10", "MEM_DISK11",
  "EQUALS", "TRAIL", "CMD_SEP", "LABEL_ASGN_COMMENT", "CMD_SIDEFX",
  "CMD_RETURN", "CMD_BLOCK_READ", "CMD_BLOCK_WRITE", "CMD_UP", "CMD_DOWN",
  "CMD_LOAD", "CMD_SAVE", "CMD_VERIFY", "CMD_IGNORE", "CMD_HUNT",
  "CMD_FILL", "CMD_MOVE", "CMD_GOTO", "CMD_REGISTERS", "CMD_READSPACE",
  "CMD_WRITESPACE", "CMD_RADIX", "CMD_MEM_DISPLAY", "CMD_BREAK",
  "CMD_TRACE", "CMD_IO", "CMD_BRMON", "CMD_COMPARE", "CMD_DUMP",
  "CMD_UNDUMP", "CMD_EXIT", "CMD_DELETE", "CMD_CONDITION", "CMD_COMMAND",
  "CMD_ASSEMBLE", "CMD_DISASSEMBLE", "CMD_NEXT", "CMD_STEP", "CMD_PRINT",
  "CMD_DEVICE", "CMD_HELP", "CMD_WATCH", "CMD_DISK", "CMD_SYSTEM",
  "CMD_QUIT", "CMD_CHDIR", "CMD_BANK", "CMD_LOAD_LABELS",
  "CMD_SAVE_LABELS", "CMD_ADD_LABEL", "CMD_DEL_LABEL", "CMD_SHOW_LABELS",
  "CMD_RECORD", "CMD_MON_STOP", "CMD_PLAYBACK", "CMD_CHAR_DISPLAY",
  "CMD_SPRITE_DISPLAY", "CMD_TEXT_DISPLAY", "CMD_SCREENCODE_DISPLAY",
  "CMD_ENTER_DATA", "CMD_ENTER_BIN_DATA", "CMD_KEYBUF", "CMD_BLOAD",
  "CMD_BSAVE", "CMD_SCREEN", "CMD_UNTIL", "CMD_CPU", "CMD_YYDEBUG",
  "CMD_BACKTRACE", "CMD_SCREENSHOT", "CMD_PWD", "CMD_DIR",
  "CMD_RESOURCE_GET", "CMD_RESOURCE_SET", "CMD_LOAD_RESOURCES",
  "CMD_SAVE_RESOURCES", "CMD_ATTACH", "CMD_DETACH", "CMD_MON_RESET",
  "CMD_TAPECTRL", "CMD_CARTFREEZE", "CMD_CPUHISTORY", "CMD_MEMMAPZAP",
  "CMD_MEMMAPSHOW", "CMD_MEMMAPSAVE", "CMD_COMMENT", "CMD_LIST",
  "CMD_STOPWATCH", "RESET", "CMD_EXPORT", "CMD_AUTOSTART", "CMD_AUTOLOAD",
  "CMD_LABEL_ASGN", "L_PAREN", "R_PAREN", "ARG_IMMEDIATE", "REG_A",
  "REG_X", "REG_Y", "COMMA", "INST_SEP", "L_BRACKET", "R_BRACKET",
  "LESS_THAN", "REG_U", "REG_S", "REG_PC", "REG_PCR", "REG_B", "REG_C",
  "REG_D", "REG_E", "REG_H", "REG_L", "REG_AF", "REG_BC", "REG_DE",
  "REG_HL", "REG_IX", "REG_IY", "REG_SP", "REG_IXH", "REG_IXL", "REG_IYH",
  "REG_IYL", "PLUS", "MINUS", "STRING", "FILENAME", "R_O_L", "OPCODE",
  "LABEL", "BANKNAME", "CPUTYPE", "MON_REGISTER", "COMPARE_OP",
  "RADIX_TYPE", "INPUT_SPEC", "CMD_CHECKPT_ON", "CMD_CHECKPT_OFF",
  "TOGGLE", "MASK", "'+'", "'-'", "'*'", "'/'", "'('", "')'", "$accept",
  "top_level", "command_list", "end_cmd", "command", "machine_state_rules",
  "register_mod", "symbol_table_rules", "asm_rules", "$@1", "memory_rules",
  "checkpoint_rules", "checkpoint_control_rules", "monitor_state_rules",
  "monitor_misc_rules", "disk_rules", "cmd_file_rules", "data_entry_rules",
  "monitor_debug_rules", "rest_of_line", "opt_rest_of_line", "filename",
  "device_num", "mem_op", "opt_mem_op", "register", "reg_list", "reg_asgn",
  "checkpt_num", "address_opt_range", "address_range", "opt_address",
  "address", "opt_sep", "memspace", "memloc", "memaddr", "expression",
  "opt_if_cond_expr", "cond_expr", "compare_operand", "data_list",
  "data_element", "hunt_list", "hunt_element", "value", "d_number",
  "guess_default", "number", "assembly_instr_list", "assembly_instruction",
  "post_assemble", "asm_operand_mode", "index_reg", "index_ureg", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,    43,    45,    42,    47,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   169,   170,   170,   170,   171,   171,   172,   172,   172,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   174,   174,
     174,   174,   174,   174,   174,   174,   174,   174,   175,   175,
     175,   176,   176,   176,   176,   176,   176,   176,   176,   176,
     176,   176,   178,   177,   177,   177,   177,   179,   179,   179,
     179,   179,   179,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   179,   179,   179,   179,   180,   180,   180,
     180,   180,   180,   180,   180,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   182,   182,   182,   182,   182,   182,
     182,   182,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   185,   185,   185,   186,   186,
     187,   188,   189,   189,   190,   190,   191,   191,   192,   192,
     193,   193,   194,   194,   195,   195,   196,   197,   197,   198,
     198,   199,   199,   199,   200,   200,   201,   201,   201,   202,
     202,   203,   203,   203,   203,   203,   204,   205,   206,   206,
     206,   206,   206,   206,   206,   207,   207,   208,   208,   208,
     208,   208,   209,   209,   210,   210,   211,   211,   212,   212,
     213,   213,   213,   214,   214,   215,   215,   215,   215,   216,
     216,   216,   217,   217,   217,   217,   217,   218,   218,   218,
     219,   220,   220,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   222,   222,   222,   222,   223
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     5,     3,     2,     2,
       3,     2,     3,     2,     4,     2,     3,     3,     2,     4,
       2,     4,     2,     4,     2,     4,     2,     1,     2,     3,
       3,     5,     3,     5,     3,     5,     3,     5,     3,     2,
       4,     5,     0,     5,     3,     3,     2,     5,     5,     5,
       5,     5,     3,     2,     3,     2,     3,     2,     3,     2,
       3,     2,     2,     2,     4,     5,     5,     5,     2,     3,
       2,     5,     2,     5,     2,     3,     3,     3,     5,     3,
       2,     5,     5,     4,     3,     2,     3,     2,     3,     2,
       2,     2,     3,     3,     2,     3,     3,     3,     3,     3,
       2,     3,     2,     3,     5,     3,     4,     3,     3,     2,
       4,     4,     2,     3,     3,     2,     5,     5,     5,     3,
       4,     5,     4,     5,     4,     5,     5,     2,     3,     4,
       3,     3,     5,     3,     5,     3,     2,     3,     4,     2,
       2,     1,     1,     0,     1,     1,     1,     1,     2,     1,
       1,     0,     1,     2,     3,     1,     3,     1,     1,     1,
       1,     3,     1,     3,     2,     0,     1,     3,     1,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       3,     3,     3,     3,     1,     2,     0,     3,     3,     3,
       3,     1,     1,     1,     3,     1,     1,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     2,
       2,     1,     1,     2,     1,     3,     3,     3,     3,     3,
       5,     7,     5,     3,     3,     3,     3,     3,     3,     5,
       5,     5,     5,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     3,     3,     4,     3,     4,     2,     3,
       3,     3,     3,     5,     5,     6,     5,     6,     4,     5,
       5,     5,     5,     3,     5,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,    22,     4,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   163,     0,
       0,     0,     0,     0,     0,     0,   190,     0,     0,     0,
       0,     0,   163,     0,     0,     0,     0,     0,     0,   263,
       0,     0,     0,     2,     5,    10,    47,    11,    13,    12,
      14,    15,    16,    17,    18,    19,    20,    21,     0,   232,
     233,   234,   235,   231,   230,   229,   191,   192,   193,   194,
     195,   172,     0,   224,     0,     0,   204,   236,   223,     9,
       8,     7,     0,   105,    35,     0,     0,   189,    42,     0,
      44,     0,   165,   164,     0,     0,     0,   178,   225,   228,
     227,   226,     0,   177,   182,   188,   190,   190,   190,   186,
     196,   197,   190,   190,    28,     0,   190,    48,     0,     0,
     175,     0,     0,   107,   190,    73,     0,   179,   190,   169,
      88,   170,     0,    94,     0,    29,     0,   190,     0,     0,
     111,     9,   100,     0,     0,     0,     0,    66,     0,    40,
       0,    38,     0,     0,     0,   161,   114,     0,    92,     0,
       0,     0,   110,     0,     0,    23,     0,     0,   190,     0,
     190,   190,     0,   190,    59,     0,     0,   156,     0,    75,
       0,    77,     0,    79,     0,    81,     0,     0,   159,     0,
       0,     0,    46,    90,     0,     0,    31,   160,   120,     0,
     122,   162,     0,     0,     0,     0,     0,     0,     0,   129,
       0,     0,   132,    33,     0,    82,    83,     0,   190,     0,
       9,   147,     0,   166,     0,   135,   109,     0,     0,     0,
       0,     0,   264,     0,     0,     0,   265,   266,   267,   268,
     269,   272,   275,   276,   277,   278,   279,   280,   281,   270,
     273,   271,   274,   244,   240,     0,     0,     1,     6,     3,
       0,   173,     0,     0,     0,     0,   117,   104,   190,     0,
       0,     0,   167,   190,   139,     0,     0,    97,     0,     0,
       0,     0,     0,     0,    27,     0,     0,     0,    50,    49,
     106,     0,    72,   168,   206,   206,    30,     0,    36,    37,
      99,     0,     0,     0,    64,     0,    65,     0,     0,   113,
     108,   115,   206,   112,   116,   118,    25,    24,     0,    52,
       0,    54,     0,     0,    56,     0,    58,   155,   157,    74,
      76,    78,    80,   217,     0,   215,   216,   119,   190,     0,
      89,    32,   123,     0,   121,   125,     0,   127,   128,     0,
     150,     0,     0,     0,     0,     0,   133,   148,   134,   151,
       0,   153,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   243,     0,   305,   306,   309,   308,     0,   288,   307,
       0,     0,     0,     0,     0,   282,     0,     0,     0,    95,
      96,   203,   202,   198,   199,   200,   201,     0,     0,     0,
      43,    45,     0,   140,     0,   144,     0,     0,   222,   221,
       0,   219,   220,   181,   183,   187,     0,     0,   176,   174,
       0,     0,     0,     0,     0,     0,   212,     0,   211,   213,
     103,     0,   242,   241,     0,    41,    39,     0,     0,     0,
       0,     0,     0,   158,     0,     0,   142,     0,     0,   126,
     149,   130,   131,    34,    84,     0,     0,     0,     0,     0,
      60,   253,   254,   255,   256,   257,   258,   249,     0,   290,
       0,   286,   284,     0,     0,     0,     0,     0,     0,   303,
     289,   291,   245,   246,   247,   292,   248,   283,   145,   184,
     146,   136,   138,   143,    98,    70,   218,    69,    67,    71,
     205,    87,    93,    68,     0,     0,   101,   102,     0,   239,
      63,    91,    26,    51,    53,    55,    57,   214,   137,   141,
     124,    85,    86,   152,   154,    61,     0,     0,     0,   287,
     285,     0,     0,     0,   298,     0,     0,     0,     0,     0,
       0,   210,   209,   208,   207,   237,   238,   259,   252,   260,
     261,   262,   250,     0,   300,     0,   296,   294,     0,   299,
     301,   302,   293,   304,     0,   297,   295,   251
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    92,    93,   133,    94,    95,    96,    97,    98,   355,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   207,
     252,   144,   272,   181,   182,   123,   169,   170,   152,   176,
     177,   447,   178,   448,   124,   159,   160,   273,   472,   477,
     478,   384,   385,   460,   461,   126,   153,   127,   161,   482,
     108,   484,   304,   428,   429
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -459
static const yytype_int16 yypact[] =
{
    1180,   958,  -459,  -459,     2,   307,   958,   958,   560,   560,
      10,    10,    10,   586,  1526,  1526,  1526,  1297,    73,    19,
     978,  1068,  1068,  1297,  1526,    10,    10,   307,   712,   586,
     586,  1544,  1145,   560,   560,   958,   747,   114,  1068,  -110,
    -110,   307,  -110,   215,   202,   202,  1544,   327,   584,    10,
     307,    10,  1145,  1145,  1145,  1145,  1544,   307,  -110,    10,
      10,   307,  1145,    29,   307,   307,    10,   307,   -86,   -48,
     -46,    10,    10,    10,   958,   560,   -15,   307,   560,   307,
     560,    10,   -86,   355,    49,   307,    10,    10,    86,  1568,
     586,   586,   127,  1316,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,    92,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,   958,  -459,   -45,    46,  -459,  -459,  -459,  -459,
    -459,  -459,   307,  -459,  -459,   938,   938,  -459,  -459,   958,
    -459,   958,  -459,  -459,   767,   792,   767,  -459,  -459,  -459,
    -459,  -459,   560,  -459,  -459,  -459,   -15,   -15,   -15,  -459,
    -459,  -459,   -15,   -15,  -459,   307,   -15,  -459,   107,   110,
    -459,    34,   307,  -459,   -15,  -459,   307,  -459,   287,  -459,
    -459,   125,  1526,  -459,  1526,  -459,   307,   -15,   307,   307,
    -459,   332,  -459,   307,   131,    57,    62,  -459,   307,  -459,
     958,  -459,   958,    46,   307,  -459,  -459,   307,  -459,  1526,
     307,   307,  -459,   307,   307,  -459,   249,   307,   -15,   307,
     -15,   -15,   307,   -15,  -459,   307,   307,  -459,   307,  -459,
     307,  -459,   307,  -459,   307,  -459,   307,   410,  -459,   307,
     767,   767,  -459,  -459,   307,   307,  -459,  -459,  -459,   560,
    -459,  -459,   307,   307,     5,   307,   307,   958,    46,  -459,
     958,   958,  -459,  -459,   958,  -459,  -459,   958,   -15,   307,
     340,  -459,   307,   158,   307,  -459,  -459,  1620,  1620,  1544,
    1580,   827,    42,   143,   521,   827,    45,  -459,    48,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,    61,  -459,   307,   307,  -459,  -459,  -459,
       6,  -459,   958,   958,   958,   958,  -459,  -459,    32,  1026,
      46,    46,  -459,   303,  1447,  1468,  1508,  -459,   958,   145,
    1544,   679,   410,  1544,  -459,   827,   827,   281,  -459,  -459,
    -459,  1526,  -459,  -459,   139,   139,  -459,  1544,  -459,  -459,
    -459,  1427,   307,    39,  -459,    37,  -459,    46,    46,  -459,
    -459,  -459,   139,  -459,  -459,  -459,  -459,  -459,    40,  -459,
      10,  -459,    10,    52,  -459,    65,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  1011,  -459,  -459,  -459,   303,  1488,
    -459,  -459,  -459,   958,  -459,  -459,   307,  -459,  -459,    46,
    -459,    46,    46,    46,   840,   958,  -459,  -459,  -459,  -459,
     827,  -459,   827,   293,    77,    99,   109,   111,   126,   128,
    -101,  -459,   -97,  -459,  -459,  -459,  -459,   261,    83,  -459,
     122,   272,   135,   137,   -22,  -459,   -97,   -97,  1604,  -459,
    -459,  -459,  -459,   -53,   -53,  -459,  -459,   307,  1544,   307,
    -459,  -459,   307,  -459,   307,  -459,   307,    46,  -459,  -459,
     119,  -459,  -459,  -459,  -459,  -459,  1011,   307,  -459,  -459,
     307,  1427,   307,   307,   307,  1427,  -459,    26,  -459,  -459,
    -459,   307,   124,   144,   307,  -459,  -459,   307,   307,   307,
     307,   307,   307,  -459,   410,   307,  -459,   307,    46,  -459,
    -459,  -459,  -459,  -459,  -459,   307,    46,   307,   307,   307,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,   138,   -43,  -459,
     -97,  -459,   101,   -97,   421,  -108,   -97,   -97,   580,   150,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
     117,  -459,  -459,  -459,    11,  1046,  -459,  -459,    37,    37,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,   286,   159,   160,  -459,
    -459,   153,   -97,   155,  -459,  -107,   164,   168,   172,   180,
     199,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,   216,  -459,   229,  -459,  -459,   241,  -459,
    -459,  -459,  -459,  -459,   238,  -459,  -459,  -459
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -459,  -459,  -459,   423,   277,  -459,  -459,  -459,  -459,  -459,
    -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,  -459,   495,
     294,   470,   108,  -459,    43,   -17,  -459,    44,    88,    -9,
      -6,  -321,    63,    28,   694,  -265,  -459,    25,  -286,  -458,
    -459,    53,  -100,  -459,   -76,  -459,  -459,  -459,    -1,  -459,
    -351,  -459,  -459,   234,   -39
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -191
static const yytype_int16 yytable[] =
{
     128,   168,   452,   129,   483,   128,   128,   441,   156,   162,
     163,   142,   591,   550,   517,   584,   607,   554,   187,   518,
     129,   423,   424,   198,   130,   131,   125,   129,   425,   426,
     129,   135,   136,  -185,   128,   129,   139,   141,   585,   608,
     205,   130,   131,   230,   232,   234,   236,   129,   130,   131,
     129,   130,   131,   244,  -185,  -185,   130,   131,   352,   473,
     203,   200,   202,   129,   251,   184,   465,   495,   130,   131,
     465,   130,   131,   128,   129,   577,   487,   157,   157,   157,
     165,   209,   128,   578,   130,   131,   186,   157,   303,   116,
     117,   118,   119,   120,   196,   130,   131,   594,   528,   258,
     253,   529,   254,   260,   261,   137,   264,   279,   267,   221,
     311,   129,   314,   315,   309,   129,   193,   194,   195,   237,
     129,   128,   109,   110,   111,   112,   592,   307,   336,   113,
     114,   115,   130,   131,   128,   128,   130,   131,   128,   343,
     128,   130,   131,   128,   128,   128,   351,   310,   109,   110,
     111,   112,   137,   396,   471,   113,   114,   115,   274,   143,
     318,   319,   422,   132,   320,   436,   321,   555,   437,   312,
     313,   314,   315,   344,   442,   345,   172,   137,   305,   306,
     328,   438,   555,   245,   329,   330,   331,   481,    89,   311,
     332,   333,   511,   488,   335,   312,   313,   314,   315,   128,
     362,   128,   341,   142,   491,  -190,   330,   595,   596,   312,
     313,   314,   315,   -62,   512,   347,   129,   492,   116,   117,
     118,   119,   120,   353,   513,   357,   514,   358,   121,   522,
     337,   116,   117,   118,   119,   120,   386,   130,   131,   128,
     128,   515,   523,   516,   368,   558,   370,   580,   372,   373,
     129,   375,   323,   325,   326,   526,   128,   527,   576,   128,
     128,   423,   424,   128,   205,   559,   128,   458,   425,   426,
     590,   130,   131,   555,   602,   603,   604,   393,   606,   420,
     421,   459,   399,   434,   435,   401,   402,   609,  -180,   403,
     427,   610,   404,   458,   129,   611,   405,   116,   117,   118,
     119,   120,  -180,   612,  -185,   410,   412,   459,   129,  -180,
    -180,   128,   128,   128,   128,   130,   131,   509,   613,   454,
     168,   312,   313,   314,   315,  -185,  -185,   128,   462,   130,
     131,   386,   470,  -178,   476,   468,   614,   443,   444,   445,
     446,  -167,   413,   116,   117,   118,   119,   120,   388,   389,
     479,   143,   615,   457,  -178,  -178,   270,   617,   109,   110,
     111,   112,  -167,  -167,   616,   113,   114,   115,   214,   137,
     308,   116,   117,   118,   119,   120,   269,   130,   131,   423,
     424,   469,   449,   497,   546,   466,   425,   426,   157,   456,
     423,   424,   128,   463,   567,   505,   467,   425,   426,   537,
       0,     0,  -190,   597,   128,   598,     0,   137,   520,   507,
     474,   508,   494,   109,   110,   111,   112,     0,   498,   524,
     113,   114,   115,   137,   599,   600,   601,     0,   134,     0,
     506,   138,   140,     0,     0,     0,   121,   536,     0,     0,
     164,   167,   173,   175,   180,   183,   185,     0,     0,     0,
     190,   192,   157,     0,   476,   197,   199,   201,   476,   462,
     206,   208,     0,     0,   212,     0,   215,     0,     0,     0,
     479,   224,     0,   227,   479,   229,   231,   233,   235,   222,
     238,   145,   146,     0,   242,   243,   246,   247,   248,     0,
     250,     0,     0,   386,   494,   188,   189,     0,   259,     0,
     262,   263,   265,   266,     0,     0,   271,   275,   276,     0,
     121,   539,     0,     0,   217,   219,     0,     0,     0,   226,
       0,   228,   122,     0,   109,   110,   111,   112,     0,   240,
     241,   113,   114,   115,   210,   211,   249,   213,   476,   423,
     424,   255,   256,   257,     0,     0,   425,   426,   316,     0,
       0,   268,     0,   239,   479,   317,   277,   278,   383,     0,
       0,   129,     0,  -190,  -190,  -190,  -190,     0,   582,     0,
    -190,  -190,  -190,     0,     0,   327,  -190,  -190,  -190,  -190,
    -190,     0,   130,   131,     0,   129,     0,   147,   334,     0,
     148,     0,   338,     0,   339,   340,   149,   150,   151,   342,
     116,   117,   118,   119,   120,     0,   130,   131,     0,   346,
       0,   348,   349,     0,     0,     0,   350,     0,     0,   354,
       0,   356,     0,     0,     0,     0,   359,   360,     0,     0,
     361,     0,     0,   363,   364,     0,   365,   366,   430,   367,
     369,   431,   371,     0,     0,   374,     0,     0,   376,   377,
     432,   378,   433,   379,     0,   380,   519,   381,     0,   382,
       0,   521,   387,     0,     0,   525,     0,   390,   391,     0,
     530,   531,   392,     0,     0,   394,   395,     0,   397,   398,
     137,   400,   109,   110,   111,   112,     0,     0,   464,   113,
     114,   115,   406,     0,     0,   407,     0,   408,   423,   424,
     409,   411,     0,     0,     0,   425,   426,   588,   158,   158,
     158,   166,   171,   191,   158,  -190,   148,   166,   158,     0,
       0,     0,   149,   150,   151,   166,   158,  -190,   439,   440,
     204,     0,     0,     0,   130,   131,     0,   216,   218,   220,
     166,   223,   225,   450,   451,     0,   158,   158,   158,   158,
     166,     0,     0,     0,   579,     0,   158,   581,   583,     0,
     586,   587,   589,   116,   117,   118,   119,   120,   322,     0,
     109,   110,   111,   112,     0,   480,     0,   113,   114,   115,
     485,   486,     0,   116,   117,   118,   119,   120,     0,     0,
       0,     0,     0,   324,     0,   109,   110,   111,   112,     0,
       0,     0,   113,   114,   115,     0,     0,   493,   116,   117,
     118,   119,   120,     0,     0,     0,   605,     0,     0,   499,
       0,     0,   500,     0,   501,   502,   503,   504,     0,     0,
     109,   110,   111,   112,     0,     0,   510,   113,   114,   115,
     489,   129,   490,   109,   110,   111,   112,     0,     0,   154,
     113,   114,   115,     0,     0,     0,   116,   117,   118,   119,
     120,     0,   130,   131,     0,     0,     0,     0,     0,     0,
     538,     0,   540,     0,     0,   541,   158,   542,   158,   543,
     544,     0,     0,   545,     0,     0,     0,     0,     0,   547,
     548,     0,     0,   549,     0,   551,   552,   553,     0,     0,
     556,     0,     0,   158,   557,     0,     0,   560,     0,     0,
     561,   562,   563,   564,   565,   566,     0,     0,   568,     0,
     569,   570,   121,     0,     0,     0,     0,     0,   571,   572,
     573,   574,   575,     0,   122,     0,     0,     0,     0,     0,
       0,   109,   110,   111,   112,     0,     0,   121,   113,   114,
     115,     0,     0,     0,   116,   117,   118,   119,   120,   122,
       0,   109,   110,   111,   112,     0,     0,     0,   113,   114,
     115,     0,     0,   166,   116,   117,   118,   119,   120,   129,
       0,   109,   110,   111,   112,     0,     0,   154,   113,   114,
     115,     0,   155,     0,   116,   117,   118,   119,   120,     0,
     130,   131,     0,   312,   313,   314,   315,     0,     0,     0,
       0,     0,   129,   166,  -190,  -190,  -190,  -190,     0,   158,
     166,  -190,  -190,  -190,   166,     0,     0,   166,     0,   109,
     110,   111,   112,   130,   131,   158,   113,   114,   115,     0,
       0,   166,   116,   117,   118,   119,   120,   593,     0,   109,
     110,   111,   112,     0,     0,     0,   113,   114,   115,     0,
       0,     0,   116,   117,   118,   119,   120,     0,     0,   129,
       0,  -171,  -171,  -171,  -171,     0,     0,  -171,  -171,  -171,
    -171,     0,   179,   158,  -171,  -171,  -171,  -171,  -171,     0,
     130,   131,     0,   121,     0,     0,     0,     0,   158,     0,
       0,   312,   313,   314,   315,   122,     0,     0,     0,     0,
       0,     0,     0,   121,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   122,     0,     0,     0,     0,
     155,   137,     0,     0,     0,   174,     0,     0,     0,     0,
       0,     0,   166,     0,     0,     0,   129,     0,   109,   110,
     111,   112,     0,     0,   154,   113,   114,   115,     0,  -190,
     475,   116,   117,   118,   119,   120,     0,   130,   131,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   155,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     0,   312,
     313,   314,   315,     2,     0,     0,     0,     0,     0,     0,
       0,   121,     3,     0,     0,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
    -171,     0,    19,    20,    21,    22,    23,     0,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,     0,
      85,    86,    87,    88,     0,     0,     0,   155,   129,     0,
     109,   110,   111,   112,     0,     0,     0,   113,   114,   115,
       0,     0,     0,   116,   117,   118,   119,   120,     0,   130,
     131,     0,     0,     1,     0,     0,     0,     0,     0,     2,
       0,    89,     0,     0,     0,     0,     0,     0,     0,    90,
      91,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,     0,     0,    19,    20,
      21,    22,    23,     0,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,     0,    85,    86,    87,    88,
     109,   110,   111,   112,     0,     0,     0,   113,   114,   115,
       0,     0,     0,   116,   117,   118,   119,   120,  -167,   155,
    -167,  -167,  -167,  -167,     0,     0,  -167,  -167,  -167,  -167,
       0,     0,     0,  -167,  -167,  -167,  -167,  -167,     0,   453,
       0,   109,   110,   111,   112,    90,    91,   154,   113,   114,
     115,     0,     0,     0,   116,   117,   118,   119,   120,   496,
       0,   109,   110,   111,   112,     0,     0,   154,   113,   114,
     115,     0,     0,     0,   116,   117,   118,   119,   120,   455,
       0,   109,   110,   111,   112,     0,     0,     0,   113,   114,
     115,     0,     0,     0,   116,   117,   118,   119,   120,   109,
     110,   111,   112,     0,     0,   154,   113,   114,   115,     0,
       0,   475,   116,   117,   118,   119,   120,   109,   110,   111,
     112,     0,     0,     0,   113,   114,   115,     0,     0,     0,
     116,   117,   118,   119,   120,     0,     0,     0,     0,     0,
       0,   109,   110,   111,   112,     0,     0,     0,   113,   114,
     115,     0,   121,   109,   110,   111,   112,     0,     0,     0,
     113,   114,   115,     0,     0,     0,     0,     0,     0,  -167,
       0,     0,     0,     0,     0,     0,     0,   109,   110,   111,
     112,     0,     0,     0,   113,   114,   115,     0,     0,     0,
     155,   129,     0,  -190,  -190,  -190,  -190,     0,     0,     0,
    -190,  -190,  -190,     0,     0,     0,     0,     0,     0,     0,
     155,     0,   130,   131,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   155,     0,
       0,     0,   280,     0,   281,   282,     0,     0,   283,     0,
     284,     0,   285,     0,     0,     0,   155,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,     0,     0,   414,   415,   416,   417,
     418,   419,   532,   533,     0,     0,     0,     0,     0,   425,
     534,   535,     0,     0,     0,     0,     0,     0,     0,     0,
     137
};

static const yytype_int16 yycheck[] =
{
       1,    18,   323,     1,   355,     6,     7,     1,    14,    15,
      16,     1,     1,   471,   115,   123,   123,   475,    24,   120,
       1,   118,   119,    32,    22,    23,     1,     1,   125,   126,
       1,     6,     7,     1,    35,     1,     8,     9,   146,   146,
     150,    22,    23,    52,    53,    54,    55,     1,    22,    23,
       1,    22,    23,    62,    22,    23,    22,    23,     1,   345,
      35,    33,    34,     1,   150,    22,   331,   388,    22,    23,
     335,    22,    23,    74,     1,   118,   362,    14,    15,    16,
      17,    38,    83,   126,    22,    23,    23,    24,    89,    16,
      17,    18,    19,    20,    31,    22,    23,   555,   120,    74,
     148,   123,   148,    75,    76,   120,    78,    21,    80,    46,
     155,     1,   165,   166,    22,     1,    28,    29,    30,    56,
       1,   122,     3,     4,     5,     6,   115,     0,    21,    10,
      11,    12,    22,    23,   135,   136,    22,    23,   139,    14,
     141,    22,    23,   144,   145,   146,    15,   122,     3,     4,
       5,     6,   120,   148,    15,    10,    11,    12,   109,   149,
     135,   136,   120,   161,   139,   120,   141,   156,   120,   163,
     164,   165,   166,   182,   168,   184,   157,   120,    90,    91,
     152,   120,   156,   154,   156,   157,   158,   148,   151,   155,
     162,   163,   115,   153,   166,   163,   164,   165,   166,   200,
     209,   202,   174,     1,   152,   148,   178,   558,   559,   163,
     164,   165,   166,   151,   115,   187,     1,   152,    16,    17,
      18,    19,    20,   195,   115,   200,   115,   202,   155,   146,
     120,    16,    17,    18,    19,    20,   237,    22,    23,   240,
     241,   115,   120,   115,   216,   121,   218,   146,   220,   221,
       1,   223,   144,   145,   146,   120,   257,   120,   120,   260,
     261,   118,   119,   264,   150,   121,   267,   148,   125,   126,
     120,    22,    23,   156,   115,   115,   123,   249,   123,   280,
     281,   162,   257,   284,   285,   260,   261,   123,     1,   264,
     147,   123,   267,   148,     1,   123,   268,    16,    17,    18,
      19,    20,    15,   123,     1,   277,   278,   162,     1,    22,
      23,   312,   313,   314,   315,    22,    23,    24,   119,   325,
     337,   163,   164,   165,   166,    22,    23,   328,   329,    22,
      23,   332,   341,     1,   351,   336,   120,   312,   313,   314,
     315,     1,   279,    16,    17,    18,    19,    20,   240,   241,
     351,   149,   123,   328,    22,    23,     1,   119,     3,     4,
       5,     6,    22,    23,   123,    10,    11,    12,   153,   120,
      93,    16,    17,    18,    19,    20,    82,    22,    23,   118,
     119,   337,   319,   389,   460,   332,   125,   126,   325,   326,
     118,   119,   393,   330,   494,   404,   333,   125,   126,   438,
      -1,    -1,   153,   117,   405,   119,    -1,   120,   147,   410,
     347,   412,   384,     3,     4,     5,     6,    -1,   393,   147,
      10,    11,    12,   120,   138,   139,   140,    -1,     5,    -1,
     405,     8,     9,    -1,    -1,    -1,   155,   438,    -1,    -1,
      17,    18,    19,    20,    21,    22,    23,    -1,    -1,    -1,
      27,    28,   389,    -1,   471,    32,    33,    34,   475,   460,
      37,    38,    -1,    -1,    41,    -1,    43,    -1,    -1,    -1,
     471,    48,    -1,    50,   475,    52,    53,    54,    55,   152,
      57,    11,    12,    -1,    61,    62,    63,    64,    65,    -1,
      67,    -1,    -1,   494,   466,    25,    26,    -1,    75,    -1,
      77,    78,    79,    80,    -1,    -1,    83,    84,    85,    -1,
     155,   448,    -1,    -1,    44,    45,    -1,    -1,    -1,    49,
      -1,    51,   167,    -1,     3,     4,     5,     6,    -1,    59,
      60,    10,    11,    12,    39,    40,    66,    42,   555,   118,
     119,    71,    72,    73,    -1,    -1,   125,   126,   125,    -1,
      -1,    81,    -1,    58,   555,   132,    86,    87,   148,    -1,
      -1,     1,    -1,     3,     4,     5,     6,    -1,   147,    -1,
      10,    11,    12,    -1,    -1,   152,    16,    17,    18,    19,
      20,    -1,    22,    23,    -1,     1,    -1,     1,   165,    -1,
       4,    -1,   169,    -1,   171,   172,    10,    11,    12,   176,
      16,    17,    18,    19,    20,    -1,    22,    23,    -1,   186,
      -1,   188,   189,    -1,    -1,    -1,   193,    -1,    -1,   196,
      -1,   198,    -1,    -1,    -1,    -1,   203,   204,    -1,    -1,
     207,    -1,    -1,   210,   211,    -1,   213,   214,   117,   216,
     217,   120,   219,    -1,    -1,   222,    -1,    -1,   225,   226,
     129,   228,   131,   230,    -1,   232,   422,   234,    -1,   236,
      -1,   427,   239,    -1,    -1,   431,    -1,   244,   245,    -1,
     436,   437,   249,    -1,    -1,   252,   253,    -1,   255,   256,
     120,   258,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,   269,    -1,    -1,   272,    -1,   274,   118,   119,
     277,   278,    -1,    -1,    -1,   125,   126,   127,    14,    15,
      16,    17,    18,     1,    20,   155,     4,    23,    24,    -1,
      -1,    -1,    10,    11,    12,    31,    32,   167,   305,   306,
      36,    -1,    -1,    -1,    22,    23,    -1,    43,    44,    45,
      46,    47,    48,   320,   321,    -1,    52,    53,    54,    55,
      56,    -1,    -1,    -1,   520,    -1,    62,   523,   524,    -1,
     526,   527,   528,    16,    17,    18,    19,    20,     1,    -1,
       3,     4,     5,     6,    -1,   352,    -1,    10,    11,    12,
     357,   358,    -1,    16,    17,    18,    19,    20,    -1,    -1,
      -1,    -1,    -1,     1,    -1,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,    -1,    -1,   384,    16,    17,
      18,    19,    20,    -1,    -1,    -1,   582,    -1,    -1,   396,
      -1,    -1,   399,    -1,   401,   402,   403,   404,    -1,    -1,
       3,     4,     5,     6,    -1,    -1,   413,    10,    11,    12,
     370,     1,   372,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
     447,    -1,   449,    -1,    -1,   452,   182,   454,   184,   456,
     457,    -1,    -1,   460,    -1,    -1,    -1,    -1,    -1,   466,
     467,    -1,    -1,   470,    -1,   472,   473,   474,    -1,    -1,
     477,    -1,    -1,   209,   481,    -1,    -1,   484,    -1,    -1,
     487,   488,   489,   490,   491,   492,    -1,    -1,   495,    -1,
     497,   498,   155,    -1,    -1,    -1,    -1,    -1,   505,   506,
     507,   508,   509,    -1,   167,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,   155,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,   167,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,    -1,   279,    16,    17,    18,    19,    20,     1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,   152,    -1,    16,    17,    18,    19,    20,    -1,
      22,    23,    -1,   163,   164,   165,   166,    -1,    -1,    -1,
      -1,    -1,     1,   319,     3,     4,     5,     6,    -1,   325,
     326,    10,    11,    12,   330,    -1,    -1,   333,    -1,     3,
       4,     5,     6,    22,    23,   341,    10,    11,    12,    -1,
      -1,   347,    16,    17,    18,    19,    20,     1,    -1,     3,
       4,     5,     6,    -1,    -1,    -1,    10,    11,    12,    -1,
      -1,    -1,    16,    17,    18,    19,    20,    -1,    -1,     1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,    14,   389,    16,    17,    18,    19,    20,    -1,
      22,    23,    -1,   155,    -1,    -1,    -1,    -1,   404,    -1,
      -1,   163,   164,   165,   166,   167,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   155,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   167,    -1,    -1,    -1,    -1,
     152,   120,    -1,    -1,    -1,   157,    -1,    -1,    -1,    -1,
      -1,    -1,   448,    -1,    -1,    -1,     1,    -1,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    -1,   148,
     114,    16,    17,    18,    19,    20,    -1,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   152,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     7,    -1,   163,
     164,   165,   166,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   155,    22,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
     152,    -1,    42,    43,    44,    45,    46,    -1,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,    -1,
     110,   111,   112,   113,    -1,    -1,    -1,   152,     1,    -1,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,    22,
      23,    -1,    -1,     7,    -1,    -1,    -1,    -1,    -1,    13,
      -1,   151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   159,
     160,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    -1,    -1,    42,    43,
      44,    45,    46,    -1,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,    -1,   110,   111,   112,   113,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,     1,   152,
       3,     4,     5,     6,    -1,    -1,     9,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,     1,
      -1,     3,     4,     5,     6,   159,   160,     9,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,     1,
      -1,     3,     4,     5,     6,    -1,    -1,     9,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,     1,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,     3,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,    -1,
      -1,   114,    16,    17,    18,    19,    20,     3,     4,     5,
       6,    -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,
      16,    17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,   155,     3,     4,     5,     6,    -1,    -1,    -1,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,   152,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
       6,    -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,
     152,     1,    -1,     3,     4,     5,     6,    -1,    -1,    -1,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     152,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     152,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   152,    -1,
      -1,    -1,   114,    -1,   116,   117,    -1,    -1,   120,    -1,
     122,    -1,   124,    -1,    -1,    -1,   152,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,    -1,    -1,   136,   137,   138,   139,
     140,   141,   118,   119,    -1,    -1,    -1,    -1,    -1,   125,
     126,   127,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     120
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    13,    22,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    42,
      43,    44,    45,    46,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   110,   111,   112,   113,   151,
     159,   160,   170,   171,   173,   174,   175,   176,   177,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   219,     3,
       4,     5,     6,    10,    11,    12,    16,    17,    18,    19,
      20,   155,   167,   194,   203,   206,   214,   216,   217,     1,
      22,    23,   161,   172,   172,   206,   206,   120,   172,   202,
     172,   202,     1,   149,   190,   190,   190,     1,     4,    10,
      11,    12,   197,   215,     9,   152,   199,   201,   203,   204,
     205,   217,   199,   199,   172,   201,   203,   172,   194,   195,
     196,   203,   157,   172,   157,   172,   198,   199,   201,    14,
     172,   192,   193,   172,   193,   172,   201,   199,   190,   190,
     172,     1,   172,   197,   197,   197,   201,   172,   198,   172,
     202,   172,   202,   206,   203,   150,   172,   188,   172,   193,
     188,   188,   172,   188,   153,   172,   203,   190,   203,   190,
     203,   201,   152,   203,   172,   203,   190,   172,   190,   172,
     198,   172,   198,   172,   198,   172,   198,   201,   172,   188,
     190,   190,   172,   172,   198,   154,   172,   172,   172,   190,
     172,   150,   189,   148,   148,   190,   190,   190,   206,   172,
     202,   202,   172,   172,   202,   172,   172,   202,   190,   189,
       1,   172,   191,   206,   109,   172,   172,   190,   190,    21,
     114,   116,   117,   120,   122,   124,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   217,   221,   197,   197,     0,   173,    22,
     206,   155,   163,   164,   165,   166,   172,   172,   206,   206,
     206,   206,     1,   191,     1,   191,   191,   172,   202,   202,
     202,   202,   202,   202,   172,   202,    21,   120,   172,   172,
     172,   202,   172,    14,   198,   198,   172,   202,   172,   172,
     172,    15,     1,   202,   172,   178,   172,   206,   206,   172,
     172,   172,   198,   172,   172,   172,   172,   172,   202,   172,
     202,   172,   202,   202,   172,   202,   172,   172,   172,   172,
     172,   172,   172,   148,   210,   211,   217,   172,   191,   191,
     172,   172,   172,   202,   172,   172,   148,   172,   172,   206,
     172,   206,   206,   206,   206,   202,   172,   172,   172,   172,
     202,   172,   202,   201,   136,   137,   138,   139,   140,   141,
     217,   217,   120,   118,   119,   125,   126,   147,   222,   223,
     117,   120,   129,   131,   217,   217,   120,   120,   120,   172,
     172,     1,   168,   206,   206,   206,   206,   200,   202,   201,
     172,   172,   200,     1,   199,     1,   201,   206,   148,   162,
     212,   213,   217,   201,     9,   204,   210,   201,   217,   196,
     198,    15,   207,   207,   201,   114,   194,   208,   209,   217,
     172,   148,   218,   219,   220,   172,   172,   207,   153,   190,
     190,   152,   152,   172,   202,   200,     1,   199,   206,   172,
     172,   172,   172,   172,   172,   198,   206,   217,   217,    24,
     172,   115,   115,   115,   115,   115,   115,   115,   120,   222,
     147,   222,   146,   120,   147,   222,   120,   120,   120,   123,
     222,   222,   118,   119,   126,   127,   217,   223,   172,   201,
     172,   172,   172,   172,   172,   172,   213,   172,   172,   172,
     208,   172,   172,   172,   208,   156,   172,   172,   121,   121,
     172,   172,   172,   172,   172,   172,   172,   211,   172,   172,
     172,   172,   172,   172,   172,   172,   120,   118,   126,   222,
     146,   222,   147,   222,   123,   146,   222,   222,   127,   222,
     120,     1,   115,     1,   208,   219,   219,   117,   119,   138,
     139,   140,   115,   115,   123,   222,   123,   123,   146,   123,
     123,   123,   123,   119,   120,   123,   123,   119
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1464 of yacc.c  */
#line 193 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 3:

/* Line 1464 of yacc.c  */
#line 194 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 195 "mon_parse.y"
    { new_cmd = 1; asm_mode = 0;  (yyval.i) = 0; }
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 204 "mon_parse.y"
    { return ERR_EXPECT_END_CMD; }
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 219 "mon_parse.y"
    { return ERR_BAD_CMD; }
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 223 "mon_parse.y"
    { mon_bank(e_default_space, NULL); }
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 225 "mon_parse.y"
    { mon_bank((yyvsp[(2) - (3)].i), NULL); }
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 227 "mon_parse.y"
    { mon_bank(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 229 "mon_parse.y"
    { mon_bank((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 231 "mon_parse.y"
    { mon_jump((yyvsp[(2) - (3)].a)); }
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 233 "mon_parse.y"
    { mon_go(); }
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 235 "mon_parse.y"
    { mon_display_io_regs(0); }
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 237 "mon_parse.y"
    { mon_display_io_regs((yyvsp[(2) - (3)].a)); }
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 239 "mon_parse.y"
    { monitor_cpu_type_set(""); }
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 241 "mon_parse.y"
    { monitor_cpu_type_set((yyvsp[(2) - (3)].str)); }
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 243 "mon_parse.y"
    { mon_cpuhistory(-1); }
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 245 "mon_parse.y"
    { mon_cpuhistory((yyvsp[(3) - (4)].i)); }
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 247 "mon_parse.y"
    { mon_instruction_return(); }
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 249 "mon_parse.y"
    { machine_write_snapshot((yyvsp[(2) - (3)].str),0,0,0); /* FIXME */ }
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 251 "mon_parse.y"
    { machine_read_snapshot((yyvsp[(2) - (3)].str), 0); }
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 253 "mon_parse.y"
    { mon_instructions_step(-1); }
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 255 "mon_parse.y"
    { mon_instructions_step((yyvsp[(3) - (4)].i)); }
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 257 "mon_parse.y"
    { mon_instructions_next(-1); }
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 259 "mon_parse.y"
    { mon_instructions_next((yyvsp[(3) - (4)].i)); }
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 261 "mon_parse.y"
    { mon_stack_up(-1); }
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 263 "mon_parse.y"
    { mon_stack_up((yyvsp[(3) - (4)].i)); }
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 265 "mon_parse.y"
    { mon_stack_down(-1); }
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 267 "mon_parse.y"
    { mon_stack_down((yyvsp[(3) - (4)].i)); }
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 269 "mon_parse.y"
    { mon_display_screen(); }
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 274 "mon_parse.y"
    { (monitor_cpu_for_memspace[default_memspace]->mon_register_print)(default_memspace); }
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 276 "mon_parse.y"
    { (monitor_cpu_for_memspace[(yyvsp[(2) - (3)].i)]->mon_register_print)((yyvsp[(2) - (3)].i)); }
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 281 "mon_parse.y"
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[(4) - (5)].str));
                    }
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 286 "mon_parse.y"
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[(2) - (3)].str));
                    }
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 291 "mon_parse.y"
    { mon_save_symbols((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 293 "mon_parse.y"
    { mon_save_symbols(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 295 "mon_parse.y"
    { mon_add_name_to_symbol_table((yyvsp[(2) - (5)].a), (yyvsp[(4) - (5)].str)); }
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 297 "mon_parse.y"
    { mon_remove_name_from_symbol_table(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 299 "mon_parse.y"
    { mon_remove_name_from_symbol_table((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 301 "mon_parse.y"
    { mon_print_symbol_table((yyvsp[(2) - (3)].i)); }
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 303 "mon_parse.y"
    { mon_print_symbol_table(e_default_space); }
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 305 "mon_parse.y"
    {
                        mon_add_name_to_symbol_table((yyvsp[(3) - (4)].a), mon_prepend_dot_to_name((yyvsp[(1) - (4)].str)));
                    }
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 309 "mon_parse.y"
    {
                        mon_add_name_to_symbol_table((yyvsp[(3) - (5)].a), mon_prepend_dot_to_name((yyvsp[(1) - (5)].str)));
                    }
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 315 "mon_parse.y"
    { mon_start_assemble_mode((yyvsp[(2) - (2)].a), NULL); }
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 316 "mon_parse.y"
    { }
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 318 "mon_parse.y"
    { mon_start_assemble_mode((yyvsp[(2) - (3)].a), NULL); }
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 320 "mon_parse.y"
    { mon_disassemble_lines((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1]); }
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 322 "mon_parse.y"
    { mon_disassemble_lines(BAD_ADDR, BAD_ADDR); }
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 326 "mon_parse.y"
    { mon_memory_move((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1], (yyvsp[(4) - (5)].a)); }
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 328 "mon_parse.y"
    { mon_memory_compare((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1], (yyvsp[(4) - (5)].a)); }
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 330 "mon_parse.y"
    { mon_memory_fill((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1],(unsigned char *)(yyvsp[(4) - (5)].str)); }
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 332 "mon_parse.y"
    { mon_memory_hunt((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1],(unsigned char *)(yyvsp[(4) - (5)].str)); }
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 334 "mon_parse.y"
    { mon_memory_display((yyvsp[(2) - (5)].rt), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], DF_PETSCII); }
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 336 "mon_parse.y"
    { mon_memory_display(default_radix, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_PETSCII); }
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 338 "mon_parse.y"
    { mon_memory_display(default_radix, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 340 "mon_parse.y"
    { mon_memory_display_data((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], 8, 8); }
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 342 "mon_parse.y"
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 8, 8); }
    break;

  case 76:

/* Line 1464 of yacc.c  */
#line 344 "mon_parse.y"
    { mon_memory_display_data((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], 24, 21); }
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 346 "mon_parse.y"
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 24, 21); }
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 348 "mon_parse.y"
    { mon_memory_display(0, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_PETSCII); }
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 350 "mon_parse.y"
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 352 "mon_parse.y"
    { mon_memory_display(0, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_SCREEN_CODE); }
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 354 "mon_parse.y"
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_SCREEN_CODE); }
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 356 "mon_parse.y"
    { mon_memmap_zap(); }
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 358 "mon_parse.y"
    { mon_memmap_show(-1,BAD_ADDR,BAD_ADDR); }
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 360 "mon_parse.y"
    { mon_memmap_show((yyvsp[(3) - (4)].i),BAD_ADDR,BAD_ADDR); }
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 362 "mon_parse.y"
    { mon_memmap_show((yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].range)[0],(yyvsp[(4) - (5)].range)[1]); }
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 364 "mon_parse.y"
    { mon_memmap_save((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i)); }
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 368 "mon_parse.y"
    {
                      if ((yyvsp[(2) - (5)].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], TRUE, (yyvsp[(2) - (5)].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], TRUE, e_exec, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[(4) - (5)].cond_node));
                  }
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 377 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 380 "mon_parse.y"
    {
                      mon_breakpoint_add_checkpoint((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], TRUE, e_exec, TRUE);
                  }
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 384 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 387 "mon_parse.y"
    {
                      if ((yyvsp[(2) - (5)].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], TRUE, (yyvsp[(2) - (5)].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], TRUE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[(4) - (5)].cond_node));
                  }
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 396 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 399 "mon_parse.y"
    {
                      if ((yyvsp[(2) - (5)].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], FALSE, (yyvsp[(2) - (5)].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[(3) - (5)].range)[0], (yyvsp[(3) - (5)].range)[1], FALSE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[(4) - (5)].cond_node));
                  }
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 408 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 413 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_ON, (yyvsp[(2) - (3)].i)); }
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 415 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_OFF, (yyvsp[(2) - (3)].i)); }
    break;

  case 97:

/* Line 1464 of yacc.c  */
#line 417 "mon_parse.y"
    { mon_breakpoint_set_ignore_count((yyvsp[(2) - (3)].i), -1); }
    break;

  case 98:

/* Line 1464 of yacc.c  */
#line 419 "mon_parse.y"
    { mon_breakpoint_set_ignore_count((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i)); }
    break;

  case 99:

/* Line 1464 of yacc.c  */
#line 421 "mon_parse.y"
    { mon_breakpoint_delete_checkpoint((yyvsp[(2) - (3)].i)); }
    break;

  case 100:

/* Line 1464 of yacc.c  */
#line 423 "mon_parse.y"
    { mon_breakpoint_delete_checkpoint(-1); }
    break;

  case 101:

/* Line 1464 of yacc.c  */
#line 425 "mon_parse.y"
    { mon_breakpoint_set_checkpoint_condition((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].cond_node)); }
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 427 "mon_parse.y"
    { mon_breakpoint_set_checkpoint_command((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 429 "mon_parse.y"
    { return ERR_EXPECT_STRING; }
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 433 "mon_parse.y"
    { sidefx = (((yyvsp[(2) - (3)].action) == e_TOGGLE) ? (sidefx ^ 1) : (yyvsp[(2) - (3)].action)); }
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 435 "mon_parse.y"
    {
                         mon_out("I/O side effects are %s\n",
                                   sidefx ? "enabled" : "disabled");
                     }
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 440 "mon_parse.y"
    { default_radix = (yyvsp[(2) - (3)].rt); }
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 442 "mon_parse.y"
    {
                         const char *p;

                         if (default_radix == e_hexadecimal)
                             p = "Hexadecimal";
                         else if (default_radix == e_decimal)
                             p = "Decimal";
                         else if (default_radix == e_octal)
                             p = "Octal";
                         else if (default_radix == e_binary)
                             p = "Binary";
                         else
                             p = "Unknown";

                         mon_out("Default radix is %s\n", p);
                     }
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 460 "mon_parse.y"
    { monitor_change_device((yyvsp[(2) - (3)].i)); }
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 462 "mon_parse.y"
    { mon_export(); }
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 464 "mon_parse.y"
    { mon_quit(); YYACCEPT; }
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 466 "mon_parse.y"
    { mon_exit(); YYACCEPT; }
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 470 "mon_parse.y"
    { mon_drive_execute_disk_cmd((yyvsp[(2) - (3)].str)); }
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 472 "mon_parse.y"
    { mon_out("\t%d\n",(yyvsp[(2) - (3)].i)); }
    break;

  case 114:

/* Line 1464 of yacc.c  */
#line 474 "mon_parse.y"
    { mon_command_print_help(NULL); }
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 476 "mon_parse.y"
    { mon_command_print_help((yyvsp[(2) - (3)].str)); }
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 478 "mon_parse.y"
    { printf("SYSTEM COMMAND: %s\n",(yyvsp[(2) - (3)].str)); }
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 480 "mon_parse.y"
    { mon_print_convert((yyvsp[(2) - (3)].i)); }
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 482 "mon_parse.y"
    { mon_change_dir((yyvsp[(2) - (3)].str)); }
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 484 "mon_parse.y"
    { mon_keyboard_feed((yyvsp[(2) - (3)].str)); }
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 486 "mon_parse.y"
    { mon_backtrace(); }
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 488 "mon_parse.y"
    { mon_show_dir((yyvsp[(2) - (3)].str)); }
    break;

  case 122:

/* Line 1464 of yacc.c  */
#line 490 "mon_parse.y"
    { mon_show_pwd(); }
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 492 "mon_parse.y"
    { mon_screenshot_save((yyvsp[(2) - (3)].str),-1); }
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 494 "mon_parse.y"
    { mon_screenshot_save((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i)); }
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 496 "mon_parse.y"
    { mon_resource_get((yyvsp[(2) - (3)].str)); }
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 498 "mon_parse.y"
    { mon_resource_set((yyvsp[(2) - (4)].str),(yyvsp[(3) - (4)].str)); }
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 500 "mon_parse.y"
    { resources_load((yyvsp[(2) - (3)].str)); }
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 502 "mon_parse.y"
    { resources_save((yyvsp[(2) - (3)].str)); }
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 504 "mon_parse.y"
    { mon_reset_machine(-1); }
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 506 "mon_parse.y"
    { mon_reset_machine((yyvsp[(3) - (4)].i)); }
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 508 "mon_parse.y"
    { mon_tape_ctrl((yyvsp[(3) - (4)].i)); }
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 510 "mon_parse.y"
    { mon_cart_freeze(); }
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 512 "mon_parse.y"
    { }
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 514 "mon_parse.y"
    { mon_stopwatch_reset(); }
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 516 "mon_parse.y"
    { mon_stopwatch_show("Stopwatch: ", "\n"); }
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 520 "mon_parse.y"
    { mon_file_load((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].a), FALSE); }
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 522 "mon_parse.y"
    { mon_file_load((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].a), TRUE); }
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 524 "mon_parse.y"
    { mon_file_save((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], FALSE); }
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 526 "mon_parse.y"
    { return ERR_EXPECT_DEVICE_NUM; }
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 528 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 530 "mon_parse.y"
    { mon_file_save((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], TRUE); }
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 532 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 534 "mon_parse.y"
    { mon_file_verify((yyvsp[(2) - (5)].str),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 536 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 538 "mon_parse.y"
    { mon_drive_block_cmd(0,(yyvsp[(2) - (5)].i),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 540 "mon_parse.y"
    { mon_drive_block_cmd(1,(yyvsp[(2) - (5)].i),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 542 "mon_parse.y"
    { mon_drive_list(-1); }
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 544 "mon_parse.y"
    { mon_drive_list((yyvsp[(2) - (3)].i)); }
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 546 "mon_parse.y"
    { mon_attach((yyvsp[(2) - (4)].str),(yyvsp[(3) - (4)].i)); }
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 548 "mon_parse.y"
    { mon_detach((yyvsp[(2) - (3)].i)); }
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 550 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (3)].str),0,1); }
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 552 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i),1); }
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 554 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (3)].str),0,0); }
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 556 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i),0); }
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 560 "mon_parse.y"
    { mon_record_commands((yyvsp[(2) - (3)].str)); }
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 562 "mon_parse.y"
    { mon_end_recording(); }
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 564 "mon_parse.y"
    { mon_playback_init((yyvsp[(2) - (3)].str)); }
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 568 "mon_parse.y"
    { mon_memory_fill((yyvsp[(2) - (4)].a), BAD_ADDR, (unsigned char *)(yyvsp[(3) - (4)].str)); }
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 570 "mon_parse.y"
    { printf("Not yet.\n"); }
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 574 "mon_parse.y"
    { yydebug = 1; }
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 577 "mon_parse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 580 "mon_parse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 163:

/* Line 1464 of yacc.c  */
#line 581 "mon_parse.y"
    { (yyval.str) = NULL; }
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 585 "mon_parse.y"
    { return ERR_EXPECT_FILENAME; }
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 589 "mon_parse.y"
    { return ERR_EXPECT_DEVICE_NUM; }
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 592 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (2)].i) | (yyvsp[(2) - (2)].i); }
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 593 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 596 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 597 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 600 "mon_parse.y"
    { (yyval.i) = new_reg(default_memspace, (yyvsp[(1) - (1)].reg)); }
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 601 "mon_parse.y"
    { (yyval.i) = new_reg((yyvsp[(1) - (2)].i), (yyvsp[(2) - (2)].reg)); }
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 609 "mon_parse.y"
    { (monitor_cpu_for_memspace[reg_memspace((yyvsp[(1) - (3)].i))]->mon_register_set_val)(reg_memspace((yyvsp[(1) - (3)].i)), reg_regid((yyvsp[(1) - (3)].i)), (WORD) (yyvsp[(3) - (3)].i)); }
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 612 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 613 "mon_parse.y"
    { return ERR_EXPECT_CHECKNUM; }
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 617 "mon_parse.y"
    { (yyval.range)[0] = (yyvsp[(1) - (1)].a); (yyval.range)[1] = BAD_ADDR; }
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 620 "mon_parse.y"
    { (yyval.range)[0] = (yyvsp[(1) - (3)].a); (yyval.range)[1] = (yyvsp[(3) - (3)].a); }
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 622 "mon_parse.y"
    { if (resolve_range(e_default_space, (yyval.range), (yyvsp[(1) - (1)].str))) return ERR_ADDR_TOO_BIG; }
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 624 "mon_parse.y"
    { if (resolve_range((yyvsp[(1) - (3)].i), (yyval.range), (yyvsp[(3) - (3)].str))) return ERR_ADDR_TOO_BIG; }
    break;

  case 184:

/* Line 1464 of yacc.c  */
#line 627 "mon_parse.y"
    { (yyval.a) = (yyvsp[(2) - (2)].a); }
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 628 "mon_parse.y"
    { (yyval.a) = BAD_ADDR; }
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 632 "mon_parse.y"
    {
             (yyval.a) = new_addr(e_default_space,(yyvsp[(1) - (1)].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 637 "mon_parse.y"
    {
             (yyval.a) = new_addr((yyvsp[(1) - (3)].i), (yyvsp[(3) - (3)].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 642 "mon_parse.y"
    {
             temp = mon_symbol_table_lookup_addr(e_default_space, (yyvsp[(1) - (1)].str));
             if (temp >= 0)
                 (yyval.a) = new_addr(e_default_space, temp);
             else
                 return ERR_UNDEFINED_LABEL;
         }
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 655 "mon_parse.y"
    { (yyval.i) = e_comp_space; }
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 656 "mon_parse.y"
    { (yyval.i) = e_disk8_space; }
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 657 "mon_parse.y"
    { (yyval.i) = e_disk9_space; }
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 658 "mon_parse.y"
    { (yyval.i) = e_disk10_space; }
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 659 "mon_parse.y"
    { (yyval.i) = e_disk11_space; }
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 662 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); if (!CHECK_ADDR((yyvsp[(1) - (1)].i))) return ERR_ADDR_TOO_BIG; }
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 664 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 666 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) + (yyvsp[(3) - (3)].i); }
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 667 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) - (yyvsp[(3) - (3)].i); }
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 668 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) * (yyvsp[(3) - (3)].i); }
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 669 "mon_parse.y"
    { (yyval.i) = ((yyvsp[(3) - (3)].i)) ? ((yyvsp[(1) - (3)].i) / (yyvsp[(3) - (3)].i)) : 1; }
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 670 "mon_parse.y"
    { (yyval.i) = (yyvsp[(2) - (3)].i); }
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 671 "mon_parse.y"
    { return ERR_MISSING_CLOSE_PAREN; }
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 672 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 675 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(2) - (2)].cond_node); }
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 676 "mon_parse.y"
    { (yyval.cond_node) = 0; }
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 679 "mon_parse.y"
    {
               (yyval.cond_node) = new_cond; (yyval.cond_node)->is_parenthized = FALSE;
               (yyval.cond_node)->child1 = (yyvsp[(1) - (3)].cond_node); (yyval.cond_node)->child2 = (yyvsp[(3) - (3)].cond_node); (yyval.cond_node)->operation = (yyvsp[(2) - (3)].cond_op);
           }
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 684 "mon_parse.y"
    { return ERR_INCOMPLETE_COMPARE_OP; }
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 686 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(2) - (3)].cond_node); (yyval.cond_node)->is_parenthized = TRUE; }
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 688 "mon_parse.y"
    { return ERR_MISSING_CLOSE_PAREN; }
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 690 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(1) - (1)].cond_node); }
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 693 "mon_parse.y"
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->reg_num = (yyvsp[(1) - (1)].i); (yyval.cond_node)->is_reg = TRUE;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 699 "mon_parse.y"
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->value = (yyvsp[(1) - (1)].i); (yyval.cond_node)->is_reg = FALSE;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 711 "mon_parse.y"
    { mon_add_number_to_buffer((yyvsp[(1) - (1)].i)); }
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 712 "mon_parse.y"
    { mon_add_string_to_buffer((yyvsp[(1) - (1)].str)); }
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 719 "mon_parse.y"
    { mon_add_number_to_buffer((yyvsp[(1) - (1)].i)); }
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 720 "mon_parse.y"
    { mon_add_number_masked_to_buffer((yyvsp[(1) - (1)].i), 0x00); }
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 721 "mon_parse.y"
    { mon_add_string_to_buffer((yyvsp[(1) - (1)].str)); }
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 724 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 224:

/* Line 1464 of yacc.c  */
#line 725 "mon_parse.y"
    { (yyval.i) = (monitor_cpu_for_memspace[reg_memspace((yyvsp[(1) - (1)].i))]->mon_register_get_val)(reg_memspace((yyvsp[(1) - (1)].i)), reg_regid((yyvsp[(1) - (1)].i))); }
    break;

  case 225:

/* Line 1464 of yacc.c  */
#line 728 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 226:

/* Line 1464 of yacc.c  */
#line 729 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 227:

/* Line 1464 of yacc.c  */
#line 730 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 228:

/* Line 1464 of yacc.c  */
#line 731 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 229:

/* Line 1464 of yacc.c  */
#line 734 "mon_parse.y"
    { (yyval.i) = resolve_datatype(B_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 230:

/* Line 1464 of yacc.c  */
#line 735 "mon_parse.y"
    { (yyval.i) = resolve_datatype(O_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 231:

/* Line 1464 of yacc.c  */
#line 736 "mon_parse.y"
    { (yyval.i) = resolve_datatype(D_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 232:

/* Line 1464 of yacc.c  */
#line 739 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 233:

/* Line 1464 of yacc.c  */
#line 740 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 741 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 742 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 236:

/* Line 1464 of yacc.c  */
#line 743 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 751 "mon_parse.y"
    { (yyval.i) = 0;
                                                if ((yyvsp[(1) - (2)].str)) {
                                                    (monitor_cpu_for_memspace[default_memspace]->mon_assemble_instr)((yyvsp[(1) - (2)].str), (yyvsp[(2) - (2)].mode));
                                                } else {
                                                    new_cmd = 1;
                                                    asm_mode = 0;
                                                }
                                                opt_asm = 0;
                                              }
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 762 "mon_parse.y"
    { asm_mode = 0; }
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 765 "mon_parse.y"
    { if ((yyvsp[(2) - (2)].i) > 0xff) {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE_16;
                          (yyval.mode).param = (yyvsp[(2) - (2)].i);
                        } else {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE;
                          (yyval.mode).param = (yyvsp[(2) - (2)].i);
                        } }
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 772 "mon_parse.y"
    { if ((yyvsp[(1) - (1)].i) >= 0x10000) {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG;
               (yyval.mode).param = (yyvsp[(1) - (1)].i);
             } else if ((yyvsp[(1) - (1)].i) < 0x100) {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE;
               (yyval.mode).param = (yyvsp[(1) - (1)].i);
             } else {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE;
               (yyval.mode).param = (yyvsp[(1) - (1)].i);
             }
           }
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 783 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) >= 0x10000) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG_X;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          } else if ((yyvsp[(1) - (3)].i) < 0x100) { 
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE_X;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          } else {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_X;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          }
                        }
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 794 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE_Y;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          } else {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_Y;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          }
                        }
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 802 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_STACK_RELATIVE;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          } else { /* 6809 */
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
                            if ((yyvsp[(1) - (3)].i) >= -16 && (yyvsp[(1) - (3)].i) < 16) {
                                (yyval.mode).addr_submode = (yyvsp[(3) - (3)].i) | ((yyvsp[(1) - (3)].i) & 0x1F);
                            } else if ((yyvsp[(1) - (3)].i) >= -128 && (yyvsp[(1) - (3)].i) < 128) {
                                (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFF8;
                                (yyval.mode).param = (yyvsp[(1) - (3)].i);
                            } else if ((yyvsp[(1) - (3)].i) >= -32768 && (yyvsp[(1) - (3)].i) < 32768) {
                                (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFF16;
                                (yyval.mode).param = (yyvsp[(1) - (3)].i);
                            } else {
                                (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
                                mon_out("offset too large even for 16 bits (signed)\n");
                            }
                          }
                        }
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 821 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_DOUBLE;
                            (yyval.mode).param = (yyvsp[(3) - (3)].i);
                            (yyval.mode).addr_submode = (yyvsp[(1) - (3)].i);
                          }
                        }
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 827 "mon_parse.y"
    { if ((yyvsp[(2) - (3)].i) < 0x100) {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT;
                               (yyval.mode).param = (yyvsp[(2) - (3)].i);
                             } else {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT;
                               (yyval.mode).param = (yyvsp[(2) - (3)].i);
                             }
                           }
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 835 "mon_parse.y"
    { if ((yyvsp[(2) - (5)].i) < 0x100) {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[(2) - (5)].i);
                                         } else {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[(2) - (5)].i);
                                         }
                                       }
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 844 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_STACK_RELATIVE_Y; (yyval.mode).param = (yyvsp[(2) - (7)].i); }
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 846 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_Y; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 847 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_BC; }
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 848 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_DE; }
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 849 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_HL; }
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 850 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IX; }
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 851 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IY; }
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 852 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_SP; }
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 854 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_A; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 260:

/* Line 1464 of yacc.c  */
#line 856 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_HL; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 858 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IX; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 262:

/* Line 1464 of yacc.c  */
#line 860 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IY; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 861 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_IMPLIED; }
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 862 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ACCUMULATOR; }
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 863 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_B; }
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 864 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_C; }
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 865 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_D; }
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 866 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_E; }
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 867 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_H; }
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 868 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXH; }
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 869 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYH; }
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 870 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_L; }
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 871 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXL; }
    break;

  case 274:

/* Line 1464 of yacc.c  */
#line 872 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYL; }
    break;

  case 275:

/* Line 1464 of yacc.c  */
#line 873 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_AF; }
    break;

  case 276:

/* Line 1464 of yacc.c  */
#line 874 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_BC; }
    break;

  case 277:

/* Line 1464 of yacc.c  */
#line 875 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_DE; }
    break;

  case 278:

/* Line 1464 of yacc.c  */
#line 876 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_HL; }
    break;

  case 279:

/* Line 1464 of yacc.c  */
#line 877 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IX; }
    break;

  case 280:

/* Line 1464 of yacc.c  */
#line 878 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IY; }
    break;

  case 281:

/* Line 1464 of yacc.c  */
#line 879 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_SP; }
    break;

  case 282:

/* Line 1464 of yacc.c  */
#line 881 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_DIRECT; (yyval.mode).param = (yyvsp[(2) - (2)].i); }
    break;

  case 283:

/* Line 1464 of yacc.c  */
#line 882 "mon_parse.y"
    {    /* Clash with addr,x addr,y addr,s modes! */
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        if ((yyvsp[(1) - (3)].i) >= -16 && (yyvsp[(1) - (3)].i) < 16) {
            (yyval.mode).addr_submode = (3 << 5) | ((yyvsp[(1) - (3)].i) & 0x1F);
        } else if ((yyvsp[(1) - (3)].i) >= -128 && (yyvsp[(1) - (3)].i) < 128) {
            (yyval.mode).addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF8;
            (yyval.mode).param = (yyvsp[(1) - (3)].i);
        } else if ((yyvsp[(1) - (3)].i) >= -32768 && (yyvsp[(1) - (3)].i) < 32768) {
            (yyval.mode).addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF16;
            (yyval.mode).param = (yyvsp[(1) - (3)].i);
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
    break;

  case 284:

/* Line 1464 of yacc.c  */
#line 897 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
    break;

  case 285:

/* Line 1464 of yacc.c  */
#line 901 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (4)].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
    break;

  case 286:

/* Line 1464 of yacc.c  */
#line 905 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (3)].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
    break;

  case 287:

/* Line 1464 of yacc.c  */
#line 909 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(4) - (4)].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
    break;

  case 288:

/* Line 1464 of yacc.c  */
#line 913 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (2)].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
    break;

  case 289:

/* Line 1464 of yacc.c  */
#line 917 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
    break;

  case 290:

/* Line 1464 of yacc.c  */
#line 921 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
    break;

  case 291:

/* Line 1464 of yacc.c  */
#line 925 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
    break;

  case 292:

/* Line 1464 of yacc.c  */
#line 929 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).param = (yyvsp[(1) - (3)].i);
        if ((yyvsp[(1) - (3)].i) >= -128 && (yyvsp[(1) - (3)].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8;
        } else if ((yyvsp[(1) - (3)].i) >= -32768 && (yyvsp[(1) - (3)].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16;
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
    break;

  case 293:

/* Line 1464 of yacc.c  */
#line 941 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        if ((yyvsp[(2) - (5)].i) >= -16 && (yyvsp[(2) - (5)].i) < 16) {
            (yyval.mode).addr_submode = (yyvsp[(2) - (5)].i) & 0x1F;
        } else if ((yyvsp[(1) - (5)].i) >= -128 && (yyvsp[(1) - (5)].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFF8;
            (yyval.mode).param = (yyvsp[(2) - (5)].i);
        } else if ((yyvsp[(2) - (5)].i) >= -32768 && (yyvsp[(2) - (5)].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFF16;
            (yyval.mode).param = (yyvsp[(2) - (5)].i);
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
    break;

  case 294:

/* Line 1464 of yacc.c  */
#line 956 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
    break;

  case 295:

/* Line 1464 of yacc.c  */
#line 960 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (6)].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
    break;

  case 296:

/* Line 1464 of yacc.c  */
#line 964 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(4) - (5)].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
    break;

  case 297:

/* Line 1464 of yacc.c  */
#line 968 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(5) - (6)].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
    break;

  case 298:

/* Line 1464 of yacc.c  */
#line 972 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (4)].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
    break;

  case 299:

/* Line 1464 of yacc.c  */
#line 976 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
    break;

  case 300:

/* Line 1464 of yacc.c  */
#line 980 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
    break;

  case 301:

/* Line 1464 of yacc.c  */
#line 984 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
    break;

  case 302:

/* Line 1464 of yacc.c  */
#line 988 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).param = (yyvsp[(2) - (5)].i);
        if ((yyvsp[(2) - (5)].i) >= -128 && (yyvsp[(2) - (5)].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8_IND;
        } else if ((yyvsp[(2) - (5)].i) >= -32768 && (yyvsp[(2) - (5)].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16_IND;
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
    break;

  case 303:

/* Line 1464 of yacc.c  */
#line 1000 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | ASM_ADDR_MODE_EXTENDED_INDIRECT;
        (yyval.mode).param = (yyvsp[(2) - (3)].i);
        }
    break;

  case 304:

/* Line 1464 of yacc.c  */
#line 1005 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_LONG_Y;
        (yyval.mode).param = (yyvsp[(2) - (5)].i);
        }
    break;

  case 305:

/* Line 1464 of yacc.c  */
#line 1013 "mon_parse.y"
    { (yyval.i) = (0 << 5); printf("reg_x\n"); }
    break;

  case 306:

/* Line 1464 of yacc.c  */
#line 1014 "mon_parse.y"
    { (yyval.i) = (1 << 5); printf("reg_y\n"); }
    break;

  case 307:

/* Line 1464 of yacc.c  */
#line 1015 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 308:

/* Line 1464 of yacc.c  */
#line 1016 "mon_parse.y"
    { (yyval.i) = (3 << 5); printf("reg_s\n"); }
    break;

  case 309:

/* Line 1464 of yacc.c  */
#line 1020 "mon_parse.y"
    { (yyval.i) = (2 << 5); printf("reg_u\n"); }
    break;



/* Line 1464 of yacc.c  */
#line 4803 "mon_parse.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1684 of yacc.c  */
#line 1024 "mon_parse.y"


void parse_and_execute_line(char *input)
{
   char *temp_buf;
   int i, rc;

   temp_buf = lib_malloc(strlen(input) + 3);
   strcpy(temp_buf,input);
   i = (int)strlen(input);
   temp_buf[i++] = '\n';
   temp_buf[i++] = '\0';
   temp_buf[i++] = '\0';

   make_buffer(temp_buf);
   if ( (rc =yyparse()) != 0) {
       mon_out("ERROR -- ");
       switch(rc) {
         case ERR_BAD_CMD:
           mon_out("Bad command:\n");
           break;
         case ERR_RANGE_BAD_START:
           mon_out("Bad first address in range:\n");
           break;
         case ERR_RANGE_BAD_END:
           mon_out("Bad second address in range:\n");
           break;
         case ERR_EXPECT_CHECKNUM:
           mon_out("Checkpoint number expected:\n");
           break;
         case ERR_EXPECT_END_CMD:
           mon_out("Unexpected token:\n");
           break;
         case ERR_MISSING_CLOSE_PAREN:
           mon_out("')' expected:\n");
           break;
         case ERR_INCOMPLETE_COMPARE_OP:
           mon_out("Compare operation missing an operand:\n");
           break;
         case ERR_EXPECT_FILENAME:
           mon_out("Expecting a filename:\n");
           break;
         case ERR_ADDR_TOO_BIG:
           mon_out("Address too large:\n");
           break;
         case ERR_IMM_TOO_BIG:
           mon_out("Immediate argument too large:\n");
           break;
         case ERR_EXPECT_STRING:
           mon_out("Expecting a string.\n");
           break;
         case ERR_UNDEFINED_LABEL:
           mon_out("Found an undefined label.\n");
           break;
         case ERR_EXPECT_DEVICE_NUM:
           mon_out("Expecting a device number.\n");
           break;
         case ERR_EXPECT_ADDRESS:
           mon_out("Expecting an address.\n");
           break;
         case ERR_ILLEGAL_INPUT:
         default:
           mon_out("Wrong syntax:\n");
       }
       mon_out("  %s\n", input);
       for (i = 0; i < last_len; i++)
           mon_out(" ");
       mon_out("  ^\n");
       asm_mode = 0;
       new_cmd = 1;
   }
   lib_free(temp_buf);
   free_buffer();
}

static int yyerror(char *s)
{
   fprintf(stderr, "ERR:%s\n", s);
   return 0;
}

static int resolve_datatype(unsigned guess_type, const char *num)
{
   /* FIXME: Handle cases when default type is non-numerical */
   if (default_radix == e_hexadecimal) {
       return strtol(num, NULL, 16);
   }

   if ((guess_type == D_NUMBER) || (default_radix == e_decimal)) {
       return strtol(num, NULL, 10);
   }

   if ((guess_type == O_NUMBER) || (default_radix == e_octal)) {
       return strtol(num, NULL, 8);
   }

   return strtol(num, NULL, 2);
}

/*
 * Resolve a character sequence containing 8 hex digits like "08001000".
 * This could be a lazy version of "0800 1000". If the default radix is not
 * hexadecimal, we handle it like a ordinary number, in the latter case there
 * is only one number in the range.
 */
static int resolve_range(enum t_memspace memspace, MON_ADDR range[2],
                         const char *num)
{
    char start[5];
    char end[5];
    long sa;

    range[1] = BAD_ADDR;

    switch (default_radix)
    {
    case e_hexadecimal:
        /* checked twice, but as the code must have exactly 8 digits: */
        if (strlen(num) == 8) {
            memcpy(start, num, 4);
            start[4] = '\0';
            memcpy(end, num + 4, 4);
            end[4] = '\0';
            sa = strtol(start, NULL, 16);
            range[1] = new_addr(memspace, strtol(end, NULL, 16));
        }
        else
            sa = strtol(num, NULL, 16);
        break;

    case e_decimal:
       sa = strtol(num, NULL, 10);
       break;

    case e_octal:
       sa = strtol(num, NULL, 8);
       break;

    default:
       sa = strtol(num, NULL, 2);
    }

    if (!CHECK_ADDR(sa))
        return ERR_ADDR_TOO_BIG;

    range[0] = new_addr(memspace, sa);
    return 0;
}



