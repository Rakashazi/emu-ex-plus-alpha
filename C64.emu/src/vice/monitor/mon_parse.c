/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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
#define YYBISON_VERSION "2.5"

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

/* Line 268 of yacc.c  */
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

#if !defined(__minix_vmd) && !defined(MACOS_COMPILE)
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
#include "mon_register.h"
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
#define ERR_INVALID_REGISTER 16

#define BAD_ADDR (new_addr(e_invalid_space, 0))
#define CHECK_ADDR(x) ((x) == addr_mask(x))

#define YYDEBUG 1



/* Line 268 of yacc.c  */
#line 202 "mon_parse.c"

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
     CMD_QUIT = 319,
     CMD_CHDIR = 320,
     CMD_BANK = 321,
     CMD_LOAD_LABELS = 322,
     CMD_SAVE_LABELS = 323,
     CMD_ADD_LABEL = 324,
     CMD_DEL_LABEL = 325,
     CMD_SHOW_LABELS = 326,
     CMD_RECORD = 327,
     CMD_MON_STOP = 328,
     CMD_PLAYBACK = 329,
     CMD_CHAR_DISPLAY = 330,
     CMD_SPRITE_DISPLAY = 331,
     CMD_TEXT_DISPLAY = 332,
     CMD_SCREENCODE_DISPLAY = 333,
     CMD_ENTER_DATA = 334,
     CMD_ENTER_BIN_DATA = 335,
     CMD_KEYBUF = 336,
     CMD_BLOAD = 337,
     CMD_BSAVE = 338,
     CMD_SCREEN = 339,
     CMD_UNTIL = 340,
     CMD_CPU = 341,
     CMD_YYDEBUG = 342,
     CMD_BACKTRACE = 343,
     CMD_SCREENSHOT = 344,
     CMD_PWD = 345,
     CMD_DIR = 346,
     CMD_RESOURCE_GET = 347,
     CMD_RESOURCE_SET = 348,
     CMD_LOAD_RESOURCES = 349,
     CMD_SAVE_RESOURCES = 350,
     CMD_ATTACH = 351,
     CMD_DETACH = 352,
     CMD_MON_RESET = 353,
     CMD_TAPECTRL = 354,
     CMD_CARTFREEZE = 355,
     CMD_CPUHISTORY = 356,
     CMD_MEMMAPZAP = 357,
     CMD_MEMMAPSHOW = 358,
     CMD_MEMMAPSAVE = 359,
     CMD_COMMENT = 360,
     CMD_LIST = 361,
     CMD_STOPWATCH = 362,
     RESET = 363,
     CMD_EXPORT = 364,
     CMD_AUTOSTART = 365,
     CMD_AUTOLOAD = 366,
     CMD_LABEL_ASGN = 367,
     L_PAREN = 368,
     R_PAREN = 369,
     ARG_IMMEDIATE = 370,
     REG_A = 371,
     REG_X = 372,
     REG_Y = 373,
     COMMA = 374,
     INST_SEP = 375,
     L_BRACKET = 376,
     R_BRACKET = 377,
     LESS_THAN = 378,
     REG_U = 379,
     REG_S = 380,
     REG_PC = 381,
     REG_PCR = 382,
     REG_B = 383,
     REG_C = 384,
     REG_D = 385,
     REG_E = 386,
     REG_H = 387,
     REG_L = 388,
     REG_AF = 389,
     REG_BC = 390,
     REG_DE = 391,
     REG_HL = 392,
     REG_IX = 393,
     REG_IY = 394,
     REG_SP = 395,
     REG_IXH = 396,
     REG_IXL = 397,
     REG_IYH = 398,
     REG_IYL = 399,
     PLUS = 400,
     MINUS = 401,
     STRING = 402,
     FILENAME = 403,
     R_O_L = 404,
     OPCODE = 405,
     LABEL = 406,
     BANKNAME = 407,
     CPUTYPE = 408,
     MON_REGISTER = 409,
     COMPARE_OP = 410,
     RADIX_TYPE = 411,
     INPUT_SPEC = 412,
     CMD_CHECKPT_ON = 413,
     CMD_CHECKPT_OFF = 414,
     TOGGLE = 415,
     MASK = 416
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
#define CMD_QUIT 319
#define CMD_CHDIR 320
#define CMD_BANK 321
#define CMD_LOAD_LABELS 322
#define CMD_SAVE_LABELS 323
#define CMD_ADD_LABEL 324
#define CMD_DEL_LABEL 325
#define CMD_SHOW_LABELS 326
#define CMD_RECORD 327
#define CMD_MON_STOP 328
#define CMD_PLAYBACK 329
#define CMD_CHAR_DISPLAY 330
#define CMD_SPRITE_DISPLAY 331
#define CMD_TEXT_DISPLAY 332
#define CMD_SCREENCODE_DISPLAY 333
#define CMD_ENTER_DATA 334
#define CMD_ENTER_BIN_DATA 335
#define CMD_KEYBUF 336
#define CMD_BLOAD 337
#define CMD_BSAVE 338
#define CMD_SCREEN 339
#define CMD_UNTIL 340
#define CMD_CPU 341
#define CMD_YYDEBUG 342
#define CMD_BACKTRACE 343
#define CMD_SCREENSHOT 344
#define CMD_PWD 345
#define CMD_DIR 346
#define CMD_RESOURCE_GET 347
#define CMD_RESOURCE_SET 348
#define CMD_LOAD_RESOURCES 349
#define CMD_SAVE_RESOURCES 350
#define CMD_ATTACH 351
#define CMD_DETACH 352
#define CMD_MON_RESET 353
#define CMD_TAPECTRL 354
#define CMD_CARTFREEZE 355
#define CMD_CPUHISTORY 356
#define CMD_MEMMAPZAP 357
#define CMD_MEMMAPSHOW 358
#define CMD_MEMMAPSAVE 359
#define CMD_COMMENT 360
#define CMD_LIST 361
#define CMD_STOPWATCH 362
#define RESET 363
#define CMD_EXPORT 364
#define CMD_AUTOSTART 365
#define CMD_AUTOLOAD 366
#define CMD_LABEL_ASGN 367
#define L_PAREN 368
#define R_PAREN 369
#define ARG_IMMEDIATE 370
#define REG_A 371
#define REG_X 372
#define REG_Y 373
#define COMMA 374
#define INST_SEP 375
#define L_BRACKET 376
#define R_BRACKET 377
#define LESS_THAN 378
#define REG_U 379
#define REG_S 380
#define REG_PC 381
#define REG_PCR 382
#define REG_B 383
#define REG_C 384
#define REG_D 385
#define REG_E 386
#define REG_H 387
#define REG_L 388
#define REG_AF 389
#define REG_BC 390
#define REG_DE 391
#define REG_HL 392
#define REG_IX 393
#define REG_IY 394
#define REG_SP 395
#define REG_IXH 396
#define REG_IXL 397
#define REG_IYH 398
#define REG_IYL 399
#define PLUS 400
#define MINUS 401
#define STRING 402
#define FILENAME 403
#define R_O_L 404
#define OPCODE 405
#define LABEL 406
#define BANKNAME 407
#define CPUTYPE 408
#define MON_REGISTER 409
#define COMPARE_OP 410
#define RADIX_TYPE 411
#define INPUT_SPEC 412
#define CMD_CHECKPT_ON 413
#define CMD_CHECKPT_OFF 414
#define TOGGLE 415
#define MASK 416




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 131 "mon_parse.y"

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



/* Line 293 of yacc.c  */
#line 575 "mon_parse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 587 "mon_parse.c"

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
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

# define YYCOPY_NEEDED 1

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

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
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
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  307
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1707

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  168
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  55
/* YYNRULES -- Number of rules.  */
#define YYNRULES  310
/* YYNRULES -- Number of states.  */
#define YYNSTATES  617

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   416

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     166,   167,   164,   162,     2,   163,     2,   165,     2,     2,
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
     155,   156,   157,   158,   159,   160,   161
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
     325,   328,   334,   337,   343,   346,   350,   353,   357,   360,
     364,   370,   374,   377,   383,   389,   394,   398,   401,   405,
     408,   412,   415,   418,   421,   425,   429,   432,   436,   440,
     444,   448,   451,   455,   458,   462,   468,   472,   477,   481,
     485,   488,   493,   498,   501,   505,   509,   512,   518,   524,
     530,   534,   539,   545,   550,   556,   561,   567,   573,   576,
     580,   585,   589,   593,   599,   603,   609,   613,   616,   620,
     625,   628,   631,   633,   635,   636,   638,   640,   642,   644,
     647,   649,   651,   652,   654,   657,   661,   663,   667,   669,
     671,   673,   675,   679,   681,   685,   688,   689,   691,   695,
     697,   699,   700,   702,   704,   706,   708,   710,   712,   714,
     718,   722,   726,   730,   734,   738,   740,   743,   744,   748,
     752,   756,   760,   762,   764,   766,   770,   772,   774,   776,
     779,   781,   783,   785,   787,   789,   791,   793,   795,   797,
     799,   801,   803,   805,   807,   809,   811,   813,   815,   819,
     823,   826,   829,   831,   833,   836,   838,   842,   846,   850,
     854,   858,   864,   872,   878,   882,   886,   890,   894,   898,
     902,   908,   914,   920,   926,   927,   929,   931,   933,   935,
     937,   939,   941,   943,   945,   947,   949,   951,   953,   955,
     957,   959,   961,   963,   966,   970,   974,   979,   983,   988,
     991,   995,   999,  1003,  1007,  1013,  1019,  1026,  1032,  1039,
    1044,  1050,  1056,  1062,  1068,  1072,  1078,  1080,  1082,  1084,
    1086
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     169,     0,    -1,   170,    -1,   218,    22,    -1,    22,    -1,
     172,    -1,   170,   172,    -1,    23,    -1,    22,    -1,     1,
      -1,   173,    -1,   175,    -1,   178,    -1,   176,    -1,   179,
      -1,   180,    -1,   181,    -1,   182,    -1,   183,    -1,   184,
      -1,   185,    -1,   186,    -1,    13,    -1,    66,   171,    -1,
      66,   202,   171,    -1,    66,   152,   171,    -1,    66,   202,
     201,   152,   171,    -1,    38,   200,   171,    -1,    38,   171,
      -1,    46,   171,    -1,    46,   200,   171,    -1,    86,   171,
      -1,    86,   153,   171,    -1,   101,   171,    -1,   101,   201,
     205,   171,    -1,    26,   171,    -1,    49,   189,   171,    -1,
      50,   189,   171,    -1,    58,   171,    -1,    58,   201,   205,
     171,    -1,    57,   171,    -1,    57,   201,   205,   171,    -1,
      29,   171,    -1,    29,   201,   205,   171,    -1,    30,   171,
      -1,    30,   201,   205,   171,    -1,    84,   171,    -1,   174,
      -1,    39,   171,    -1,    39,   202,   171,    -1,    39,   194,
     171,    -1,    67,   202,   201,   189,   171,    -1,    67,   189,
     171,    -1,    68,   202,   201,   189,   171,    -1,    68,   189,
     171,    -1,    69,   200,   201,   151,   171,    -1,    70,   151,
     171,    -1,    70,   202,   201,   151,   171,    -1,    71,   202,
     171,    -1,    71,   171,    -1,   112,    21,   200,   171,    -1,
     112,    21,   200,    24,   171,    -1,    -1,    55,   200,   177,
     219,   171,    -1,    55,   200,   171,    -1,    56,   197,   171,
      -1,    56,   171,    -1,    37,   198,   201,   200,   171,    -1,
      48,   198,   201,   200,   171,    -1,    36,   198,   201,   209,
     171,    -1,    35,   198,   201,   211,   171,    -1,    43,   156,
     201,   197,   171,    -1,    43,   197,   171,    -1,    43,   171,
      -1,    75,   197,   171,    -1,    75,   171,    -1,    76,   197,
     171,    -1,    76,   171,    -1,    77,   197,   171,    -1,    77,
     171,    -1,    78,   197,   171,    -1,    78,   171,    -1,   102,
     171,    -1,   103,   171,    -1,   103,   201,   205,   171,    -1,
     103,   201,   205,   197,   171,    -1,   104,   189,   201,   205,
     171,    -1,    44,   192,   197,   206,   171,    -1,    44,   171,
      -1,    85,   197,   171,    -1,    85,   171,    -1,    62,   192,
     197,   206,   171,    -1,    62,   171,    -1,    45,   192,   197,
     206,   171,    -1,    45,   171,    -1,   158,   196,   171,    -1,
     158,   171,    -1,   159,   196,   171,    -1,   159,   171,    -1,
      34,   196,   171,    -1,    34,   196,   201,   205,   171,    -1,
      52,   196,   171,    -1,    52,   171,    -1,    53,   196,    15,
     207,   171,    -1,    54,   196,   201,   147,   171,    -1,    54,
     196,     1,   171,    -1,    25,   160,   171,    -1,    25,   171,
      -1,    42,   156,   171,    -1,    42,   171,    -1,    60,   202,
     171,    -1,   109,   171,    -1,    64,   171,    -1,    51,   171,
      -1,    63,   187,   171,    -1,    59,   205,   171,    -1,    61,
     171,    -1,    61,   187,   171,    -1,     7,   205,   171,    -1,
      65,   187,   171,    -1,    81,   187,   171,    -1,    88,   171,
      -1,    91,   188,   171,    -1,    90,   171,    -1,    89,   189,
     171,    -1,    89,   189,   201,   205,   171,    -1,    92,   147,
     171,    -1,    93,   147,   147,   171,    -1,    94,   189,   171,
      -1,    95,   189,   171,    -1,    98,   171,    -1,    98,   201,
     205,   171,    -1,    99,   201,   205,   171,    -1,   100,   171,
      -1,   105,   188,   171,    -1,   107,   108,   171,    -1,   107,
     171,    -1,    31,   189,   190,   199,   171,    -1,    82,   189,
     190,   199,   171,    -1,    32,   189,   190,   198,   171,    -1,
      32,   189,     1,    -1,    32,   189,   190,     1,    -1,    83,
     189,   190,   198,   171,    -1,    83,   189,   190,     1,    -1,
      33,   189,   190,   200,   171,    -1,    33,   189,   190,     1,
      -1,    27,   205,   205,   199,   171,    -1,    28,   205,   205,
     200,   171,    -1,   106,   171,    -1,   106,   190,   171,    -1,
      96,   189,   205,   171,    -1,    97,   205,   171,    -1,   110,
     189,   171,    -1,   110,   189,   201,   216,   171,    -1,   111,
     189,   171,    -1,   111,   189,   201,   216,   171,    -1,    72,
     189,   171,    -1,    73,   171,    -1,    74,   189,   171,    -1,
      79,   200,   209,   171,    -1,    80,   171,    -1,    87,   171,
      -1,   149,    -1,   149,    -1,    -1,   148,    -1,     1,    -1,
     205,    -1,     1,    -1,   191,    14,    -1,    14,    -1,   191,
      -1,    -1,   154,    -1,   202,   154,    -1,   194,   119,   195,
      -1,   195,    -1,   193,    21,   216,    -1,   214,    -1,     1,
      -1,   198,    -1,   200,    -1,   200,   201,   200,    -1,     9,
      -1,   202,   201,     9,    -1,   201,   200,    -1,    -1,   203,
      -1,   202,   201,   203,    -1,   151,    -1,   119,    -1,    -1,
      16,    -1,    17,    -1,    18,    -1,    19,    -1,    20,    -1,
     204,    -1,   216,    -1,   205,   162,   205,    -1,   205,   163,
     205,    -1,   205,   164,   205,    -1,   205,   165,   205,    -1,
     166,   205,   167,    -1,   166,   205,     1,    -1,   213,    -1,
      15,   207,    -1,    -1,   207,   155,   207,    -1,   207,   155,
       1,    -1,   113,   207,   114,    -1,   113,   207,     1,    -1,
     208,    -1,   193,    -1,   216,    -1,   209,   201,   210,    -1,
     210,    -1,   216,    -1,   147,    -1,   211,   212,    -1,   212,
      -1,   216,    -1,   161,    -1,   147,    -1,   216,    -1,   193,
      -1,     4,    -1,    12,    -1,    11,    -1,    10,    -1,    12,
      -1,    11,    -1,    10,    -1,     3,    -1,     4,    -1,     5,
      -1,     6,    -1,   215,    -1,   217,   120,   218,    -1,   218,
     120,   218,    -1,   218,   120,    -1,   150,   220,    -1,   218,
      -1,   217,    -1,   115,   216,    -1,   216,    -1,   216,   119,
     117,    -1,   216,   119,   118,    -1,   216,   119,   125,    -1,
     216,   119,   216,    -1,   113,   216,   114,    -1,   113,   216,
     119,   117,   114,    -1,   113,   216,   119,   125,   114,   119,
     118,    -1,   113,   216,   114,   119,   118,    -1,   113,   135,
     114,    -1,   113,   136,   114,    -1,   113,   137,   114,    -1,
     113,   138,   114,    -1,   113,   139,   114,    -1,   113,   140,
     114,    -1,   113,   216,   114,   119,   116,    -1,   113,   216,
     114,   119,   137,    -1,   113,   216,   114,   119,   138,    -1,
     113,   216,   114,   119,   139,    -1,    -1,   116,    -1,   128,
      -1,   129,    -1,   130,    -1,   131,    -1,   132,    -1,   141,
      -1,   143,    -1,   133,    -1,   142,    -1,   144,    -1,   134,
      -1,   135,    -1,   136,    -1,   137,    -1,   138,    -1,   139,
      -1,   140,    -1,   123,   216,    -1,   216,   119,   222,    -1,
     119,   221,   145,    -1,   119,   221,   145,   145,    -1,   119,
     146,   221,    -1,   119,   146,   146,   221,    -1,   119,   221,
      -1,   128,   119,   221,    -1,   116,   119,   221,    -1,   130,
     119,   221,    -1,   216,   119,   126,    -1,   121,   216,   119,
     221,   122,    -1,   121,   119,   221,   145,   122,    -1,   121,
     119,   221,   145,   145,   122,    -1,   121,   119,   146,   221,
     122,    -1,   121,   119,   146,   146,   221,   122,    -1,   121,
     119,   221,   122,    -1,   121,   128,   119,   221,   122,    -1,
     121,   116,   119,   221,   122,    -1,   121,   130,   119,   221,
     122,    -1,   121,   216,   119,   126,   122,    -1,   121,   216,
     122,    -1,   121,   216,   122,   119,   118,    -1,   117,    -1,
     118,    -1,   222,    -1,   125,    -1,   124,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   195,   195,   196,   197,   200,   201,   204,   205,   206,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   224,   226,   228,   230,   232,   234,   236,
     238,   240,   242,   244,   246,   248,   250,   252,   254,   256,
     258,   260,   262,   264,   266,   268,   270,   272,   275,   277,
     279,   282,   287,   292,   294,   296,   298,   300,   302,   304,
     306,   310,   317,   316,   319,   321,   323,   327,   329,   331,
     333,   335,   337,   339,   341,   343,   345,   347,   349,   351,
     353,   355,   357,   359,   361,   363,   365,   369,   378,   381,
     385,   388,   397,   400,   409,   414,   416,   418,   420,   422,
     424,   426,   428,   430,   432,   434,   438,   440,   445,   447,
     465,   467,   469,   471,   475,   477,   479,   481,   483,   485,
     487,   489,   491,   493,   495,   497,   499,   501,   503,   505,
     507,   509,   511,   513,   515,   517,   519,   523,   525,   527,
     529,   531,   533,   535,   537,   539,   541,   543,   545,   547,
     549,   551,   553,   555,   557,   559,   563,   565,   567,   571,
     573,   577,   581,   584,   585,   588,   589,   592,   593,   596,
     597,   600,   601,   604,   610,   618,   619,   622,   626,   627,
     630,   631,   634,   635,   637,   641,   642,   645,   650,   655,
     665,   666,   669,   670,   671,   672,   673,   676,   678,   680,
     681,   682,   683,   684,   685,   686,   689,   690,   692,   697,
     699,   701,   703,   707,   713,   721,   722,   725,   726,   729,
     730,   733,   734,   735,   738,   739,   742,   743,   744,   745,
     748,   749,   750,   753,   754,   755,   756,   757,   760,   761,
     762,   765,   775,   776,   779,   786,   797,   808,   816,   835,
     841,   849,   857,   859,   861,   862,   863,   864,   865,   866,
     867,   869,   871,   873,   875,   876,   877,   878,   879,   880,
     881,   882,   883,   884,   885,   886,   887,   888,   889,   890,
     891,   892,   893,   895,   896,   911,   915,   919,   923,   927,
     931,   935,   939,   943,   955,   970,   974,   978,   982,   986,
     990,   994,   998,  1002,  1014,  1019,  1027,  1028,  1029,  1030,
    1034
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
  "CMD_DEVICE", "CMD_HELP", "CMD_WATCH", "CMD_DISK", "CMD_QUIT",
  "CMD_CHDIR", "CMD_BANK", "CMD_LOAD_LABELS", "CMD_SAVE_LABELS",
  "CMD_ADD_LABEL", "CMD_DEL_LABEL", "CMD_SHOW_LABELS", "CMD_RECORD",
  "CMD_MON_STOP", "CMD_PLAYBACK", "CMD_CHAR_DISPLAY", "CMD_SPRITE_DISPLAY",
  "CMD_TEXT_DISPLAY", "CMD_SCREENCODE_DISPLAY", "CMD_ENTER_DATA",
  "CMD_ENTER_BIN_DATA", "CMD_KEYBUF", "CMD_BLOAD", "CMD_BSAVE",
  "CMD_SCREEN", "CMD_UNTIL", "CMD_CPU", "CMD_YYDEBUG", "CMD_BACKTRACE",
  "CMD_SCREENSHOT", "CMD_PWD", "CMD_DIR", "CMD_RESOURCE_GET",
  "CMD_RESOURCE_SET", "CMD_LOAD_RESOURCES", "CMD_SAVE_RESOURCES",
  "CMD_ATTACH", "CMD_DETACH", "CMD_MON_RESET", "CMD_TAPECTRL",
  "CMD_CARTFREEZE", "CMD_CPUHISTORY", "CMD_MEMMAPZAP", "CMD_MEMMAPSHOW",
  "CMD_MEMMAPSAVE", "CMD_COMMENT", "CMD_LIST", "CMD_STOPWATCH", "RESET",
  "CMD_EXPORT", "CMD_AUTOSTART", "CMD_AUTOLOAD", "CMD_LABEL_ASGN",
  "L_PAREN", "R_PAREN", "ARG_IMMEDIATE", "REG_A", "REG_X", "REG_Y",
  "COMMA", "INST_SEP", "L_BRACKET", "R_BRACKET", "LESS_THAN", "REG_U",
  "REG_S", "REG_PC", "REG_PCR", "REG_B", "REG_C", "REG_D", "REG_E",
  "REG_H", "REG_L", "REG_AF", "REG_BC", "REG_DE", "REG_HL", "REG_IX",
  "REG_IY", "REG_SP", "REG_IXH", "REG_IXL", "REG_IYH", "REG_IYL", "PLUS",
  "MINUS", "STRING", "FILENAME", "R_O_L", "OPCODE", "LABEL", "BANKNAME",
  "CPUTYPE", "MON_REGISTER", "COMPARE_OP", "RADIX_TYPE", "INPUT_SPEC",
  "CMD_CHECKPT_ON", "CMD_CHECKPT_OFF", "TOGGLE", "MASK", "'+'", "'-'",
  "'*'", "'/'", "'('", "')'", "$accept", "top_level", "command_list",
  "end_cmd", "command", "machine_state_rules", "register_mod",
  "symbol_table_rules", "asm_rules", "$@1", "memory_rules",
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
     415,   416,    43,    45,    42,    47,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   168,   169,   169,   169,   170,   170,   171,   171,   171,
     172,   172,   172,   172,   172,   172,   172,   172,   172,   172,
     172,   172,   172,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   174,   174,
     174,   175,   175,   175,   175,   175,   175,   175,   175,   175,
     175,   175,   177,   176,   176,   176,   176,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   178,   178,   179,   179,   179,
     179,   179,   179,   179,   179,   180,   180,   180,   180,   180,
     180,   180,   180,   180,   180,   180,   181,   181,   181,   181,
     181,   181,   181,   181,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   182,   182,   182,
     182,   182,   182,   182,   182,   182,   182,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   184,   184,   184,   185,
     185,   186,   187,   188,   188,   189,   189,   190,   190,   191,
     191,   192,   192,   193,   193,   194,   194,   195,   196,   196,
     197,   197,   198,   198,   198,   199,   199,   200,   200,   200,
     201,   201,   202,   202,   202,   202,   202,   203,   204,   205,
     205,   205,   205,   205,   205,   205,   206,   206,   207,   207,
     207,   207,   207,   208,   208,   209,   209,   210,   210,   211,
     211,   212,   212,   212,   213,   213,   214,   214,   214,   214,
     215,   215,   215,   216,   216,   216,   216,   216,   217,   217,
     217,   218,   219,   219,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   220,   220,   220,   220,
     220,   220,   220,   220,   220,   220,   221,   221,   221,   221,
     222
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
       2,     5,     2,     5,     2,     3,     2,     3,     2,     3,
       5,     3,     2,     5,     5,     4,     3,     2,     3,     2,
       3,     2,     2,     2,     3,     3,     2,     3,     3,     3,
       3,     2,     3,     2,     3,     5,     3,     4,     3,     3,
       2,     4,     4,     2,     3,     3,     2,     5,     5,     5,
       3,     4,     5,     4,     5,     4,     5,     5,     2,     3,
       4,     3,     3,     5,     3,     5,     3,     2,     3,     4,
       2,     2,     1,     1,     0,     1,     1,     1,     1,     2,
       1,     1,     0,     1,     2,     3,     1,     3,     1,     1,
       1,     1,     3,     1,     3,     2,     0,     1,     3,     1,
       1,     0,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     1,     2,     0,     3,     3,
       3,     3,     1,     1,     1,     3,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       2,     2,     1,     1,     2,     1,     3,     3,     3,     3,
       3,     5,     7,     5,     3,     3,     3,     3,     3,     3,
       5,     5,     5,     5,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     4,     3,     4,     2,
       3,     3,     3,     3,     5,     5,     6,     5,     6,     4,
       5,     5,     5,     5,     3,     5,     1,     1,     1,     1,
       1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,    22,     4,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   164,     0,     0,
       0,     0,     0,     0,     0,   191,     0,     0,     0,     0,
       0,   164,     0,     0,     0,     0,     0,     0,   264,     0,
       0,     0,     2,     5,    10,    47,    11,    13,    12,    14,
      15,    16,    17,    18,    19,    20,    21,     0,   233,   234,
     235,   236,   232,   231,   230,   192,   193,   194,   195,   196,
     173,     0,   225,     0,     0,   205,   237,   224,     9,     8,
       7,     0,   107,    35,     0,     0,   190,    42,     0,    44,
       0,   166,   165,     0,     0,     0,   179,   226,   229,   228,
     227,     0,   178,   183,   189,   191,   191,   191,   187,   197,
     198,   191,   191,    28,     0,   191,    48,     0,     0,   176,
       0,     0,   109,   191,    73,     0,   180,   191,   170,    88,
     171,     0,    94,     0,    29,     0,   191,     0,     0,   113,
       9,   102,     0,     0,     0,     0,    66,     0,    40,     0,
      38,     0,     0,     0,   162,   116,     0,    92,     0,     0,
     112,     0,     0,    23,     0,     0,   191,     0,   191,   191,
       0,   191,    59,     0,     0,   157,     0,    75,     0,    77,
       0,    79,     0,    81,     0,     0,   160,     0,     0,     0,
      46,    90,     0,     0,    31,   161,   121,     0,   123,   163,
       0,     0,     0,     0,     0,     0,     0,   130,     0,     0,
     133,    33,     0,    82,    83,     0,   191,     0,     9,   148,
       0,   167,     0,   136,   111,     0,     0,     0,     0,     0,
     265,     0,     0,     0,   266,   267,   268,   269,   270,   273,
     276,   277,   278,   279,   280,   281,   282,   271,   274,   272,
     275,   245,   241,    96,     0,    98,     0,     1,     6,     3,
       0,   174,     0,     0,     0,     0,   118,   106,   191,     0,
       0,     0,   168,   191,   140,     0,     0,    99,     0,     0,
       0,     0,     0,     0,    27,     0,     0,     0,    50,    49,
     108,     0,    72,   169,   207,   207,    30,     0,    36,    37,
     101,     0,     0,     0,    64,     0,    65,     0,     0,   115,
     110,   117,   207,   114,   119,    25,    24,     0,    52,     0,
      54,     0,     0,    56,     0,    58,   156,   158,    74,    76,
      78,    80,   218,     0,   216,   217,   120,   191,     0,    89,
      32,   124,     0,   122,   126,     0,   128,   129,     0,   151,
       0,     0,     0,     0,     0,   134,   149,   135,   152,     0,
     154,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     244,     0,   306,   307,   310,   309,     0,   289,   308,     0,
       0,     0,     0,     0,   283,     0,     0,     0,    95,    97,
     204,   203,   199,   200,   201,   202,     0,     0,     0,    43,
      45,     0,   141,     0,   145,     0,     0,   223,   222,     0,
     220,   221,   182,   184,   188,     0,     0,   177,   175,     0,
       0,     0,     0,     0,     0,   213,     0,   212,   214,   105,
       0,   243,   242,     0,    41,    39,     0,     0,     0,     0,
       0,     0,   159,     0,     0,   143,     0,     0,   127,   150,
     131,   132,    34,    84,     0,     0,     0,     0,     0,    60,
     254,   255,   256,   257,   258,   259,   250,     0,   291,     0,
     287,   285,     0,     0,     0,     0,     0,     0,   304,   290,
     292,   246,   247,   248,   293,   249,   284,   146,   185,   147,
     137,   139,   144,   100,    70,   219,    69,    67,    71,   206,
      87,    93,    68,     0,     0,   103,   104,     0,   240,    63,
      91,    26,    51,    53,    55,    57,   215,   138,   142,   125,
      85,    86,   153,   155,    61,     0,     0,     0,   288,   286,
       0,     0,     0,   299,     0,     0,     0,     0,     0,     0,
     211,   210,   209,   208,   238,   239,   260,   253,   261,   262,
     263,   251,     0,   301,     0,   297,   295,     0,   300,   302,
     303,   294,   305,     0,   298,   296,   252
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    91,    92,   132,    93,    94,    95,    96,    97,   355,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   206,
     250,   143,   270,   180,   181,   122,   168,   169,   151,   175,
     176,   446,   177,   447,   123,   158,   159,   271,   471,   476,
     477,   383,   384,   459,   460,   125,   152,   126,   160,   481,
     107,   483,   302,   427,   428
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -428
static const yytype_int16 yypact[] =
{
    1193,   860,  -428,  -428,    19,    62,   860,   860,    85,    85,
       9,     9,     9,   619,  1505,  1505,  1505,  1173,   329,    14,
    1061,  1089,  1089,  1173,  1505,     9,     9,    62,   708,   619,
     619,  1523,  1150,    85,    85,   860,   579,    91,  1089,  -130,
      62,  -130,   398,   366,   366,  1523,   305,  1117,     9,    62,
       9,  1150,  1150,  1150,  1150,  1523,    62,  -130,     9,     9,
      62,  1150,    52,    62,    62,     9,    62,  -123,   -96,   -53,
       9,     9,     9,   860,    85,   -13,    62,    85,    62,    85,
       9,  -123,   551,   270,    62,     9,     9,   102,  1547,   708,
     708,   112,  1328,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,   125,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,   860,  -428,   -30,    57,  -428,  -428,  -428,  -428,  -428,
    -428,    62,  -428,  -428,   831,   831,  -428,  -428,   860,  -428,
     860,  -428,  -428,   574,   599,   574,  -428,  -428,  -428,  -428,
    -428,    85,  -428,  -428,  -428,   -13,   -13,   -13,  -428,  -428,
    -428,   -13,   -13,  -428,    62,   -13,  -428,   127,    99,  -428,
      60,    62,  -428,   -13,  -428,    62,  -428,   426,  -428,  -428,
     138,  1505,  -428,  1505,  -428,    62,   -13,    62,    62,  -428,
     410,  -428,    62,   143,    13,    76,  -428,    62,  -428,   860,
    -428,   860,    57,    62,  -428,  -428,    62,  -428,  1505,    62,
    -428,    62,    62,  -428,    17,    62,   -13,    62,   -13,   -13,
      62,   -13,  -428,    62,    62,  -428,    62,  -428,    62,  -428,
      62,  -428,    62,  -428,    62,   358,  -428,    62,   574,   574,
    -428,  -428,    62,    62,  -428,  -428,  -428,    85,  -428,  -428,
      62,    62,     6,    62,    62,   860,    57,  -428,   860,   860,
    -428,  -428,   860,  -428,  -428,   860,   -13,    62,   459,  -428,
      62,   225,    62,  -428,  -428,  1027,  1027,  1523,  1557,  1327,
      42,   192,  1013,  1327,    46,  -428,    49,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,    63,  -428,  -428,    62,  -428,    62,  -428,  -428,  -428,
      28,  -428,   860,   860,   860,   860,  -428,  -428,    10,   878,
      57,    57,  -428,   224,  1309,  1445,  1487,  -428,   860,   262,
    1523,  1198,   358,  1523,  -428,  1327,  1327,   508,  -428,  -428,
    -428,  1505,  -428,  -428,   147,   147,  -428,  1523,  -428,  -428,
    -428,   972,    62,    16,  -428,    26,  -428,    57,    57,  -428,
    -428,  -428,   147,  -428,  -428,  -428,  -428,    33,  -428,     9,
    -428,     9,    36,  -428,    38,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,   998,  -428,  -428,  -428,   224,  1465,  -428,
    -428,  -428,   860,  -428,  -428,    62,  -428,  -428,    57,  -428,
      57,    57,    57,   807,   860,  -428,  -428,  -428,  -428,  1327,
    -428,  1327,   406,    80,    82,    83,    95,   103,   109,   -54,
    -428,   131,  -428,  -428,  -428,  -428,   276,    71,  -428,   110,
     326,   114,   117,  -106,  -428,   131,   131,  1581,  -428,  -428,
    -428,  -428,   -46,   -46,  -428,  -428,    62,  1523,    62,  -428,
    -428,    62,  -428,    62,  -428,    62,    57,  -428,  -428,   284,
    -428,  -428,  -428,  -428,  -428,   998,    62,  -428,  -428,    62,
     972,    62,    62,    62,   972,  -428,    44,  -428,  -428,  -428,
      62,   115,   123,    62,  -428,  -428,    62,    62,    62,    62,
      62,    62,  -428,   358,    62,  -428,    62,    57,  -428,  -428,
    -428,  -428,  -428,  -428,    62,    57,    62,    62,    62,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,   126,    -8,  -428,   131,
    -428,   105,   131,   441,   -97,   131,   131,   522,   133,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,    98,
    -428,  -428,  -428,    11,   787,  -428,  -428,    26,    26,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,   496,   145,   161,  -428,  -428,
     154,   131,   157,  -428,   -52,   158,   169,   175,   177,   165,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,   181,  -428,   182,  -428,  -428,   193,  -428,  -428,
    -428,  -428,  -428,   185,  -428,  -428,  -428
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -428,  -428,  -428,   457,   227,  -428,  -428,  -428,  -428,  -428,
    -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,  -428,    89,
     248,   354,   229,  -428,   116,   -17,  -428,     2,   173,     3,
      -7,  -277,     7,    -6,   728,  -220,  -428,    43,  -286,  -427,
    -428,     4,  -152,  -428,  -117,  -428,  -428,  -428,    -1,  -428,
    -351,  -428,  -428,  -295,   -84
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -192
static const yytype_int16 yytable[] =
{
     127,   167,   138,   140,   482,   127,   127,   155,   161,   162,
     141,  -186,   590,   527,   352,   128,   528,   186,   128,   204,
     128,   156,   156,   156,   164,   583,   249,   199,   201,   440,
     185,   156,  -186,  -186,   127,   197,   129,   130,   195,   129,
     130,   129,   130,   549,   124,   128,   451,   553,   584,   134,
     135,   251,   219,   128,   228,   230,   232,   234,   128,   472,
     516,   128,   235,   128,   242,   517,   129,   130,   258,   259,
     606,   262,   127,   265,   129,   130,   486,   128,   202,   129,
     130,   127,   129,   130,   129,   130,   128,   301,  -191,  -191,
    -191,  -191,   128,   607,   252,  -191,  -191,  -191,   129,   130,
     128,  -191,  -191,  -191,  -191,  -191,   136,   129,   130,   576,
     494,   464,   307,   129,   130,   464,   256,   577,   314,   315,
     127,   129,   130,   277,   311,   591,   518,   593,   209,   136,
     211,   520,   136,   127,   127,   524,   136,   127,   183,   127,
     529,   530,   127,   127,   127,   328,   237,   309,   336,   329,
     330,   331,   343,   395,   208,   332,   333,   142,   351,   335,
    -191,   421,   470,   480,   310,   435,   554,   341,   436,  -191,
     171,   330,   312,   313,   314,   315,    88,   318,   319,   131,
     347,   320,   437,   321,   344,   487,   345,   490,   353,   491,
     312,   313,   314,   315,   510,   441,   511,   512,   127,   554,
     127,   192,   193,   194,   136,   243,   594,   595,   367,   513,
     369,   362,   371,   372,   311,   374,   521,   514,   337,   312,
     313,   314,   315,   515,   578,  -186,   -62,   580,   582,   522,
     585,   586,   588,   525,   385,   557,   526,   127,   127,  -191,
     204,   392,   357,   558,   358,   575,  -186,  -186,   422,   423,
     579,  -191,   589,   554,   127,   424,   425,   127,   127,   601,
     404,   127,   304,   306,   127,   108,   109,   110,   111,   409,
     411,   128,   112,   113,   114,   602,   603,   419,   420,   605,
     608,   433,   434,   612,   412,   128,   604,   108,   109,   110,
     111,   609,   129,   130,   112,   113,   114,   610,   398,   611,
     613,   400,   401,   616,   614,   402,   129,   130,   403,   422,
     423,   127,   127,   127,   127,   615,   424,   425,   453,   308,
     167,   115,   116,   117,   118,   119,   448,   127,   461,   267,
     128,   385,   156,   455,   475,   467,   465,   462,   426,   468,
     466,   566,   545,   136,   469,   115,   116,   117,   118,   119,
     478,   129,   130,   536,   473,   442,   443,   444,   445,     0,
       0,   108,   109,   110,   111,   144,   145,   141,   112,   113,
     114,   456,   323,   325,   326,     0,     0,   493,   272,   187,
     188,   496,   115,   116,   117,   118,   119,   312,   313,   314,
     315,   127,     0,   422,   423,   156,     0,   215,   217,   128,
     424,   425,   224,   127,   226,     0,   504,   128,   506,   457,
     507,  -179,   238,   239,   115,   116,   117,   118,   119,   247,
     129,   130,   519,   458,   253,   254,   255,  -181,   129,   130,
     508,   457,  -179,  -179,   266,   497,   535,     0,     0,   275,
     276,  -181,     0,   422,   423,   458,     0,   505,  -181,  -181,
     424,   425,     0,   475,   538,     0,   220,   475,   461,   493,
    -168,     0,   133,     0,     0,   137,   139,   387,   388,   478,
       0,     0,   523,   478,   163,   166,   172,   174,   179,   182,
     184,  -168,  -168,   120,   189,   191,     0,     0,     0,   196,
     198,   200,   385,     0,   205,   207,     0,   210,     0,   213,
       0,     0,     0,     0,   222,   382,   225,     0,   227,   229,
     231,   233,     0,   236,   142,     0,     0,   240,   241,   244,
     245,   246,     0,   248,   115,   116,   117,   118,   119,     0,
       0,   257,     0,   260,   261,   263,   264,   475,     0,   269,
     273,   274,     0,     0,     0,   136,   303,   305,     0,     0,
     212,     0,   268,   478,   108,   109,   110,   111,   422,   423,
       0,   112,   113,   114,     0,   424,   425,   115,   116,   117,
     118,   119,     0,   129,   130,   322,     0,   108,   109,   110,
     111,   316,     0,     0,   112,   113,   114,   581,   317,     0,
     115,   116,   117,   118,   119,   115,   116,   117,   118,   119,
     324,     0,   108,   109,   110,   111,     0,     0,   327,   112,
     113,   114,   596,     0,   597,   115,   116,   117,   118,   119,
     146,   334,     0,   147,     0,   338,     0,   339,   340,   148,
     149,   150,   342,   598,   599,   600,     0,     0,     0,   422,
     423,     0,   346,     0,   348,   349,   424,   425,   587,   350,
       0,     0,   354,     0,   356,     0,     0,     0,     0,   359,
     360,     0,   120,   361,     0,     0,   363,     0,   364,   365,
       0,   366,   368,     0,   370,     0,     0,   373,     0,     0,
     375,   376,     0,   377,     0,   378,     0,   379,     0,   380,
       0,   381,     0,     0,   386,     0,     0,     0,     0,   389,
     390,     0,     0,     0,   391,   120,     0,   393,   394,   190,
     396,   397,   147,   399,     0,     0,     0,   121,   148,   149,
     150,     0,     0,   488,   405,   489,     0,   406,   120,   407,
     129,   130,   408,   410,     0,     0,     0,     0,     0,     0,
     121,     0,   157,   157,   157,   165,   170,     0,   157,     0,
       0,   165,   157,   120,     0,     0,     0,     0,     0,   165,
     157,   438,     0,   439,   203,   121,     0,     0,     0,     0,
     214,   216,   218,   165,   221,   223,     0,   449,   450,   157,
     157,   157,   157,   165,     0,     0,     0,     0,   592,   157,
     108,   109,   110,   111,     0,     0,     0,   112,   113,   114,
       0,     0,     0,   115,   116,   117,   118,   119,   128,   479,
     108,   109,   110,   111,   484,   485,   153,   112,   113,   114,
       0,     0,     0,   115,   116,   117,   118,   119,     0,   129,
     130,     0,     0,     0,   108,   109,   110,   111,     0,     0,
     492,   112,   113,   114,     0,     0,     0,   115,   116,   117,
     118,   119,   498,     0,     0,   499,     0,   500,   501,   502,
     503,     0,     0,   108,   109,   110,   111,     0,     0,   509,
     112,   113,   114,     0,     0,     0,   115,   116,   117,   118,
     119,   108,   109,   110,   111,     0,     0,     0,   112,   113,
     114,     0,     0,     0,   115,   116,   117,   118,   119,     0,
     474,     0,     0,   537,     0,   539,     0,     0,   540,   157,
     541,   157,   542,   543,     0,     0,   544,     0,     0,     0,
       0,     0,   546,   547,     0,     0,   548,     0,   550,   551,
     552,     0,     0,   555,     0,     0,   157,   556,     0,     0,
     559,   120,     0,   560,   561,   562,   563,   564,   565,     0,
       0,   567,     0,   568,   569,     0,     0,     0,   154,     0,
       0,   570,   571,   572,   573,   574,     0,     0,     0,   312,
     313,   314,   315,     0,     0,   108,   109,   110,   111,     0,
       0,     0,   112,   113,   114,   120,     0,     0,   115,   116,
     117,   118,   119,   312,   313,   314,   315,   121,     0,   128,
       0,  -191,  -191,  -191,  -191,   165,     0,     0,  -191,  -191,
    -191,     0,     0,     0,   120,     0,   108,   109,   110,   111,
     129,   130,     0,   112,   113,   114,   121,     0,   128,   154,
    -191,  -191,  -191,  -191,     0,     0,     0,  -191,  -191,  -191,
     312,   313,   314,   315,     0,     0,     0,   165,     0,   129,
     130,     0,     0,   157,   165,     0,     0,     0,   165,     0,
       0,   165,   128,     0,   108,   109,   110,   111,     0,   157,
     153,   112,   113,   114,     0,   165,     0,   115,   116,   117,
     118,   119,     0,   129,   130,   474,     0,     0,     0,     0,
     128,     0,  -172,  -172,  -172,  -172,     0,     0,  -172,  -172,
    -172,  -172,     0,   178,     0,  -172,  -172,  -172,  -172,  -172,
       0,   129,   130,     0,     0,     0,   157,   136,   128,     0,
       0,     0,     0,     0,     0,     0,   120,     0,     0,   429,
       0,   157,   430,   115,   116,   117,   118,   119,     0,   129,
     130,   431,     0,   432,     0,  -191,   136,     0,     0,     0,
       0,   128,     0,   108,   109,   110,   111,     0,     0,   153,
     112,   113,   114,     0,     0,     0,   115,   116,   117,   118,
     119,     0,   129,   130,   128,   165,   108,   109,   110,   111,
       0,     0,     0,   112,   113,   114,     0,     0,     0,   115,
     116,   117,   118,   119,     0,   129,   130,     0,     0,     0,
       1,   108,   109,   110,   111,     0,     2,   463,   112,   113,
     114,     0,   154,     0,     0,     3,     0,   173,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,     0,     0,    19,    20,    21,    22,    23,
    -172,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,   154,    84,    85,    86,    87,     0,     0,     0,     0,
    -168,     0,  -168,  -168,  -168,  -168,     0,     0,  -168,  -168,
    -168,  -168,     0,     0,   154,  -168,  -168,  -168,  -168,  -168,
     108,   109,   110,   111,     0,     1,     0,   112,   113,   114,
       0,     2,     0,    88,     0,     0,     0,     0,     0,     0,
       0,    89,    90,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,     0,     0,
      19,    20,    21,    22,    23,     0,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,    84,    85,    86,
      87,     0,     0,     0,     0,     0,   452,     0,   108,   109,
     110,   111,     0,     0,   153,   112,   113,   114,     0,     0,
    -168,   115,   116,   117,   118,   119,   495,     0,   108,   109,
     110,   111,     0,     0,   153,   112,   113,   114,     0,     0,
       0,   115,   116,   117,   118,   119,    89,    90,   454,     0,
     108,   109,   110,   111,     0,     0,     0,   112,   113,   114,
       0,     0,     0,   115,   116,   117,   118,   119,   108,   109,
     110,   111,     0,     0,   153,   112,   113,   114,     0,     0,
       0,   115,   116,   117,   118,   119,   108,   109,   110,   111,
       0,     0,     0,   112,   113,   114,     0,     0,     0,   115,
     116,   117,   118,   119,     0,     0,     0,     0,     0,     0,
     108,   109,   110,   111,     0,     0,     0,   112,   113,   114,
     108,   109,   110,   111,     0,     0,     0,   112,   113,   114,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   108,   109,   110,   111,     0,     0,
       0,   112,   113,   114,     0,     0,   154,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   154,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   154,     0,     0,     0,
     278,     0,   279,   280,     0,     0,   281,     0,   282,     0,
     283,     0,     0,     0,   154,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   413,   414,   415,   416,   417,   418,   531,   532,
       0,     0,     0,     0,     0,   424,   533,   534
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-428))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       1,    18,     8,     9,   355,     6,     7,    14,    15,    16,
       1,     1,     1,   119,     1,     1,   122,    24,     1,   149,
       1,    14,    15,    16,    17,   122,   149,    33,    34,     1,
      23,    24,    22,    23,    35,    32,    22,    23,    31,    22,
      23,    22,    23,   470,     1,     1,   323,   474,   145,     6,
       7,   147,    45,     1,    51,    52,    53,    54,     1,   345,
     114,     1,    55,     1,    61,   119,    22,    23,    74,    75,
     122,    77,    73,    79,    22,    23,   362,     1,    35,    22,
      23,    82,    22,    23,    22,    23,     1,    88,     3,     4,
       5,     6,     1,   145,   147,    10,    11,    12,    22,    23,
       1,    16,    17,    18,    19,    20,   119,    22,    23,   117,
     387,   331,     0,    22,    23,   335,    73,   125,   164,   165,
     121,    22,    23,    21,   154,   114,   421,   554,    39,   119,
      41,   426,   119,   134,   135,   430,   119,   138,    22,   140,
     435,   436,   143,   144,   145,   151,    57,    22,    21,   155,
     156,   157,    14,   147,    38,   161,   162,   148,    15,   165,
     147,   119,    15,   147,   121,   119,   155,   173,   119,   152,
     156,   177,   162,   163,   164,   165,   150,   134,   135,   160,
     186,   138,   119,   140,   181,   152,   183,   151,   194,   151,
     162,   163,   164,   165,   114,   167,   114,   114,   199,   155,
     201,    28,    29,    30,   119,   153,   557,   558,   214,   114,
     216,   208,   218,   219,   154,   221,   145,   114,   119,   162,
     163,   164,   165,   114,   519,     1,   150,   522,   523,   119,
     525,   526,   527,   119,   235,   120,   119,   238,   239,   154,
     149,   247,   199,   120,   201,   119,    22,    23,   117,   118,
     145,   166,   119,   155,   255,   124,   125,   258,   259,   114,
     266,   262,    89,    90,   265,     3,     4,     5,     6,   275,
     276,     1,    10,    11,    12,   114,   122,   278,   279,   122,
     122,   282,   283,   118,   277,     1,   581,     3,     4,     5,
       6,   122,    22,    23,    10,    11,    12,   122,   255,   122,
     119,   258,   259,   118,   122,   262,    22,    23,   265,   117,
     118,   312,   313,   314,   315,   122,   124,   125,   325,    92,
     337,    16,    17,    18,    19,    20,   319,   328,   329,    81,
       1,   332,   325,   326,   351,   336,   332,   330,   146,   337,
     333,   493,   459,   119,   341,    16,    17,    18,    19,    20,
     351,    22,    23,   437,   347,   312,   313,   314,   315,    -1,
      -1,     3,     4,     5,     6,    11,    12,     1,    10,    11,
      12,   328,   143,   144,   145,    -1,    -1,   383,   108,    25,
      26,   388,    16,    17,    18,    19,    20,   162,   163,   164,
     165,   392,    -1,   117,   118,   388,    -1,    43,    44,     1,
     124,   125,    48,   404,    50,    -1,   403,     1,   409,   147,
     411,     1,    58,    59,    16,    17,    18,    19,    20,    65,
      22,    23,   146,   161,    70,    71,    72,     1,    22,    23,
      24,   147,    22,    23,    80,   392,   437,    -1,    -1,    85,
      86,    15,    -1,   117,   118,   161,    -1,   404,    22,    23,
     124,   125,    -1,   470,   447,    -1,   151,   474,   459,   465,
       1,    -1,     5,    -1,    -1,     8,     9,   238,   239,   470,
      -1,    -1,   146,   474,    17,    18,    19,    20,    21,    22,
      23,    22,    23,   154,    27,    28,    -1,    -1,    -1,    32,
      33,    34,   493,    -1,    37,    38,    -1,    40,    -1,    42,
      -1,    -1,    -1,    -1,    47,   147,    49,    -1,    51,    52,
      53,    54,    -1,    56,   148,    -1,    -1,    60,    61,    62,
      63,    64,    -1,    66,    16,    17,    18,    19,    20,    -1,
      -1,    74,    -1,    76,    77,    78,    79,   554,    -1,    82,
      83,    84,    -1,    -1,    -1,   119,    89,    90,    -1,    -1,
     152,    -1,     1,   554,     3,     4,     5,     6,   117,   118,
      -1,    10,    11,    12,    -1,   124,   125,    16,    17,    18,
      19,    20,    -1,    22,    23,     1,    -1,     3,     4,     5,
       6,   124,    -1,    -1,    10,    11,    12,   146,   131,    -1,
      16,    17,    18,    19,    20,    16,    17,    18,    19,    20,
       1,    -1,     3,     4,     5,     6,    -1,    -1,   151,    10,
      11,    12,   116,    -1,   118,    16,    17,    18,    19,    20,
       1,   164,    -1,     4,    -1,   168,    -1,   170,   171,    10,
      11,    12,   175,   137,   138,   139,    -1,    -1,    -1,   117,
     118,    -1,   185,    -1,   187,   188,   124,   125,   126,   192,
      -1,    -1,   195,    -1,   197,    -1,    -1,    -1,    -1,   202,
     203,    -1,   154,   206,    -1,    -1,   209,    -1,   211,   212,
      -1,   214,   215,    -1,   217,    -1,    -1,   220,    -1,    -1,
     223,   224,    -1,   226,    -1,   228,    -1,   230,    -1,   232,
      -1,   234,    -1,    -1,   237,    -1,    -1,    -1,    -1,   242,
     243,    -1,    -1,    -1,   247,   154,    -1,   250,   251,     1,
     253,   254,     4,   256,    -1,    -1,    -1,   166,    10,    11,
      12,    -1,    -1,   369,   267,   371,    -1,   270,   154,   272,
      22,    23,   275,   276,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    -1,    14,    15,    16,    17,    18,    -1,    20,    -1,
      -1,    23,    24,   154,    -1,    -1,    -1,    -1,    -1,    31,
      32,   304,    -1,   306,    36,   166,    -1,    -1,    -1,    -1,
      42,    43,    44,    45,    46,    47,    -1,   320,   321,    51,
      52,    53,    54,    55,    -1,    -1,    -1,    -1,     1,    61,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,     1,   352,
       3,     4,     5,     6,   357,   358,     9,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,    22,
      23,    -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,
     383,    10,    11,    12,    -1,    -1,    -1,    16,    17,    18,
      19,    20,   395,    -1,    -1,   398,    -1,   400,   401,   402,
     403,    -1,    -1,     3,     4,     5,     6,    -1,    -1,   412,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,
     113,    -1,    -1,   446,    -1,   448,    -1,    -1,   451,   181,
     453,   183,   455,   456,    -1,    -1,   459,    -1,    -1,    -1,
      -1,    -1,   465,   466,    -1,    -1,   469,    -1,   471,   472,
     473,    -1,    -1,   476,    -1,    -1,   208,   480,    -1,    -1,
     483,   154,    -1,   486,   487,   488,   489,   490,   491,    -1,
      -1,   494,    -1,   496,   497,    -1,    -1,    -1,   151,    -1,
      -1,   504,   505,   506,   507,   508,    -1,    -1,    -1,   162,
     163,   164,   165,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,   154,    -1,    -1,    16,    17,
      18,    19,    20,   162,   163,   164,   165,   166,    -1,     1,
      -1,     3,     4,     5,     6,   277,    -1,    -1,    10,    11,
      12,    -1,    -1,    -1,   154,    -1,     3,     4,     5,     6,
      22,    23,    -1,    10,    11,    12,   166,    -1,     1,   151,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
     162,   163,   164,   165,    -1,    -1,    -1,   319,    -1,    22,
      23,    -1,    -1,   325,   326,    -1,    -1,    -1,   330,    -1,
      -1,   333,     1,    -1,     3,     4,     5,     6,    -1,   341,
       9,    10,    11,    12,    -1,   347,    -1,    16,    17,    18,
      19,    20,    -1,    22,    23,   113,    -1,    -1,    -1,    -1,
       1,    -1,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    -1,    14,    -1,    16,    17,    18,    19,    20,
      -1,    22,    23,    -1,    -1,    -1,   388,   119,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,   116,
      -1,   403,   119,    16,    17,    18,    19,    20,    -1,    22,
      23,   128,    -1,   130,    -1,   147,   119,    -1,    -1,    -1,
      -1,     1,    -1,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    -1,    22,    23,     1,   447,     3,     4,     5,     6,
      -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,    16,
      17,    18,    19,    20,    -1,    22,    23,    -1,    -1,    -1,
       7,     3,     4,     5,     6,    -1,    13,     9,    10,    11,
      12,    -1,   151,    -1,    -1,    22,    -1,   156,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    -1,    -1,    42,    43,    44,    45,    46,
     151,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   151,   109,   110,   111,   112,    -1,    -1,    -1,    -1,
       1,    -1,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    -1,    -1,   151,    16,    17,    18,    19,    20,
       3,     4,     5,     6,    -1,     7,    -1,    10,    11,    12,
      -1,    13,    -1,   150,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   158,   159,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    -1,    -1,
      42,    43,    44,    45,    46,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,    -1,   109,   110,   111,
     112,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
     151,    16,    17,    18,    19,    20,     1,    -1,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
      -1,    16,    17,    18,    19,    20,   158,   159,     1,    -1,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    16,    17,    18,    19,    20,     3,     4,
       5,     6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,
      -1,    16,    17,    18,    19,    20,     3,     4,     5,     6,
      -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,    16,
      17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,    -1,    -1,
      -1,    10,    11,    12,    -1,    -1,   151,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   151,    -1,    -1,    -1,
     113,    -1,   115,   116,    -1,    -1,   119,    -1,   121,    -1,
     123,    -1,    -1,    -1,   151,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   135,   136,   137,   138,   139,   140,   117,   118,
      -1,    -1,    -1,    -1,    -1,   124,   125,   126
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
     104,   105,   106,   107,   109,   110,   111,   112,   150,   158,
     159,   169,   170,   172,   173,   174,   175,   176,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   218,     3,     4,
       5,     6,    10,    11,    12,    16,    17,    18,    19,    20,
     154,   166,   193,   202,   205,   213,   215,   216,     1,    22,
      23,   160,   171,   171,   205,   205,   119,   171,   201,   171,
     201,     1,   148,   189,   189,   189,     1,     4,    10,    11,
      12,   196,   214,     9,   151,   198,   200,   202,   203,   204,
     216,   198,   198,   171,   200,   202,   171,   193,   194,   195,
     202,   156,   171,   156,   171,   197,   198,   200,    14,   171,
     191,   192,   171,   192,   171,   200,   198,   189,   189,   171,
       1,   171,   196,   196,   196,   200,   171,   197,   171,   201,
     171,   201,   205,   202,   149,   171,   187,   171,   192,   187,
     171,   187,   152,   171,   202,   189,   202,   189,   202,   200,
     151,   202,   171,   202,   189,   171,   189,   171,   197,   171,
     197,   171,   197,   171,   197,   200,   171,   187,   189,   189,
     171,   171,   197,   153,   171,   171,   171,   189,   171,   149,
     188,   147,   147,   189,   189,   189,   205,   171,   201,   201,
     171,   171,   201,   171,   171,   201,   189,   188,     1,   171,
     190,   205,   108,   171,   171,   189,   189,    21,   113,   115,
     116,   119,   121,   123,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   216,   220,   171,   196,   171,   196,     0,   172,    22,
     205,   154,   162,   163,   164,   165,   171,   171,   205,   205,
     205,   205,     1,   190,     1,   190,   190,   171,   201,   201,
     201,   201,   201,   201,   171,   201,    21,   119,   171,   171,
     171,   201,   171,    14,   197,   197,   171,   201,   171,   171,
     171,    15,     1,   201,   171,   177,   171,   205,   205,   171,
     171,   171,   197,   171,   171,   171,   171,   201,   171,   201,
     171,   201,   201,   171,   201,   171,   171,   171,   171,   171,
     171,   171,   147,   209,   210,   216,   171,   190,   190,   171,
     171,   171,   201,   171,   171,   147,   171,   171,   205,   171,
     205,   205,   205,   205,   201,   171,   171,   171,   171,   201,
     171,   201,   200,   135,   136,   137,   138,   139,   140,   216,
     216,   119,   117,   118,   124,   125,   146,   221,   222,   116,
     119,   128,   130,   216,   216,   119,   119,   119,   171,   171,
       1,   167,   205,   205,   205,   205,   199,   201,   200,   171,
     171,   199,     1,   198,     1,   200,   205,   147,   161,   211,
     212,   216,   200,     9,   203,   209,   200,   216,   195,   197,
      15,   206,   206,   200,   113,   193,   207,   208,   216,   171,
     147,   217,   218,   219,   171,   171,   206,   152,   189,   189,
     151,   151,   171,   201,   199,     1,   198,   205,   171,   171,
     171,   171,   171,   171,   197,   205,   216,   216,    24,   171,
     114,   114,   114,   114,   114,   114,   114,   119,   221,   146,
     221,   145,   119,   146,   221,   119,   119,   119,   122,   221,
     221,   117,   118,   125,   126,   216,   222,   171,   200,   171,
     171,   171,   171,   171,   171,   212,   171,   171,   171,   207,
     171,   171,   171,   207,   155,   171,   171,   120,   120,   171,
     171,   171,   171,   171,   171,   171,   210,   171,   171,   171,
     171,   171,   171,   171,   171,   119,   117,   125,   221,   145,
     221,   146,   221,   122,   145,   221,   221,   126,   221,   119,
       1,   114,     1,   207,   218,   218,   116,   118,   137,   138,
     139,   114,   114,   122,   221,   122,   122,   145,   122,   122,
     122,   122,   118,   119,   122,   122,   118
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


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
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

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
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


/*----------.
| yyparse.  |
`----------*/

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
  if (yypact_value_is_default (yyn))
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
      if (yytable_value_is_error (yyn))
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

/* Line 1806 of yacc.c  */
#line 195 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 196 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 197 "mon_parse.y"
    { new_cmd = 1; asm_mode = 0;  (yyval.i) = 0; }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 206 "mon_parse.y"
    { return ERR_EXPECT_END_CMD; }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 221 "mon_parse.y"
    { return ERR_BAD_CMD; }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 225 "mon_parse.y"
    { mon_bank(e_default_space, NULL); }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 227 "mon_parse.y"
    { mon_bank((yyvsp[(2) - (3)].i), NULL); }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 229 "mon_parse.y"
    { mon_bank(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 231 "mon_parse.y"
    { mon_bank((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 233 "mon_parse.y"
    { mon_jump((yyvsp[(2) - (3)].a)); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 235 "mon_parse.y"
    { mon_go(); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 237 "mon_parse.y"
    { mon_display_io_regs(0); }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 239 "mon_parse.y"
    { mon_display_io_regs((yyvsp[(2) - (3)].a)); }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 241 "mon_parse.y"
    { monitor_cpu_type_set(""); }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 243 "mon_parse.y"
    { monitor_cpu_type_set((yyvsp[(2) - (3)].str)); }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 245 "mon_parse.y"
    { mon_cpuhistory(-1); }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 247 "mon_parse.y"
    { mon_cpuhistory((yyvsp[(3) - (4)].i)); }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 249 "mon_parse.y"
    { mon_instruction_return(); }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 251 "mon_parse.y"
    { machine_write_snapshot((yyvsp[(2) - (3)].str),0,0,0); /* FIXME */ }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 253 "mon_parse.y"
    { machine_read_snapshot((yyvsp[(2) - (3)].str), 0); }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 255 "mon_parse.y"
    { mon_instructions_step(-1); }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 257 "mon_parse.y"
    { mon_instructions_step((yyvsp[(3) - (4)].i)); }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 259 "mon_parse.y"
    { mon_instructions_next(-1); }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 261 "mon_parse.y"
    { mon_instructions_next((yyvsp[(3) - (4)].i)); }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 263 "mon_parse.y"
    { mon_stack_up(-1); }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 265 "mon_parse.y"
    { mon_stack_up((yyvsp[(3) - (4)].i)); }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 267 "mon_parse.y"
    { mon_stack_down(-1); }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 269 "mon_parse.y"
    { mon_stack_down((yyvsp[(3) - (4)].i)); }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 271 "mon_parse.y"
    { mon_display_screen(); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 276 "mon_parse.y"
    { (monitor_cpu_for_memspace[default_memspace]->mon_register_print)(default_memspace); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 278 "mon_parse.y"
    { (monitor_cpu_for_memspace[(yyvsp[(2) - (3)].i)]->mon_register_print)((yyvsp[(2) - (3)].i)); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 283 "mon_parse.y"
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[(4) - (5)].str));
                    }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 288 "mon_parse.y"
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[(2) - (3)].str));
                    }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 293 "mon_parse.y"
    { mon_save_symbols((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 295 "mon_parse.y"
    { mon_save_symbols(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 297 "mon_parse.y"
    { mon_add_name_to_symbol_table((yyvsp[(2) - (5)].a), (yyvsp[(4) - (5)].str)); }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 299 "mon_parse.y"
    { mon_remove_name_from_symbol_table(e_default_space, (yyvsp[(2) - (3)].str)); }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 301 "mon_parse.y"
    { mon_remove_name_from_symbol_table((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 303 "mon_parse.y"
    { mon_print_symbol_table((yyvsp[(2) - (3)].i)); }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 305 "mon_parse.y"
    { mon_print_symbol_table(e_default_space); }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 307 "mon_parse.y"
    {
                        mon_add_name_to_symbol_table((yyvsp[(3) - (4)].a), mon_prepend_dot_to_name((yyvsp[(1) - (4)].str)));
                    }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 311 "mon_parse.y"
    {
                        mon_add_name_to_symbol_table((yyvsp[(3) - (5)].a), mon_prepend_dot_to_name((yyvsp[(1) - (5)].str)));
                    }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 317 "mon_parse.y"
    { mon_start_assemble_mode((yyvsp[(2) - (2)].a), NULL); }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 318 "mon_parse.y"
    { }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 320 "mon_parse.y"
    { mon_start_assemble_mode((yyvsp[(2) - (3)].a), NULL); }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 322 "mon_parse.y"
    { mon_disassemble_lines((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1]); }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 324 "mon_parse.y"
    { mon_disassemble_lines(BAD_ADDR, BAD_ADDR); }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 328 "mon_parse.y"
    { mon_memory_move((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1], (yyvsp[(4) - (5)].a)); }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 330 "mon_parse.y"
    { mon_memory_compare((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1], (yyvsp[(4) - (5)].a)); }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 332 "mon_parse.y"
    { mon_memory_fill((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1],(unsigned char *)(yyvsp[(4) - (5)].str)); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 334 "mon_parse.y"
    { mon_memory_hunt((yyvsp[(2) - (5)].range)[0], (yyvsp[(2) - (5)].range)[1],(unsigned char *)(yyvsp[(4) - (5)].str)); }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 336 "mon_parse.y"
    { mon_memory_display((yyvsp[(2) - (5)].rt), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], DF_PETSCII); }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 338 "mon_parse.y"
    { mon_memory_display(default_radix, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_PETSCII); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 340 "mon_parse.y"
    { mon_memory_display(default_radix, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 342 "mon_parse.y"
    { mon_memory_display_data((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], 8, 8); }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 344 "mon_parse.y"
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 8, 8); }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 346 "mon_parse.y"
    { mon_memory_display_data((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], 24, 21); }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 348 "mon_parse.y"
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 24, 21); }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 350 "mon_parse.y"
    { mon_memory_display(0, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_PETSCII); }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 352 "mon_parse.y"
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 354 "mon_parse.y"
    { mon_memory_display(0, (yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], DF_SCREEN_CODE); }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 356 "mon_parse.y"
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_SCREEN_CODE); }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 358 "mon_parse.y"
    { mon_memmap_zap(); }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 360 "mon_parse.y"
    { mon_memmap_show(-1,BAD_ADDR,BAD_ADDR); }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 362 "mon_parse.y"
    { mon_memmap_show((yyvsp[(3) - (4)].i),BAD_ADDR,BAD_ADDR); }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 364 "mon_parse.y"
    { mon_memmap_show((yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].range)[0],(yyvsp[(4) - (5)].range)[1]); }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 366 "mon_parse.y"
    { mon_memmap_save((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i)); }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 370 "mon_parse.y"
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

/* Line 1806 of yacc.c  */
#line 379 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 382 "mon_parse.y"
    {
                      mon_breakpoint_add_checkpoint((yyvsp[(2) - (3)].range)[0], (yyvsp[(2) - (3)].range)[1], TRUE, e_exec, TRUE);
                  }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 386 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 389 "mon_parse.y"
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

/* Line 1806 of yacc.c  */
#line 398 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 401 "mon_parse.y"
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

/* Line 1806 of yacc.c  */
#line 410 "mon_parse.y"
    { mon_breakpoint_print_checkpoints(); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 415 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_ON, (yyvsp[(2) - (3)].i)); }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 417 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_ON, -1); }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 419 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_OFF, (yyvsp[(2) - (3)].i)); }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 421 "mon_parse.y"
    { mon_breakpoint_switch_checkpoint(e_OFF, -1); }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 423 "mon_parse.y"
    { mon_breakpoint_set_ignore_count((yyvsp[(2) - (3)].i), -1); }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 425 "mon_parse.y"
    { mon_breakpoint_set_ignore_count((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].i)); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 427 "mon_parse.y"
    { mon_breakpoint_delete_checkpoint((yyvsp[(2) - (3)].i)); }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 429 "mon_parse.y"
    { mon_breakpoint_delete_checkpoint(-1); }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 431 "mon_parse.y"
    { mon_breakpoint_set_checkpoint_condition((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].cond_node)); }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 433 "mon_parse.y"
    { mon_breakpoint_set_checkpoint_command((yyvsp[(2) - (5)].i), (yyvsp[(4) - (5)].str)); }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 435 "mon_parse.y"
    { return ERR_EXPECT_STRING; }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 439 "mon_parse.y"
    { sidefx = (((yyvsp[(2) - (3)].action) == e_TOGGLE) ? (sidefx ^ 1) : (yyvsp[(2) - (3)].action)); }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 441 "mon_parse.y"
    {
                         mon_out("I/O side effects are %s\n",
                                   sidefx ? "enabled" : "disabled");
                     }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 446 "mon_parse.y"
    { default_radix = (yyvsp[(2) - (3)].rt); }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 448 "mon_parse.y"
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

  case 110:

/* Line 1806 of yacc.c  */
#line 466 "mon_parse.y"
    { monitor_change_device((yyvsp[(2) - (3)].i)); }
    break;

  case 111:

/* Line 1806 of yacc.c  */
#line 468 "mon_parse.y"
    { mon_export(); }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 470 "mon_parse.y"
    { mon_quit(); YYACCEPT; }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 472 "mon_parse.y"
    { mon_exit(); YYACCEPT; }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 476 "mon_parse.y"
    { mon_drive_execute_disk_cmd((yyvsp[(2) - (3)].str)); }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 478 "mon_parse.y"
    { mon_out("\t%d\n",(yyvsp[(2) - (3)].i)); }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 480 "mon_parse.y"
    { mon_command_print_help(NULL); }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 482 "mon_parse.y"
    { mon_command_print_help((yyvsp[(2) - (3)].str)); }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 484 "mon_parse.y"
    { mon_print_convert((yyvsp[(2) - (3)].i)); }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 486 "mon_parse.y"
    { mon_change_dir((yyvsp[(2) - (3)].str)); }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 488 "mon_parse.y"
    { mon_keyboard_feed((yyvsp[(2) - (3)].str)); }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 490 "mon_parse.y"
    { mon_backtrace(); }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 492 "mon_parse.y"
    { mon_show_dir((yyvsp[(2) - (3)].str)); }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 494 "mon_parse.y"
    { mon_show_pwd(); }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 496 "mon_parse.y"
    { mon_screenshot_save((yyvsp[(2) - (3)].str),-1); }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 498 "mon_parse.y"
    { mon_screenshot_save((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i)); }
    break;

  case 126:

/* Line 1806 of yacc.c  */
#line 500 "mon_parse.y"
    { mon_resource_get((yyvsp[(2) - (3)].str)); }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 502 "mon_parse.y"
    { mon_resource_set((yyvsp[(2) - (4)].str),(yyvsp[(3) - (4)].str)); }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 504 "mon_parse.y"
    { resources_load((yyvsp[(2) - (3)].str)); }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 506 "mon_parse.y"
    { resources_save((yyvsp[(2) - (3)].str)); }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 508 "mon_parse.y"
    { mon_reset_machine(-1); }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 510 "mon_parse.y"
    { mon_reset_machine((yyvsp[(3) - (4)].i)); }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 512 "mon_parse.y"
    { mon_tape_ctrl((yyvsp[(3) - (4)].i)); }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 514 "mon_parse.y"
    { mon_cart_freeze(); }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 516 "mon_parse.y"
    { }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 518 "mon_parse.y"
    { mon_stopwatch_reset(); }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 520 "mon_parse.y"
    { mon_stopwatch_show("Stopwatch: ", "\n"); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 524 "mon_parse.y"
    { mon_file_load((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].a), FALSE); }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 526 "mon_parse.y"
    { mon_file_load((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].a), TRUE); }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 528 "mon_parse.y"
    { mon_file_save((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], FALSE); }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 530 "mon_parse.y"
    { return ERR_EXPECT_DEVICE_NUM; }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 532 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 534 "mon_parse.y"
    { mon_file_save((yyvsp[(2) - (5)].str), (yyvsp[(3) - (5)].i), (yyvsp[(4) - (5)].range)[0], (yyvsp[(4) - (5)].range)[1], TRUE); }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 536 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 538 "mon_parse.y"
    { mon_file_verify((yyvsp[(2) - (5)].str),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 540 "mon_parse.y"
    { return ERR_EXPECT_ADDRESS; }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 542 "mon_parse.y"
    { mon_drive_block_cmd(0,(yyvsp[(2) - (5)].i),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 544 "mon_parse.y"
    { mon_drive_block_cmd(1,(yyvsp[(2) - (5)].i),(yyvsp[(3) - (5)].i),(yyvsp[(4) - (5)].a)); }
    break;

  case 148:

/* Line 1806 of yacc.c  */
#line 546 "mon_parse.y"
    { mon_drive_list(-1); }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 548 "mon_parse.y"
    { mon_drive_list((yyvsp[(2) - (3)].i)); }
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 550 "mon_parse.y"
    { mon_attach((yyvsp[(2) - (4)].str),(yyvsp[(3) - (4)].i)); }
    break;

  case 151:

/* Line 1806 of yacc.c  */
#line 552 "mon_parse.y"
    { mon_detach((yyvsp[(2) - (3)].i)); }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 554 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (3)].str),0,1); }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 556 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i),1); }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 558 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (3)].str),0,0); }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 560 "mon_parse.y"
    { mon_autostart((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].i),0); }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 564 "mon_parse.y"
    { mon_record_commands((yyvsp[(2) - (3)].str)); }
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 566 "mon_parse.y"
    { mon_end_recording(); }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 568 "mon_parse.y"
    { mon_playback_init((yyvsp[(2) - (3)].str)); }
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 572 "mon_parse.y"
    { mon_memory_fill((yyvsp[(2) - (4)].a), BAD_ADDR, (unsigned char *)(yyvsp[(3) - (4)].str)); }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 574 "mon_parse.y"
    { printf("Not yet.\n"); }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 578 "mon_parse.y"
    { yydebug = 1; }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 581 "mon_parse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 584 "mon_parse.y"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 585 "mon_parse.y"
    { (yyval.str) = NULL; }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 589 "mon_parse.y"
    { return ERR_EXPECT_FILENAME; }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 593 "mon_parse.y"
    { return ERR_EXPECT_DEVICE_NUM; }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 596 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (2)].i) | (yyvsp[(2) - (2)].i); }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 597 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 600 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 601 "mon_parse.y"
    { (yyval.i) = 0; }
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 604 "mon_parse.y"
    {
                                    if (!mon_register_valid(default_memspace, (yyvsp[(1) - (1)].reg))) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    (yyval.i) = new_reg(default_memspace, (yyvsp[(1) - (1)].reg));
                                }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 610 "mon_parse.y"
    {
                                    if (!mon_register_valid((yyvsp[(1) - (2)].i), (yyvsp[(2) - (2)].reg))) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    (yyval.i) = new_reg((yyvsp[(1) - (2)].i), (yyvsp[(2) - (2)].reg));
                                }
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 623 "mon_parse.y"
    { (monitor_cpu_for_memspace[reg_memspace((yyvsp[(1) - (3)].i))]->mon_register_set_val)(reg_memspace((yyvsp[(1) - (3)].i)), reg_regid((yyvsp[(1) - (3)].i)), (WORD) (yyvsp[(3) - (3)].i)); }
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 626 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 627 "mon_parse.y"
    { return ERR_EXPECT_CHECKNUM; }
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 631 "mon_parse.y"
    { (yyval.range)[0] = (yyvsp[(1) - (1)].a); (yyval.range)[1] = BAD_ADDR; }
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 634 "mon_parse.y"
    { (yyval.range)[0] = (yyvsp[(1) - (3)].a); (yyval.range)[1] = (yyvsp[(3) - (3)].a); }
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 636 "mon_parse.y"
    { if (resolve_range(e_default_space, (yyval.range), (yyvsp[(1) - (1)].str))) return ERR_ADDR_TOO_BIG; }
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 638 "mon_parse.y"
    { if (resolve_range((yyvsp[(1) - (3)].i), (yyval.range), (yyvsp[(3) - (3)].str))) return ERR_ADDR_TOO_BIG; }
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 641 "mon_parse.y"
    { (yyval.a) = (yyvsp[(2) - (2)].a); }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 642 "mon_parse.y"
    { (yyval.a) = BAD_ADDR; }
    break;

  case 187:

/* Line 1806 of yacc.c  */
#line 646 "mon_parse.y"
    {
             (yyval.a) = new_addr(e_default_space,(yyvsp[(1) - (1)].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
    break;

  case 188:

/* Line 1806 of yacc.c  */
#line 651 "mon_parse.y"
    {
             (yyval.a) = new_addr((yyvsp[(1) - (3)].i), (yyvsp[(3) - (3)].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
    break;

  case 189:

/* Line 1806 of yacc.c  */
#line 656 "mon_parse.y"
    {
             temp = mon_symbol_table_lookup_addr(e_default_space, (yyvsp[(1) - (1)].str));
             if (temp >= 0)
                 (yyval.a) = new_addr(e_default_space, temp);
             else
                 return ERR_UNDEFINED_LABEL;
         }
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 669 "mon_parse.y"
    { (yyval.i) = e_comp_space; }
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 670 "mon_parse.y"
    { (yyval.i) = e_disk8_space; }
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 671 "mon_parse.y"
    { (yyval.i) = e_disk9_space; }
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 672 "mon_parse.y"
    { (yyval.i) = e_disk10_space; }
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 673 "mon_parse.y"
    { (yyval.i) = e_disk11_space; }
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 676 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); if (!CHECK_ADDR((yyvsp[(1) - (1)].i))) return ERR_ADDR_TOO_BIG; }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 678 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 680 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) + (yyvsp[(3) - (3)].i); }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 681 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) - (yyvsp[(3) - (3)].i); }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 682 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (3)].i) * (yyvsp[(3) - (3)].i); }
    break;

  case 202:

/* Line 1806 of yacc.c  */
#line 683 "mon_parse.y"
    { (yyval.i) = ((yyvsp[(3) - (3)].i)) ? ((yyvsp[(1) - (3)].i) / (yyvsp[(3) - (3)].i)) : 1; }
    break;

  case 203:

/* Line 1806 of yacc.c  */
#line 684 "mon_parse.y"
    { (yyval.i) = (yyvsp[(2) - (3)].i); }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 685 "mon_parse.y"
    { return ERR_MISSING_CLOSE_PAREN; }
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 686 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 689 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(2) - (2)].cond_node); }
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 690 "mon_parse.y"
    { (yyval.cond_node) = 0; }
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 693 "mon_parse.y"
    {
               (yyval.cond_node) = new_cond; (yyval.cond_node)->is_parenthized = FALSE;
               (yyval.cond_node)->child1 = (yyvsp[(1) - (3)].cond_node); (yyval.cond_node)->child2 = (yyvsp[(3) - (3)].cond_node); (yyval.cond_node)->operation = (yyvsp[(2) - (3)].cond_op);
           }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 698 "mon_parse.y"
    { return ERR_INCOMPLETE_COMPARE_OP; }
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 700 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(2) - (3)].cond_node); (yyval.cond_node)->is_parenthized = TRUE; }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 702 "mon_parse.y"
    { return ERR_MISSING_CLOSE_PAREN; }
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 704 "mon_parse.y"
    { (yyval.cond_node) = (yyvsp[(1) - (1)].cond_node); }
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 707 "mon_parse.y"
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->reg_num = (yyvsp[(1) - (1)].i); (yyval.cond_node)->is_reg = TRUE;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 713 "mon_parse.y"
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->value = (yyvsp[(1) - (1)].i); (yyval.cond_node)->is_reg = FALSE;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 725 "mon_parse.y"
    { mon_add_number_to_buffer((yyvsp[(1) - (1)].i)); }
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 726 "mon_parse.y"
    { mon_add_string_to_buffer((yyvsp[(1) - (1)].str)); }
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 733 "mon_parse.y"
    { mon_add_number_to_buffer((yyvsp[(1) - (1)].i)); }
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 734 "mon_parse.y"
    { mon_add_number_masked_to_buffer((yyvsp[(1) - (1)].i), 0x00); }
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 735 "mon_parse.y"
    { mon_add_string_to_buffer((yyvsp[(1) - (1)].str)); }
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 738 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 739 "mon_parse.y"
    { (yyval.i) = (monitor_cpu_for_memspace[reg_memspace((yyvsp[(1) - (1)].i))]->mon_register_get_val)(reg_memspace((yyvsp[(1) - (1)].i)), reg_regid((yyvsp[(1) - (1)].i))); }
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 742 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 743 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 744 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 745 "mon_parse.y"
    { (yyval.i) = strtol((yyvsp[(1) - (1)].str), NULL, 10); }
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 748 "mon_parse.y"
    { (yyval.i) = resolve_datatype(B_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 749 "mon_parse.y"
    { (yyval.i) = resolve_datatype(O_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 750 "mon_parse.y"
    { (yyval.i) = resolve_datatype(D_NUMBER,(yyvsp[(1) - (1)].str)); }
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 753 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 754 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 755 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 756 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 237:

/* Line 1806 of yacc.c  */
#line 757 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 765 "mon_parse.y"
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

  case 243:

/* Line 1806 of yacc.c  */
#line 776 "mon_parse.y"
    { asm_mode = 0; }
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 779 "mon_parse.y"
    { if ((yyvsp[(2) - (2)].i) > 0xff) {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE_16;
                          (yyval.mode).param = (yyvsp[(2) - (2)].i);
                        } else {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE;
                          (yyval.mode).param = (yyvsp[(2) - (2)].i);
                        } }
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 786 "mon_parse.y"
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

  case 246:

/* Line 1806 of yacc.c  */
#line 797 "mon_parse.y"
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

  case 247:

/* Line 1806 of yacc.c  */
#line 808 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE_Y;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          } else {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_Y;
                            (yyval.mode).param = (yyvsp[(1) - (3)].i);
                          }
                        }
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 816 "mon_parse.y"
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

  case 249:

/* Line 1806 of yacc.c  */
#line 835 "mon_parse.y"
    { if ((yyvsp[(1) - (3)].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_DOUBLE;
                            (yyval.mode).param = (yyvsp[(3) - (3)].i);
                            (yyval.mode).addr_submode = (yyvsp[(1) - (3)].i);
                          }
                        }
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 841 "mon_parse.y"
    { if ((yyvsp[(2) - (3)].i) < 0x100) {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT;
                               (yyval.mode).param = (yyvsp[(2) - (3)].i);
                             } else {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT;
                               (yyval.mode).param = (yyvsp[(2) - (3)].i);
                             }
                           }
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 849 "mon_parse.y"
    { if ((yyvsp[(2) - (5)].i) < 0x100) {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[(2) - (5)].i);
                                         } else {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[(2) - (5)].i);
                                         }
                                       }
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 858 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_STACK_RELATIVE_Y; (yyval.mode).param = (yyvsp[(2) - (7)].i); }
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 860 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_Y; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 254:

/* Line 1806 of yacc.c  */
#line 861 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_BC; }
    break;

  case 255:

/* Line 1806 of yacc.c  */
#line 862 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_DE; }
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 863 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_HL; }
    break;

  case 257:

/* Line 1806 of yacc.c  */
#line 864 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IX; }
    break;

  case 258:

/* Line 1806 of yacc.c  */
#line 865 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IY; }
    break;

  case 259:

/* Line 1806 of yacc.c  */
#line 866 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_SP; }
    break;

  case 260:

/* Line 1806 of yacc.c  */
#line 868 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_A; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 261:

/* Line 1806 of yacc.c  */
#line 870 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_HL; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 262:

/* Line 1806 of yacc.c  */
#line 872 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IX; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 874 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IY; (yyval.mode).param = (yyvsp[(2) - (5)].i); }
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 875 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_IMPLIED; }
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 876 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ACCUMULATOR; }
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 877 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_B; }
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 878 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_C; }
    break;

  case 268:

/* Line 1806 of yacc.c  */
#line 879 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_D; }
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 880 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_E; }
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 881 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_H; }
    break;

  case 271:

/* Line 1806 of yacc.c  */
#line 882 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXH; }
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 883 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYH; }
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 884 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_L; }
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 885 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXL; }
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 886 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYL; }
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 887 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_AF; }
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 888 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_BC; }
    break;

  case 278:

/* Line 1806 of yacc.c  */
#line 889 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_DE; }
    break;

  case 279:

/* Line 1806 of yacc.c  */
#line 890 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_HL; }
    break;

  case 280:

/* Line 1806 of yacc.c  */
#line 891 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IX; }
    break;

  case 281:

/* Line 1806 of yacc.c  */
#line 892 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IY; }
    break;

  case 282:

/* Line 1806 of yacc.c  */
#line 893 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_SP; }
    break;

  case 283:

/* Line 1806 of yacc.c  */
#line 895 "mon_parse.y"
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_DIRECT; (yyval.mode).param = (yyvsp[(2) - (2)].i); }
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 896 "mon_parse.y"
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

  case 285:

/* Line 1806 of yacc.c  */
#line 911 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 915 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (4)].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
    break;

  case 287:

/* Line 1806 of yacc.c  */
#line 919 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (3)].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
    break;

  case 288:

/* Line 1806 of yacc.c  */
#line 923 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(4) - (4)].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
    break;

  case 289:

/* Line 1806 of yacc.c  */
#line 927 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (2)].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
    break;

  case 290:

/* Line 1806 of yacc.c  */
#line 931 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 935 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 939 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(2) - (3)].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 943 "mon_parse.y"
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

  case 294:

/* Line 1806 of yacc.c  */
#line 955 "mon_parse.y"
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

  case 295:

/* Line 1806 of yacc.c  */
#line 970 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
    break;

  case 296:

/* Line 1806 of yacc.c  */
#line 974 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (6)].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 978 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(4) - (5)].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 982 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(5) - (6)].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 986 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (4)].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 990 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 994 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
    break;

  case 302:

/* Line 1806 of yacc.c  */
#line 998 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[(3) - (5)].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 1002 "mon_parse.y"
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

  case 304:

/* Line 1806 of yacc.c  */
#line 1014 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | ASM_ADDR_MODE_EXTENDED_INDIRECT;
        (yyval.mode).param = (yyvsp[(2) - (3)].i);
        }
    break;

  case 305:

/* Line 1806 of yacc.c  */
#line 1019 "mon_parse.y"
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_LONG_Y;
        (yyval.mode).param = (yyvsp[(2) - (5)].i);
        }
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 1027 "mon_parse.y"
    { (yyval.i) = (0 << 5); printf("reg_x\n"); }
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 1028 "mon_parse.y"
    { (yyval.i) = (1 << 5); printf("reg_y\n"); }
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 1029 "mon_parse.y"
    { (yyval.i) = (yyvsp[(1) - (1)].i); }
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 1030 "mon_parse.y"
    { (yyval.i) = (3 << 5); printf("reg_s\n"); }
    break;

  case 310:

/* Line 1806 of yacc.c  */
#line 1034 "mon_parse.y"
    { (yyval.i) = (2 << 5); printf("reg_u\n"); }
    break;



/* Line 1806 of yacc.c  */
#line 4838 "mon_parse.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
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
      if (!yypact_value_is_default (yyn))
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
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
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



/* Line 2067 of yacc.c  */
#line 1038 "mon_parse.y"


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
         case ERR_INVALID_REGISTER:
           mon_out("Invalid register.\n");
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



