/* A Bison parser, made by GNU Bison 3.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "mon_parse.y" /* yacc.c:339  */

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

#if !defined(MACOS_COMPILE) && !(defined(__OS2__) && defined(IDE_COMPILE))
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
#endif /* MACOS OS2 */

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
#include "mon_memmap.h"
#include "mon_memory.h"
#include "mon_register.h"
#include "mon_util.h"
#include "montypes.h"
#include "resources.h"
#include "types.h"
#include "uimon.h"

#ifdef AMIGA_MORPHOS
#undef REG_PC
#endif

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
#define ERR_INCOMPLETE_COND_OP 8
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


#line 200 "mon_parse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_MON_PARSE_H_INCLUDED
# define YY_YY_MON_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
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
    CMD_LOG = 280,
    CMD_LOGNAME = 281,
    CMD_SIDEFX = 282,
    CMD_RETURN = 283,
    CMD_BLOCK_READ = 284,
    CMD_BLOCK_WRITE = 285,
    CMD_UP = 286,
    CMD_DOWN = 287,
    CMD_LOAD = 288,
    CMD_SAVE = 289,
    CMD_VERIFY = 290,
    CMD_IGNORE = 291,
    CMD_HUNT = 292,
    CMD_FILL = 293,
    CMD_MOVE = 294,
    CMD_GOTO = 295,
    CMD_REGISTERS = 296,
    CMD_READSPACE = 297,
    CMD_WRITESPACE = 298,
    CMD_RADIX = 299,
    CMD_MEM_DISPLAY = 300,
    CMD_BREAK = 301,
    CMD_TRACE = 302,
    CMD_IO = 303,
    CMD_BRMON = 304,
    CMD_COMPARE = 305,
    CMD_DUMP = 306,
    CMD_UNDUMP = 307,
    CMD_EXIT = 308,
    CMD_DELETE = 309,
    CMD_CONDITION = 310,
    CMD_COMMAND = 311,
    CMD_ASSEMBLE = 312,
    CMD_DISASSEMBLE = 313,
    CMD_NEXT = 314,
    CMD_STEP = 315,
    CMD_PRINT = 316,
    CMD_DEVICE = 317,
    CMD_HELP = 318,
    CMD_WATCH = 319,
    CMD_DISK = 320,
    CMD_QUIT = 321,
    CMD_CHDIR = 322,
    CMD_BANK = 323,
    CMD_LOAD_LABELS = 324,
    CMD_SAVE_LABELS = 325,
    CMD_ADD_LABEL = 326,
    CMD_DEL_LABEL = 327,
    CMD_SHOW_LABELS = 328,
    CMD_CLEAR_LABELS = 329,
    CMD_RECORD = 330,
    CMD_MON_STOP = 331,
    CMD_PLAYBACK = 332,
    CMD_CHAR_DISPLAY = 333,
    CMD_SPRITE_DISPLAY = 334,
    CMD_TEXT_DISPLAY = 335,
    CMD_SCREENCODE_DISPLAY = 336,
    CMD_ENTER_DATA = 337,
    CMD_ENTER_BIN_DATA = 338,
    CMD_KEYBUF = 339,
    CMD_BLOAD = 340,
    CMD_BSAVE = 341,
    CMD_SCREEN = 342,
    CMD_UNTIL = 343,
    CMD_CPU = 344,
    CMD_YYDEBUG = 345,
    CMD_BACKTRACE = 346,
    CMD_SCREENSHOT = 347,
    CMD_PWD = 348,
    CMD_DIR = 349,
    CMD_RESOURCE_GET = 350,
    CMD_RESOURCE_SET = 351,
    CMD_LOAD_RESOURCES = 352,
    CMD_SAVE_RESOURCES = 353,
    CMD_ATTACH = 354,
    CMD_DETACH = 355,
    CMD_MON_RESET = 356,
    CMD_TAPECTRL = 357,
    CMD_CARTFREEZE = 358,
    CMD_CPUHISTORY = 359,
    CMD_MEMMAPZAP = 360,
    CMD_MEMMAPSHOW = 361,
    CMD_MEMMAPSAVE = 362,
    CMD_COMMENT = 363,
    CMD_LIST = 364,
    CMD_STOPWATCH = 365,
    RESET = 366,
    CMD_EXPORT = 367,
    CMD_AUTOSTART = 368,
    CMD_AUTOLOAD = 369,
    CMD_MAINCPU_TRACE = 370,
    CMD_LABEL_ASGN = 371,
    L_PAREN = 372,
    R_PAREN = 373,
    ARG_IMMEDIATE = 374,
    REG_A = 375,
    REG_X = 376,
    REG_Y = 377,
    COMMA = 378,
    INST_SEP = 379,
    L_BRACKET = 380,
    R_BRACKET = 381,
    LESS_THAN = 382,
    REG_U = 383,
    REG_S = 384,
    REG_PC = 385,
    REG_PCR = 386,
    REG_B = 387,
    REG_C = 388,
    REG_D = 389,
    REG_E = 390,
    REG_H = 391,
    REG_L = 392,
    REG_AF = 393,
    REG_BC = 394,
    REG_DE = 395,
    REG_HL = 396,
    REG_IX = 397,
    REG_IY = 398,
    REG_SP = 399,
    REG_IXH = 400,
    REG_IXL = 401,
    REG_IYH = 402,
    REG_IYL = 403,
    PLUS = 404,
    MINUS = 405,
    STRING = 406,
    FILENAME = 407,
    R_O_L = 408,
    OPCODE = 409,
    LABEL = 410,
    BANKNAME = 411,
    CPUTYPE = 412,
    MON_REGISTER = 413,
    COND_OP = 414,
    RADIX_TYPE = 415,
    INPUT_SPEC = 416,
    CMD_CHECKPT_ON = 417,
    CMD_CHECKPT_OFF = 418,
    TOGGLE = 419,
    MASK = 420
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
#define CMD_LOG 280
#define CMD_LOGNAME 281
#define CMD_SIDEFX 282
#define CMD_RETURN 283
#define CMD_BLOCK_READ 284
#define CMD_BLOCK_WRITE 285
#define CMD_UP 286
#define CMD_DOWN 287
#define CMD_LOAD 288
#define CMD_SAVE 289
#define CMD_VERIFY 290
#define CMD_IGNORE 291
#define CMD_HUNT 292
#define CMD_FILL 293
#define CMD_MOVE 294
#define CMD_GOTO 295
#define CMD_REGISTERS 296
#define CMD_READSPACE 297
#define CMD_WRITESPACE 298
#define CMD_RADIX 299
#define CMD_MEM_DISPLAY 300
#define CMD_BREAK 301
#define CMD_TRACE 302
#define CMD_IO 303
#define CMD_BRMON 304
#define CMD_COMPARE 305
#define CMD_DUMP 306
#define CMD_UNDUMP 307
#define CMD_EXIT 308
#define CMD_DELETE 309
#define CMD_CONDITION 310
#define CMD_COMMAND 311
#define CMD_ASSEMBLE 312
#define CMD_DISASSEMBLE 313
#define CMD_NEXT 314
#define CMD_STEP 315
#define CMD_PRINT 316
#define CMD_DEVICE 317
#define CMD_HELP 318
#define CMD_WATCH 319
#define CMD_DISK 320
#define CMD_QUIT 321
#define CMD_CHDIR 322
#define CMD_BANK 323
#define CMD_LOAD_LABELS 324
#define CMD_SAVE_LABELS 325
#define CMD_ADD_LABEL 326
#define CMD_DEL_LABEL 327
#define CMD_SHOW_LABELS 328
#define CMD_CLEAR_LABELS 329
#define CMD_RECORD 330
#define CMD_MON_STOP 331
#define CMD_PLAYBACK 332
#define CMD_CHAR_DISPLAY 333
#define CMD_SPRITE_DISPLAY 334
#define CMD_TEXT_DISPLAY 335
#define CMD_SCREENCODE_DISPLAY 336
#define CMD_ENTER_DATA 337
#define CMD_ENTER_BIN_DATA 338
#define CMD_KEYBUF 339
#define CMD_BLOAD 340
#define CMD_BSAVE 341
#define CMD_SCREEN 342
#define CMD_UNTIL 343
#define CMD_CPU 344
#define CMD_YYDEBUG 345
#define CMD_BACKTRACE 346
#define CMD_SCREENSHOT 347
#define CMD_PWD 348
#define CMD_DIR 349
#define CMD_RESOURCE_GET 350
#define CMD_RESOURCE_SET 351
#define CMD_LOAD_RESOURCES 352
#define CMD_SAVE_RESOURCES 353
#define CMD_ATTACH 354
#define CMD_DETACH 355
#define CMD_MON_RESET 356
#define CMD_TAPECTRL 357
#define CMD_CARTFREEZE 358
#define CMD_CPUHISTORY 359
#define CMD_MEMMAPZAP 360
#define CMD_MEMMAPSHOW 361
#define CMD_MEMMAPSAVE 362
#define CMD_COMMENT 363
#define CMD_LIST 364
#define CMD_STOPWATCH 365
#define RESET 366
#define CMD_EXPORT 367
#define CMD_AUTOSTART 368
#define CMD_AUTOLOAD 369
#define CMD_MAINCPU_TRACE 370
#define CMD_LABEL_ASGN 371
#define L_PAREN 372
#define R_PAREN 373
#define ARG_IMMEDIATE 374
#define REG_A 375
#define REG_X 376
#define REG_Y 377
#define COMMA 378
#define INST_SEP 379
#define L_BRACKET 380
#define R_BRACKET 381
#define LESS_THAN 382
#define REG_U 383
#define REG_S 384
#define REG_PC 385
#define REG_PCR 386
#define REG_B 387
#define REG_C 388
#define REG_D 389
#define REG_E 390
#define REG_H 391
#define REG_L 392
#define REG_AF 393
#define REG_BC 394
#define REG_DE 395
#define REG_HL 396
#define REG_IX 397
#define REG_IY 398
#define REG_SP 399
#define REG_IXH 400
#define REG_IXL 401
#define REG_IYH 402
#define REG_IYL 403
#define PLUS 404
#define MINUS 405
#define STRING 406
#define FILENAME 407
#define R_O_L 408
#define OPCODE 409
#define LABEL 410
#define BANKNAME 411
#define CPUTYPE 412
#define MON_REGISTER 413
#define COND_OP 414
#define RADIX_TYPE 415
#define INPUT_SPEC 416
#define CMD_CHECKPT_ON 417
#define CMD_CHECKPT_OFF 418
#define TOGGLE 419
#define MASK 420

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 135 "mon_parse.y" /* yacc.c:355  */

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

#line 583 "mon_parse.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MON_PARSE_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 600 "mon_parse.c" /* yacc.c:358  */

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
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
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
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  317
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1741

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  174
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  55
/* YYNRULES -- Number of rules.  */
#define YYNRULES  317
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  635

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   420

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     170,   171,   168,   166,     2,   167,     2,   169,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   173,     2,
       2,     2,     2,     2,   172,     2,     2,     2,     2,     2,
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
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   199,   199,   200,   201,   204,   205,   208,   209,   210,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   228,   230,   232,   234,   236,   238,   240,
     242,   244,   246,   248,   250,   252,   254,   256,   258,   260,
     262,   264,   266,   268,   270,   272,   274,   276,   279,   281,
     283,   286,   291,   296,   298,   300,   302,   304,   306,   308,
     310,   312,   314,   318,   325,   324,   327,   329,   331,   335,
     337,   339,   341,   343,   345,   347,   349,   351,   353,   355,
     357,   359,   361,   363,   365,   367,   369,   371,   373,   377,
     386,   389,   393,   396,   405,   408,   417,   422,   424,   426,
     428,   430,   432,   434,   436,   438,   440,   442,   446,   448,
     453,   460,   472,   476,   478,   496,   498,   500,   502,   504,
     508,   510,   512,   514,   516,   518,   520,   522,   524,   526,
     528,   530,   532,   534,   536,   538,   540,   542,   544,   546,
     548,   550,   552,   556,   558,   560,   562,   564,   566,   568,
     570,   572,   574,   576,   578,   580,   582,   584,   586,   588,
     590,   592,   596,   598,   600,   604,   606,   610,   614,   617,
     618,   621,   622,   625,   626,   629,   630,   633,   634,   637,
     643,   651,   652,   655,   659,   660,   663,   664,   667,   668,
     670,   674,   675,   678,   683,   688,   698,   699,   702,   703,
     704,   705,   706,   709,   711,   713,   714,   715,   716,   717,
     718,   719,   722,   723,   725,   730,   732,   734,   736,   740,
     746,   752,   760,   761,   764,   765,   768,   769,   772,   773,
     774,   777,   778,   781,   782,   783,   784,   787,   788,   789,
     792,   793,   794,   795,   796,   799,   800,   801,   804,   814,
     815,   818,   825,   836,   847,   855,   874,   880,   888,   896,
     898,   900,   901,   902,   903,   904,   905,   906,   908,   910,
     912,   914,   915,   916,   917,   918,   919,   920,   921,   922,
     923,   924,   925,   926,   927,   928,   929,   930,   931,   932,
     934,   935,   950,   954,   958,   962,   966,   970,   974,   978,
     982,   994,  1009,  1013,  1017,  1021,  1025,  1029,  1033,  1037,
    1041,  1053,  1058,  1066,  1067,  1068,  1069,  1073
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "H_NUMBER", "D_NUMBER", "O_NUMBER",
  "B_NUMBER", "CONVERT_OP", "B_DATA", "H_RANGE_GUESS", "D_NUMBER_GUESS",
  "O_NUMBER_GUESS", "B_NUMBER_GUESS", "BAD_CMD", "MEM_OP", "IF",
  "MEM_COMP", "MEM_DISK8", "MEM_DISK9", "MEM_DISK10", "MEM_DISK11",
  "EQUALS", "TRAIL", "CMD_SEP", "LABEL_ASGN_COMMENT", "CMD_LOG",
  "CMD_LOGNAME", "CMD_SIDEFX", "CMD_RETURN", "CMD_BLOCK_READ",
  "CMD_BLOCK_WRITE", "CMD_UP", "CMD_DOWN", "CMD_LOAD", "CMD_SAVE",
  "CMD_VERIFY", "CMD_IGNORE", "CMD_HUNT", "CMD_FILL", "CMD_MOVE",
  "CMD_GOTO", "CMD_REGISTERS", "CMD_READSPACE", "CMD_WRITESPACE",
  "CMD_RADIX", "CMD_MEM_DISPLAY", "CMD_BREAK", "CMD_TRACE", "CMD_IO",
  "CMD_BRMON", "CMD_COMPARE", "CMD_DUMP", "CMD_UNDUMP", "CMD_EXIT",
  "CMD_DELETE", "CMD_CONDITION", "CMD_COMMAND", "CMD_ASSEMBLE",
  "CMD_DISASSEMBLE", "CMD_NEXT", "CMD_STEP", "CMD_PRINT", "CMD_DEVICE",
  "CMD_HELP", "CMD_WATCH", "CMD_DISK", "CMD_QUIT", "CMD_CHDIR", "CMD_BANK",
  "CMD_LOAD_LABELS", "CMD_SAVE_LABELS", "CMD_ADD_LABEL", "CMD_DEL_LABEL",
  "CMD_SHOW_LABELS", "CMD_CLEAR_LABELS", "CMD_RECORD", "CMD_MON_STOP",
  "CMD_PLAYBACK", "CMD_CHAR_DISPLAY", "CMD_SPRITE_DISPLAY",
  "CMD_TEXT_DISPLAY", "CMD_SCREENCODE_DISPLAY", "CMD_ENTER_DATA",
  "CMD_ENTER_BIN_DATA", "CMD_KEYBUF", "CMD_BLOAD", "CMD_BSAVE",
  "CMD_SCREEN", "CMD_UNTIL", "CMD_CPU", "CMD_YYDEBUG", "CMD_BACKTRACE",
  "CMD_SCREENSHOT", "CMD_PWD", "CMD_DIR", "CMD_RESOURCE_GET",
  "CMD_RESOURCE_SET", "CMD_LOAD_RESOURCES", "CMD_SAVE_RESOURCES",
  "CMD_ATTACH", "CMD_DETACH", "CMD_MON_RESET", "CMD_TAPECTRL",
  "CMD_CARTFREEZE", "CMD_CPUHISTORY", "CMD_MEMMAPZAP", "CMD_MEMMAPSHOW",
  "CMD_MEMMAPSAVE", "CMD_COMMENT", "CMD_LIST", "CMD_STOPWATCH", "RESET",
  "CMD_EXPORT", "CMD_AUTOSTART", "CMD_AUTOLOAD", "CMD_MAINCPU_TRACE",
  "CMD_LABEL_ASGN", "L_PAREN", "R_PAREN", "ARG_IMMEDIATE", "REG_A",
  "REG_X", "REG_Y", "COMMA", "INST_SEP", "L_BRACKET", "R_BRACKET",
  "LESS_THAN", "REG_U", "REG_S", "REG_PC", "REG_PCR", "REG_B", "REG_C",
  "REG_D", "REG_E", "REG_H", "REG_L", "REG_AF", "REG_BC", "REG_DE",
  "REG_HL", "REG_IX", "REG_IY", "REG_SP", "REG_IXH", "REG_IXL", "REG_IYH",
  "REG_IYL", "PLUS", "MINUS", "STRING", "FILENAME", "R_O_L", "OPCODE",
  "LABEL", "BANKNAME", "CPUTYPE", "MON_REGISTER", "COND_OP", "RADIX_TYPE",
  "INPUT_SPEC", "CMD_CHECKPT_ON", "CMD_CHECKPT_OFF", "TOGGLE", "MASK",
  "'+'", "'-'", "'*'", "'/'", "'('", "')'", "'@'", "':'", "$accept",
  "top_level", "command_list", "end_cmd", "command", "machine_state_rules",
  "register_mod", "symbol_table_rules", "asm_rules", "$@1", "memory_rules",
  "checkpoint_rules", "checkpoint_control_rules", "monitor_state_rules",
  "monitor_misc_rules", "disk_rules", "cmd_file_rules", "data_entry_rules",
  "monitor_debug_rules", "rest_of_line", "opt_rest_of_line", "filename",
  "device_num", "mem_op", "opt_mem_op", "register", "reg_list", "reg_asgn",
  "checkpt_num", "address_opt_range", "address_range", "opt_address",
  "address", "opt_sep", "memspace", "memloc", "memaddr", "expression",
  "opt_if_cond_expr", "cond_expr", "cond_operand", "data_list",
  "data_element", "hunt_list", "hunt_element", "value", "d_number",
  "guess_default", "number", "assembly_instr_list", "assembly_instruction",
  "post_assemble", "asm_operand_mode", "index_reg", "index_ureg", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
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
     415,   416,   417,   418,   419,   420,    43,    45,    42,    47,
      40,    41,    64,    58
};
# endif

#define YYPACT_NINF -361

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-361)))

#define YYTABLE_NINF -198

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1130,   862,  -361,  -361,     4,    24,    53,   301,   862,   862,
     147,   147,    24,    24,    24,   804,  1490,  1490,  1490,  1386,
     476,    56,   994,  1104,  1104,  1386,  1490,    24,    24,   301,
     736,   804,   804,  1508,  1246,   147,   147,   862,   643,   174,
    1104,  -142,   301,  -142,   936,   424,   424,  1508,   516,   711,
     711,    24,   301,    24,  1246,  1246,  1246,  1246,  1508,   301,
    -142,    24,    24,   301,  1246,   112,   301,   301,    24,   301,
    -140,  -116,   -80,    24,    24,    24,   862,   147,   -31,   301,
     147,   301,   147,    24,  -140,   449,   277,   301,    24,    24,
     -62,    88,  1549,   736,   736,   107,  1269,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,   119,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,   862,  -361,    -9,    96,  -361,
    -361,  -361,  -361,  -361,  -361,   301,  -361,  -361,  -361,   301,
     301,  -361,  -361,   929,   929,  -361,  -361,   862,  -361,   862,
     386,   508,   386,  -361,  -361,  -361,  -361,  -361,   147,  -361,
    -361,  -361,   -31,   -31,   -31,  -361,  -361,  -361,   -31,   -31,
    -361,   301,   -31,  -361,   152,   233,  -361,    99,   301,  -361,
     -31,  -361,   301,  -361,   217,  -361,  -361,   163,  1490,  -361,
    1490,  -361,   301,   -31,   301,   301,  -361,   348,  -361,   301,
     164,    57,    58,  -361,   301,  -361,   862,  -361,   862,    96,
     301,  -361,  -361,   301,  -361,  1490,   301,  -361,   301,   301,
    -361,   160,   301,   -31,   301,   -31,   -31,   301,   -31,  -361,
     301,  -361,   301,   301,  -361,   301,  -361,   301,  -361,   301,
    -361,   301,  -361,   301,  1129,  -361,   301,   386,   386,  -361,
    -361,   301,   301,  -361,  -361,  -361,   147,  -361,  -361,   301,
     301,    35,   301,   301,   862,    96,  -361,   862,   862,  -361,
    -361,   862,  -361,  -361,   862,   -31,   301,   361,  -361,   301,
     140,   301,  -361,  -361,  1618,  1618,   301,  1508,  1562,  1267,
      70,   175,  1587,  1267,    77,  -361,    84,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,   101,  -361,  -361,   301,  -361,   301,  -361,  -361,  -361,
      54,  -361,   862,   862,   862,   862,  -361,  -361,  -361,  -361,
      92,   269,    96,    96,  -361,   244,  1409,  1432,  1472,  -361,
     862,   699,  1508,  1572,  1129,  1508,  -361,  1267,  1267,   328,
    -361,  -361,  -361,  1490,  -361,  -361,   194,   194,  -361,  1508,
    -361,  -361,  -361,   535,   301,    75,  -361,    87,  -361,    96,
      96,  -361,  -361,  -361,   194,  -361,  -361,  -361,  -361,    90,
    -361,    24,  -361,    24,    76,  -361,    89,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  1528,  -361,  -361,  -361,
     244,  1452,  -361,  -361,  -361,   862,  -361,  -361,   301,  -361,
    -361,    96,  -361,    96,    96,    96,   664,   862,  -361,  -361,
    -361,  -361,  1267,  -361,  1267,  -361,   478,   130,   135,   150,
     158,   159,   172,   -27,  -361,   210,  -361,  -361,  -361,  -361,
     335,   146,  -361,   127,   529,   170,   189,    79,  -361,   210,
     210,  1605,  -361,  -361,  -361,  -361,    23,    23,  -361,  -361,
     301,  1508,   301,  -361,  -361,   301,  -361,   301,  -361,   301,
      96,  -361,  -361,   904,  -361,  -361,  -361,  -361,  -361,  1528,
     301,  -361,  -361,   301,   535,   301,   301,   301,   535,   157,
    -361,   132,  -361,  -361,  -361,   301,   204,   212,   301,  -361,
    -361,   301,   301,   301,   301,   301,   301,  -361,  1129,   301,
    -361,   301,    96,  -361,  -361,  -361,  -361,  -361,  -361,   301,
      96,   301,   301,   301,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,   219,    18,  -361,   210,  -361,   202,   210,   570,   -63,
     210,   210,   353,   229,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,   200,  -361,  -361,  -361,    22,   188,
     126,  -361,  -361,    87,    87,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,
    -361,   273,   245,   250,  -361,  -361,   247,   210,   251,  -361,
     -60,   254,   268,   275,   281,   290,  -361,  -361,  1508,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,   253,
    -361,   287,  -361,  -361,   291,  -361,  -361,  -361,  -361,  -361,
    -361,   297,  -361,  -361,  -361
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,    22,     4,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     170,     0,     0,     0,     0,     0,     0,     0,   197,     0,
       0,     0,     0,     0,   170,     0,     0,     0,     0,     0,
       0,     0,   271,     0,     0,     0,     2,     5,    10,    47,
      11,    13,    12,    14,    15,    16,    17,    18,    19,    20,
      21,     0,   240,   241,   242,   243,   239,   238,   237,   198,
     199,   200,   201,   202,   179,     0,   232,     0,     0,   211,
     244,   231,     9,     8,     7,     0,   111,   172,   171,     0,
       0,   109,    35,     0,     0,   196,    42,     0,    44,     0,
       0,     0,     0,   185,   233,   236,   235,   234,     0,   184,
     189,   195,   197,   197,   197,   193,   203,   204,   197,   197,
      28,     0,   197,    48,     0,     0,   182,     0,     0,   114,
     197,    75,     0,   186,   197,   176,    90,   177,     0,    96,
       0,    29,     0,   197,     0,     0,   118,     9,   104,     0,
       0,     0,     0,    68,     0,    40,     0,    38,     0,     0,
       0,   168,   122,     0,    94,     0,     0,   117,     0,     0,
      23,     0,     0,   197,     0,   197,   197,     0,   197,    59,
       0,    61,     0,     0,   163,     0,    77,     0,    79,     0,
      81,     0,    83,     0,     0,   166,     0,     0,     0,    46,
      92,     0,     0,    31,   167,   127,     0,   129,   169,     0,
       0,     0,     0,     0,     0,     0,   136,     0,     0,   139,
      33,     0,    84,    85,     0,   197,     0,     9,   154,     0,
     173,     0,   142,   116,     0,     0,     0,     0,     0,     0,
     272,     0,     0,     0,   273,   274,   275,   276,   277,   280,
     283,   284,   285,   286,   287,   288,   289,   278,   281,   279,
     282,   252,   248,    98,     0,   100,     0,     1,     6,     3,
       0,   180,     0,     0,     0,     0,   124,   110,   112,   108,
     197,     0,     0,     0,   174,   197,   146,     0,     0,   101,
       0,     0,     0,     0,     0,     0,    27,     0,     0,     0,
      50,    49,   113,     0,    74,   175,   213,   213,    30,     0,
      36,    37,   103,     0,     0,     0,    66,     0,    67,     0,
       0,   121,   115,   123,   213,   120,   125,    25,    24,     0,
      52,     0,    54,     0,     0,    56,     0,    58,    60,   162,
     164,    76,    78,    80,    82,   225,     0,   223,   224,   126,
     197,     0,    91,    32,   130,     0,   128,   132,     0,   134,
     135,     0,   157,     0,     0,     0,     0,     0,   140,   155,
     141,   158,     0,   160,     0,   119,     0,     0,     0,     0,
       0,     0,     0,     0,   251,     0,   313,   314,   317,   316,
       0,   296,   315,     0,     0,     0,     0,     0,   290,     0,
       0,     0,    97,    99,   210,   209,   205,   206,   207,   208,
       0,     0,     0,    43,    45,     0,   147,     0,   151,     0,
       0,   230,   229,     0,   227,   228,   188,   190,   194,     0,
       0,   183,   181,     0,     0,     0,     0,     0,     0,     0,
     219,     0,   218,   220,   107,     0,   250,   249,     0,    41,
      39,     0,     0,     0,     0,     0,     0,   165,     0,     0,
     149,     0,     0,   133,   156,   137,   138,    34,    86,     0,
       0,     0,     0,     0,    62,   261,   262,   263,   264,   265,
     266,   257,     0,   298,     0,   294,   292,     0,     0,     0,
       0,     0,     0,   311,   297,   299,   253,   254,   255,   300,
     256,   291,   152,   191,   153,   143,   145,   150,   102,    72,
     226,    71,    69,    73,   212,    89,    95,    70,     0,     0,
       0,   105,   106,     0,   247,    65,    93,    26,    51,    53,
      55,    57,   222,   144,   148,   131,    87,    88,   159,   161,
      63,     0,     0,     0,   295,   293,     0,     0,     0,   306,
       0,     0,     0,     0,     0,     0,   217,   216,     0,   215,
     214,   245,   246,   267,   260,   268,   269,   270,   258,     0,
     308,     0,   304,   302,     0,   307,   309,   310,   301,   312,
     221,     0,   305,   303,   259
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -361,  -361,  -361,   561,   325,  -361,  -361,  -361,  -361,  -361,
    -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,  -361,   144,
     339,    37,  -131,  -361,    32,    -8,  -361,    82,   288,    13,
      -2,  -327,    27,    26,   -16,  -219,  -361,   714,  -273,  -328,
    -361,    95,   -76,  -361,   -40,  -361,  -361,  -361,    86,  -361,
    -360,  -361,  -361,   452,    -5
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    95,    96,   136,    97,    98,    99,   100,   101,   367,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   213,
     259,   139,   279,   187,   188,   126,   175,   176,   158,   182,
     183,   460,   184,   461,   127,   165,   166,   280,   485,   491,
     492,   396,   397,   473,   474,   129,   159,   130,   167,   496,
     111,   498,   312,   441,   442
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     164,   164,   164,   172,   177,   132,   164,   497,   465,   172,
     164,   211,   174,   258,   162,   168,   169,   172,   164,   335,
     337,   338,   210,   606,   193,   137,   133,   134,   221,   223,
     225,   172,   228,   230,   232,   260,   147,   149,   164,   164,
     164,   164,   172,   163,   163,   163,   171,   204,   164,   150,
     151,   152,   192,   163,   132,   454,   190,   132,   364,   132,
     202,   206,   208,   599,   194,   195,   623,   237,   239,   241,
     243,   261,   215,   509,   226,   133,   134,   251,   133,   134,
     133,   134,   222,   224,   486,   244,   600,   131,   233,   624,
     235,   531,   145,  -192,   131,   131,   532,   132,   247,   248,
     132,   501,   286,   267,   268,   256,   271,   317,   274,   287,
     262,   263,   264,   132,  -192,  -192,   400,   401,   133,   134,
     275,   133,   134,   131,   478,   284,   285,   609,   478,   112,
     113,   114,   115,   132,   133,   134,   116,   117,   118,   592,
     607,   319,   119,   120,   121,   122,   123,   593,   132,   321,
    -197,  -197,  -197,  -197,   133,   134,   564,  -197,  -197,  -197,
     568,   132,   131,  -197,  -197,  -197,  -197,  -197,   135,   133,
     134,   131,   164,   348,   164,   132,   138,   355,   311,   363,
     145,   570,   133,   134,   340,   216,   408,   218,   341,   342,
     343,   324,   325,   435,   344,   345,   133,   134,   347,   164,
     449,   356,   542,   357,   246,   543,   353,   450,  -197,   484,
     342,   131,   -64,   611,   612,   145,   178,   140,  -187,   359,
     322,   323,   324,   325,   451,   455,   495,   365,   374,   131,
     131,   505,  -187,   131,   132,   131,   131,   131,   131,  -187,
    -187,    92,   610,   488,   506,  -192,   502,   379,   525,   381,
     537,   383,   384,   526,   386,   133,   134,   321,   322,   323,
     324,   325,   322,   323,   324,   325,  -192,  -192,   527,   252,
     145,   172,   112,   113,   114,   115,   528,   529,   132,   116,
     117,   118,   405,   145,   124,   119,   120,   121,   122,   123,
     530,   570,   131,   540,   131,   536,   436,   437,   489,   133,
     134,   417,   132,   438,   439,  -197,   322,   323,   324,   325,
     422,   424,   541,   569,   426,   172,  -197,  -197,   199,   200,
     201,   164,   172,   133,   134,   440,   172,   211,   573,   172,
     398,   436,   437,   131,   131,   467,   574,   164,   438,   439,
     145,   174,   591,   172,   119,   120,   121,   122,   123,  -185,
     131,   595,   605,   131,   131,   490,   349,   131,   462,   570,
     131,   608,  -174,   618,   163,   469,   483,   145,   619,   476,
    -185,  -185,   480,   620,   433,   434,   631,   622,   447,   448,
     625,   314,   316,  -174,  -174,   164,   487,   334,   281,   112,
     113,   114,   115,   613,   626,   614,   116,   117,   118,   511,
     164,   627,   119,   120,   121,   122,   123,   628,   131,   131,
     131,   131,   629,   632,   615,   616,   617,   633,   503,   634,
     504,   318,   508,   276,   161,   137,   131,   475,   163,   519,
     398,   482,   582,   560,   481,   322,   323,   324,   325,   479,
     119,   120,   121,   122,   123,   172,   551,     0,     0,   493,
     277,     0,   112,   113,   114,   115,   436,   437,     0,   116,
     117,   118,     0,   438,   439,   119,   120,   121,   122,   123,
       0,   133,   134,     0,   436,   437,   490,   132,     0,   132,
     490,   438,   439,   603,     0,   534,   124,     0,   553,     0,
       0,   131,   119,   120,   121,   122,   123,     0,   133,   134,
     133,   134,   523,   131,     0,   508,     0,     0,   521,   336,
     522,   112,   113,   114,   115,     0,     0,     0,   116,   117,
     118,     0,     0,     0,   119,   120,   121,   122,   123,     0,
       0,     0,   119,   120,   121,   122,   123,   550,   112,   113,
     114,   115,     0,     0,   124,   116,   117,   118,     0,     0,
       0,   119,   120,   121,   122,   123,   125,     0,     0,   475,
       0,     0,   490,     0,     0,     0,     0,   141,   142,     0,
     493,   146,   148,     0,   493,     0,   138,     0,     0,     0,
     170,   173,   179,   181,   186,   189,   191,     0,     0,     0,
     196,   198,   172,     0,   398,   203,   205,   207,     0,     0,
     212,   214,     0,   217,     0,   220,     0,   124,     0,     0,
     229,   231,     0,   234,     0,   236,   238,   240,   242,   125,
     245,     0,     0,     0,   249,   250,   253,   254,   255,     0,
     257,     0,     0,     0,   124,   630,     0,     0,   266,     0,
     269,   270,   272,   273,     0,     0,   278,   282,   283,     0,
     436,   437,   488,     0,   313,   315,   493,   438,   439,   119,
     120,   121,   122,   123,     0,   132,   124,   112,   113,   114,
     115,   227,     0,   160,   116,   117,   118,     0,   125,   538,
     119,   120,   121,   122,   123,     0,   133,   134,     0,   326,
       0,   436,   437,   124,     0,     0,   327,     0,   438,   439,
     328,   329,   112,   113,   114,   115,     0,   489,     0,   116,
     117,   118,   132,     0,     0,   128,     0,     0,     0,   339,
     597,     0,   143,   144,     0,     0,     0,   119,   120,   121,
     122,   123,   346,   133,   134,     0,   350,   197,   351,   352,
     154,     0,     0,   354,     0,     0,   155,   156,   157,     0,
       0,   209,     0,   358,     0,   360,   361,     0,   133,   134,
     362,     0,     0,   366,     0,   368,     0,     0,     0,     0,
     371,   372,     0,     0,   373,     0,     0,   375,     0,   376,
     377,     0,   378,   380,     0,   382,     0,     0,   385,     0,
     265,   387,     0,   388,   389,     0,   390,     0,   391,     0,
     392,     0,   393,     0,   394,   153,     0,   399,   154,     0,
       0,     0,   402,   403,   155,   156,   157,   404,     0,   161,
     406,   407,     0,   409,   410,     0,   412,     0,     0,     0,
     322,   323,   324,   325,     0,     0,     0,   418,     0,   320,
     419,     0,   420,     0,     0,   421,   423,   425,     0,     0,
     471,     0,     0,     0,     0,     0,     0,   330,   331,     0,
       0,   332,     0,   333,   472,   112,   113,   114,   115,     0,
       0,     0,   116,   117,   118,   452,     0,   453,   119,   120,
     121,   122,   123,     0,     0,     0,     0,   533,     0,     0,
       0,     0,   535,   463,   464,     0,   539,     0,     0,     0,
       0,   544,   545,     0,     0,   132,     0,   112,   113,   114,
     115,     0,     0,     0,   116,   117,   118,     0,     0,     0,
     369,     0,   370,     0,     0,   494,   133,   134,     0,     0,
     499,   500,   112,   113,   114,   115,     0,   132,     0,   116,
     117,   118,     0,     0,     0,   119,   120,   121,   122,   123,
       0,     0,   119,   120,   121,   122,   123,   507,   133,   134,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   513,
       0,     0,   514,     0,   515,   516,   517,   518,   411,     0,
       0,   413,   414,     0,     0,   415,   594,   524,   416,   596,
     598,     0,   601,   602,   604,   132,     0,   112,   113,   114,
     115,     0,     0,   160,   116,   117,   118,     0,     0,     0,
     119,   120,   121,   122,   123,     0,   133,   134,     0,     0,
     124,   552,     0,   554,     0,     0,   555,     0,   556,     0,
     557,   558,   125,     0,   559,     0,   456,   457,   458,   459,
     561,   562,     0,     0,   563,     0,   565,   566,   567,   621,
       0,     0,   571,     0,   470,   471,   572,     0,     0,   575,
       0,     0,   576,   577,   578,   579,   580,   581,     0,   472,
     583,     0,   584,   585,     0,     0,     0,     0,     0,     0,
     586,   587,   588,   589,   590,     0,     0,   124,     0,     0,
       0,     0,   219,     0,     0,   322,   323,   324,   325,   125,
       0,     0,     0,     0,     0,   132,     0,  -178,  -178,  -178,
    -178,     0,     0,  -178,  -178,  -178,  -178,     0,   185,   512,
    -178,  -178,  -178,  -178,  -178,     0,   133,   134,     0,     0,
       0,   520,   112,   113,   114,   115,     0,     1,     0,   116,
     117,   118,     0,     2,     0,     0,     0,     0,     0,   161,
       0,     0,     3,     0,   180,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,     0,    21,    22,    23,    24,    25,     0,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,     0,    87,    88,    89,    90,    91,   132,     0,   112,
     113,   114,   115,     0,     0,   160,   116,   117,   118,  -178,
       0,     0,   119,   120,   121,   122,   123,     0,   133,   134,
     112,   113,   114,   115,     0,     0,     1,   116,   117,   118,
     395,     0,     2,     0,    92,     0,     0,     0,     0,     0,
       0,     0,    93,    94,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,     0,    21,    22,    23,    24,    25,     0,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
       0,    87,    88,    89,    90,    91,     0,   132,     0,   112,
     113,   114,   115,     0,     0,     0,   116,   117,   118,     0,
       0,   161,   119,   120,   121,   122,   123,     0,   133,   134,
    -174,     0,  -174,  -174,  -174,  -174,     0,     0,  -174,  -174,
    -174,  -174,     0,     0,     0,  -174,  -174,  -174,  -174,  -174,
       0,    93,    94,   466,     0,   112,   113,   114,   115,     0,
       0,   160,   116,   117,   118,     0,     0,     0,   119,   120,
     121,   122,   123,   510,     0,   112,   113,   114,   115,     0,
       0,   160,   116,   117,   118,     0,     0,     0,   119,   120,
     121,   122,   123,   468,     0,   112,   113,   114,   115,     0,
       0,     0,   116,   117,   118,     0,     0,     0,   119,   120,
     121,   122,   123,   112,   113,   114,   115,     0,     0,   160,
     116,   117,   118,     0,     0,     0,   119,   120,   121,   122,
     123,   112,   113,   114,   115,     0,     0,     0,   116,   117,
     118,     0,     0,     0,   119,   120,   121,   122,   123,   132,
       0,  -197,  -197,  -197,  -197,     0,     0,     0,  -197,  -197,
    -197,   161,     0,     0,     0,     0,     0,     0,     0,     0,
     133,   134,   112,   113,   114,   115,     0,     0,     0,   116,
     117,   118,     0,     0,  -174,   112,   113,   114,   115,     0,
       0,     0,   116,   117,   118,   112,   113,   114,   115,     0,
       0,   477,   116,   117,   118,     0,     0,   161,     0,     0,
     112,   113,   114,   115,     0,     0,     0,   116,   117,   118,
       0,     0,     0,     0,     0,     0,     0,   161,   112,   113,
     114,   115,     0,     0,     0,   116,   117,   118,     0,   132,
       0,  -197,  -197,  -197,  -197,     0,     0,   161,  -197,  -197,
    -197,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     133,   134,     0,     0,     0,   161,     0,     0,     0,     0,
       0,   145,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   161,     0,     0,   288,     0,   289,   290,
       0,     0,   291,     0,   292,     0,   293,     0,     0,  -197,
       0,   294,   295,   296,   297,   298,   299,   300,   301,   302,
     303,   304,   305,   306,   307,   308,   309,   310,     0,     0,
       0,   427,   428,   429,   430,   431,   432,   443,     0,     0,
     444,     0,     0,     0,     0,     0,     0,     0,     0,   445,
       0,   446,     0,     0,     0,     0,   546,   547,     0,     0,
       0,     0,     0,   438,   548,   549,     0,     0,     0,     0,
       0,   145
};

static const yytype_int16 yycheck[] =
{
      16,    17,    18,    19,    20,     1,    22,   367,   335,    25,
      26,   153,    20,   153,    16,    17,    18,    33,    34,   150,
     151,   152,    38,     1,    26,     1,    22,    23,    44,    45,
      46,    47,    48,    49,    50,   151,    10,    11,    54,    55,
      56,    57,    58,    16,    17,    18,    19,    34,    64,    12,
      13,    14,    25,    26,     1,     1,    24,     1,     1,     1,
      33,    35,    36,   126,    27,    28,   126,    54,    55,    56,
      57,   151,    40,   400,    47,    22,    23,    64,    22,    23,
      22,    23,    45,    46,   357,    58,   149,     1,    51,   149,
      53,   118,   123,     1,     8,     9,   123,     1,    61,    62,
       1,   374,   164,    77,    78,    68,    80,     0,    82,    21,
      73,    74,    75,     1,    22,    23,   247,   248,    22,    23,
      83,    22,    23,    37,   343,    88,    89,     1,   347,     3,
       4,     5,     6,     1,    22,    23,    10,    11,    12,   121,
     118,    22,    16,    17,    18,    19,    20,   129,     1,   158,
       3,     4,     5,     6,    22,    23,   484,    10,    11,    12,
     488,     1,    76,    16,    17,    18,    19,    20,   164,    22,
      23,    85,   188,    21,   190,     1,   152,    14,    92,    15,
     123,   159,    22,    23,   158,    41,   151,    43,   162,   163,
     164,   168,   169,   123,   168,   169,    22,    23,   172,   215,
     123,   188,   123,   190,    60,   126,   180,   123,   151,    15,
     184,   125,   154,   573,   574,   123,   160,   164,     1,   193,
     166,   167,   168,   169,   123,   171,   151,   201,   215,   143,
     144,   155,    15,   147,     1,   149,   150,   151,   152,    22,
      23,   154,   570,   117,   155,     1,   156,   221,   118,   223,
     123,   225,   226,   118,   228,    22,    23,   158,   166,   167,
     168,   169,   166,   167,   168,   169,    22,    23,   118,   157,
     123,   287,     3,     4,     5,     6,   118,   118,     1,    10,
      11,    12,   256,   123,   158,    16,    17,    18,    19,    20,
     118,   159,   206,   123,   208,   149,   121,   122,   172,    22,
      23,   275,     1,   128,   129,   158,   166,   167,   168,   169,
     284,   285,   123,   156,   287,   331,   156,   170,    30,    31,
      32,   337,   338,    22,    23,   150,   342,   153,   124,   345,
     244,   121,   122,   247,   248,   337,   124,   353,   128,   129,
     123,   349,   123,   359,    16,    17,    18,    19,    20,     1,
     264,   149,   123,   267,   268,   363,   123,   271,   331,   159,
     274,   173,     1,   118,   337,   338,   353,   123,   118,   342,
      22,    23,   345,   126,   288,   289,   123,   126,   292,   293,
     126,    93,    94,    22,    23,   401,   359,     1,   111,     3,
       4,     5,     6,   120,   126,   122,    10,    11,    12,   401,
     416,   126,    16,    17,    18,    19,    20,   126,   322,   323,
     324,   325,   122,   126,   141,   142,   143,   126,   381,   122,
     383,    96,   396,    84,   155,     1,   340,   341,   401,   416,
     344,   349,   508,   473,   348,   166,   167,   168,   169,   344,
      16,    17,    18,    19,    20,   461,   451,    -1,    -1,   363,
       1,    -1,     3,     4,     5,     6,   121,   122,    -1,    10,
      11,    12,    -1,   128,   129,    16,    17,    18,    19,    20,
      -1,    22,    23,    -1,   121,   122,   484,     1,    -1,     1,
     488,   128,   129,   130,    -1,   150,   158,    -1,   461,    -1,
      -1,   405,    16,    17,    18,    19,    20,    -1,    22,    23,
      22,    23,    24,   417,    -1,   479,    -1,    -1,   422,     1,
     424,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    16,    17,    18,    19,    20,   451,     3,     4,
       5,     6,    -1,    -1,   158,    10,    11,    12,    -1,    -1,
      -1,    16,    17,    18,    19,    20,   170,    -1,    -1,   473,
      -1,    -1,   570,    -1,    -1,    -1,    -1,     6,     7,    -1,
     484,    10,    11,    -1,   488,    -1,   152,    -1,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    -1,    -1,    -1,
      29,    30,   608,    -1,   508,    34,    35,    36,    -1,    -1,
      39,    40,    -1,    42,    -1,    44,    -1,   158,    -1,    -1,
      49,    50,    -1,    52,    -1,    54,    55,    56,    57,   170,
      59,    -1,    -1,    -1,    63,    64,    65,    66,    67,    -1,
      69,    -1,    -1,    -1,   158,   608,    -1,    -1,    77,    -1,
      79,    80,    81,    82,    -1,    -1,    85,    86,    87,    -1,
     121,   122,   117,    -1,    93,    94,   570,   128,   129,    16,
      17,    18,    19,    20,    -1,     1,   158,     3,     4,     5,
       6,   155,    -1,     9,    10,    11,    12,    -1,   170,   150,
      16,    17,    18,    19,    20,    -1,    22,    23,    -1,   128,
      -1,   121,   122,   158,    -1,    -1,   135,    -1,   128,   129,
     139,   140,     3,     4,     5,     6,    -1,   172,    -1,    10,
      11,    12,     1,    -1,    -1,     1,    -1,    -1,    -1,   158,
     150,    -1,     8,     9,    -1,    -1,    -1,    16,    17,    18,
      19,    20,   171,    22,    23,    -1,   175,     1,   177,   178,
       4,    -1,    -1,   182,    -1,    -1,    10,    11,    12,    -1,
      -1,    37,    -1,   192,    -1,   194,   195,    -1,    22,    23,
     199,    -1,    -1,   202,    -1,   204,    -1,    -1,    -1,    -1,
     209,   210,    -1,    -1,   213,    -1,    -1,   216,    -1,   218,
     219,    -1,   221,   222,    -1,   224,    -1,    -1,   227,    -1,
      76,   230,    -1,   232,   233,    -1,   235,    -1,   237,    -1,
     239,    -1,   241,    -1,   243,     1,    -1,   246,     4,    -1,
      -1,    -1,   251,   252,    10,    11,    12,   256,    -1,   155,
     259,   260,    -1,   262,   263,    -1,   265,    -1,    -1,    -1,
     166,   167,   168,   169,    -1,    -1,    -1,   276,    -1,   125,
     279,    -1,   281,    -1,    -1,   284,   285,   286,    -1,    -1,
     151,    -1,    -1,    -1,    -1,    -1,    -1,   143,   144,    -1,
      -1,   147,    -1,   149,   165,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,   314,    -1,   316,    16,    17,
      18,    19,    20,    -1,    -1,    -1,    -1,   435,    -1,    -1,
      -1,    -1,   440,   332,   333,    -1,   444,    -1,    -1,    -1,
      -1,   449,   450,    -1,    -1,     1,    -1,     3,     4,     5,
       6,    -1,    -1,    -1,    10,    11,    12,    -1,    -1,    -1,
     206,    -1,   208,    -1,    -1,   364,    22,    23,    -1,    -1,
     369,   370,     3,     4,     5,     6,    -1,     1,    -1,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    20,
      -1,    -1,    16,    17,    18,    19,    20,   396,    22,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   408,
      -1,    -1,   411,    -1,   413,   414,   415,   416,   264,    -1,
      -1,   267,   268,    -1,    -1,   271,   534,   426,   274,   537,
     538,    -1,   540,   541,   542,     1,    -1,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    -1,    -1,    -1,
      16,    17,    18,    19,    20,    -1,    22,    23,    -1,    -1,
     158,   460,    -1,   462,    -1,    -1,   465,    -1,   467,    -1,
     469,   470,   170,    -1,   473,    -1,   322,   323,   324,   325,
     479,   480,    -1,    -1,   483,    -1,   485,   486,   487,   597,
      -1,    -1,   491,    -1,   340,   151,   495,    -1,    -1,   498,
      -1,    -1,   501,   502,   503,   504,   505,   506,    -1,   165,
     509,    -1,   511,   512,    -1,    -1,    -1,    -1,    -1,    -1,
     519,   520,   521,   522,   523,    -1,    -1,   158,    -1,    -1,
      -1,    -1,   156,    -1,    -1,   166,   167,   168,   169,   170,
      -1,    -1,    -1,    -1,    -1,     1,    -1,     3,     4,     5,
       6,    -1,    -1,     9,    10,    11,    12,    -1,    14,   405,
      16,    17,    18,    19,    20,    -1,    22,    23,    -1,    -1,
      -1,   417,     3,     4,     5,     6,    -1,     7,    -1,    10,
      11,    12,    -1,    13,    -1,    -1,    -1,    -1,    -1,   155,
      -1,    -1,    22,    -1,   160,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    -1,    -1,    44,    45,    46,    47,    48,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,    -1,   112,   113,   114,   115,   116,     1,    -1,     3,
       4,     5,     6,    -1,    -1,     9,    10,    11,    12,   155,
      -1,    -1,    16,    17,    18,    19,    20,    -1,    22,    23,
       3,     4,     5,     6,    -1,    -1,     7,    10,    11,    12,
     151,    -1,    13,    -1,   154,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   162,   163,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    -1,    44,    45,    46,    47,    48,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
      -1,   112,   113,   114,   115,   116,    -1,     1,    -1,     3,
       4,     5,     6,    -1,    -1,    -1,    10,    11,    12,    -1,
      -1,   155,    16,    17,    18,    19,    20,    -1,    22,    23,
       1,    -1,     3,     4,     5,     6,    -1,    -1,     9,    10,
      11,    12,    -1,    -1,    -1,    16,    17,    18,    19,    20,
      -1,   162,   163,     1,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    16,    17,
      18,    19,    20,     1,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,    -1,    16,    17,
      18,    19,    20,     1,    -1,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,    -1,    -1,    -1,    16,    17,
      18,    19,    20,     3,     4,     5,     6,    -1,    -1,     9,
      10,    11,    12,    -1,    -1,    -1,    16,    17,    18,    19,
      20,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,    -1,    -1,    -1,    16,    17,    18,    19,    20,     1,
      -1,     3,     4,     5,     6,    -1,    -1,    -1,    10,    11,
      12,   155,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,     3,     4,     5,     6,    -1,    -1,    -1,    10,
      11,    12,    -1,    -1,   155,     3,     4,     5,     6,    -1,
      -1,    -1,    10,    11,    12,     3,     4,     5,     6,    -1,
      -1,     9,    10,    11,    12,    -1,    -1,   155,    -1,    -1,
       3,     4,     5,     6,    -1,    -1,    -1,    10,    11,    12,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   155,     3,     4,
       5,     6,    -1,    -1,    -1,    10,    11,    12,    -1,     1,
      -1,     3,     4,     5,     6,    -1,    -1,   155,    10,    11,
      12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      22,    23,    -1,    -1,    -1,   155,    -1,    -1,    -1,    -1,
      -1,   123,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   155,    -1,    -1,   117,    -1,   119,   120,
      -1,    -1,   123,    -1,   125,    -1,   127,    -1,    -1,   151,
      -1,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,    -1,    -1,
      -1,   139,   140,   141,   142,   143,   144,   120,    -1,    -1,
     123,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   132,
      -1,   134,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,
      -1,    -1,    -1,   128,   129,   130,    -1,    -1,    -1,    -1,
      -1,   123
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    13,    22,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    44,    45,    46,    47,    48,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   112,   113,   114,
     115,   116,   154,   162,   163,   175,   176,   178,   179,   180,
     181,   182,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   224,     3,     4,     5,     6,    10,    11,    12,    16,
      17,    18,    19,    20,   158,   170,   199,   208,   211,   219,
     221,   222,     1,    22,    23,   164,   177,     1,   152,   195,
     164,   177,   177,   211,   211,   123,   177,   207,   177,   207,
     195,   195,   195,     1,     4,    10,    11,    12,   202,   220,
       9,   155,   204,   206,   208,   209,   210,   222,   204,   204,
     177,   206,   208,   177,   199,   200,   201,   208,   160,   177,
     160,   177,   203,   204,   206,    14,   177,   197,   198,   177,
     198,   177,   206,   204,   195,   195,   177,     1,   177,   202,
     202,   202,   206,   177,   203,   177,   207,   177,   207,   211,
     208,   153,   177,   193,   177,   198,   193,   177,   193,   156,
     177,   208,   195,   208,   195,   208,   206,   155,   208,   177,
     208,   177,   208,   195,   177,   195,   177,   203,   177,   203,
     177,   203,   177,   203,   206,   177,   193,   195,   195,   177,
     177,   203,   157,   177,   177,   177,   195,   177,   153,   194,
     151,   151,   195,   195,   195,   211,   177,   207,   207,   177,
     177,   207,   177,   177,   207,   195,   194,     1,   177,   196,
     211,   111,   177,   177,   195,   195,   164,    21,   117,   119,
     120,   123,   125,   127,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   222,   226,   177,   202,   177,   202,     0,   178,    22,
     211,   158,   166,   167,   168,   169,   177,   177,   177,   177,
     211,   211,   211,   211,     1,   196,     1,   196,   196,   177,
     207,   207,   207,   207,   207,   207,   177,   207,    21,   123,
     177,   177,   177,   207,   177,    14,   203,   203,   177,   207,
     177,   177,   177,    15,     1,   207,   177,   183,   177,   211,
     211,   177,   177,   177,   203,   177,   177,   177,   177,   207,
     177,   207,   177,   207,   207,   177,   207,   177,   177,   177,
     177,   177,   177,   177,   177,   151,   215,   216,   222,   177,
     196,   196,   177,   177,   177,   207,   177,   177,   151,   177,
     177,   211,   177,   211,   211,   211,   211,   207,   177,   177,
     177,   177,   207,   177,   207,   177,   206,   139,   140,   141,
     142,   143,   144,   222,   222,   123,   121,   122,   128,   129,
     150,   227,   228,   120,   123,   132,   134,   222,   222,   123,
     123,   123,   177,   177,     1,   171,   211,   211,   211,   211,
     205,   207,   206,   177,   177,   205,     1,   204,     1,   206,
     211,   151,   165,   217,   218,   222,   206,     9,   209,   215,
     206,   222,   201,   203,    15,   212,   212,   206,   117,   172,
     199,   213,   214,   222,   177,   151,   223,   224,   225,   177,
     177,   212,   156,   195,   195,   155,   155,   177,   207,   205,
       1,   204,   211,   177,   177,   177,   177,   177,   177,   203,
     211,   222,   222,    24,   177,   118,   118,   118,   118,   118,
     118,   118,   123,   227,   150,   227,   149,   123,   150,   227,
     123,   123,   123,   126,   227,   227,   121,   122,   129,   130,
     222,   228,   177,   206,   177,   177,   177,   177,   177,   177,
     218,   177,   177,   177,   213,   177,   177,   177,   213,   156,
     159,   177,   177,   124,   124,   177,   177,   177,   177,   177,
     177,   177,   216,   177,   177,   177,   177,   177,   177,   177,
     177,   123,   121,   129,   227,   149,   227,   150,   227,   126,
     149,   227,   227,   130,   227,   123,     1,   118,   173,     1,
     213,   224,   224,   120,   122,   141,   142,   143,   118,   118,
     126,   227,   126,   126,   149,   126,   126,   126,   126,   122,
     206,   123,   126,   126,   122
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   174,   175,   175,   175,   176,   176,   177,   177,   177,
     178,   178,   178,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   179,   179,   179,   179,   179,   179,   179,
     179,   179,   179,   179,   179,   179,   179,   179,   180,   180,
     180,   181,   181,   181,   181,   181,   181,   181,   181,   181,
     181,   181,   181,   181,   183,   182,   182,   182,   182,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   185,
     185,   185,   185,   185,   185,   185,   185,   186,   186,   186,
     186,   186,   186,   186,   186,   186,   186,   186,   187,   187,
     187,   187,   187,   187,   187,   187,   187,   187,   187,   187,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   188,   188,   188,   188,   188,   188,   188,
     188,   188,   188,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   189,   189,   189,   189,   189,   189,   189,   189,
     189,   189,   190,   190,   190,   191,   191,   192,   193,   194,
     194,   195,   195,   196,   196,   197,   197,   198,   198,   199,
     199,   200,   200,   201,   202,   202,   203,   203,   204,   204,
     204,   205,   205,   206,   206,   206,   207,   207,   208,   208,
     208,   208,   208,   209,   210,   211,   211,   211,   211,   211,
     211,   211,   212,   212,   213,   213,   213,   213,   213,   214,
     214,   214,   215,   215,   216,   216,   217,   217,   218,   218,
     218,   219,   219,   220,   220,   220,   220,   221,   221,   221,
     222,   222,   222,   222,   222,   223,   223,   223,   224,   225,
     225,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   227,   227,   227,   227,   228
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     3,     5,     3,     2,     2,
       3,     2,     3,     2,     4,     2,     3,     3,     2,     4,
       2,     4,     2,     4,     2,     4,     2,     1,     2,     3,
       3,     5,     3,     5,     3,     5,     3,     5,     3,     2,
       3,     2,     4,     5,     0,     5,     3,     3,     2,     5,
       5,     5,     5,     5,     3,     2,     3,     2,     3,     2,
       3,     2,     3,     2,     2,     2,     4,     5,     5,     5,
       2,     3,     2,     5,     2,     5,     2,     3,     2,     3,
       2,     3,     5,     3,     2,     5,     5,     4,     3,     2,
       3,     2,     3,     3,     2,     3,     2,     2,     2,     3,
       3,     3,     2,     3,     3,     3,     3,     2,     3,     2,
       3,     5,     3,     4,     3,     3,     2,     4,     4,     2,
       3,     3,     2,     5,     5,     5,     3,     4,     5,     4,
       5,     4,     5,     5,     2,     3,     4,     3,     3,     5,
       3,     5,     3,     2,     3,     4,     2,     2,     1,     1,
       0,     1,     1,     1,     1,     2,     1,     1,     0,     1,
       2,     3,     1,     3,     1,     1,     1,     1,     3,     1,
       3,     2,     0,     1,     3,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
       3,     1,     2,     0,     3,     3,     3,     3,     1,     1,
       1,     4,     3,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     2,     2,     1,
       1,     2,     1,     3,     3,     3,     3,     3,     5,     7,
       5,     3,     3,     3,     3,     3,     3,     5,     5,     5,
       5,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     3,     3,     4,     3,     4,     2,     3,     3,     3,
       3,     5,     5,     6,     5,     6,     4,     5,     5,     5,
       5,     3,     5,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

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
#ifndef YYINITDEPTH
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
static YYSIZE_T
yystrlen (const char *yystr)
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
static char *
yystpcpy (char *yydest, const char *yysrc)
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
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
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
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

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

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
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
  int yytoken = 0;
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

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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
                  (unsigned long) yystacksize));

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
      yychar = yylex ();
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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 199 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = 0; }
#line 2385 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 3:
#line 200 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = 0; }
#line 2391 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 4:
#line 201 "mon_parse.y" /* yacc.c:1651  */
    { new_cmd = 1; asm_mode = 0;  (yyval.i) = 0; }
#line 2397 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 9:
#line 210 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_END_CMD; }
#line 2403 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 22:
#line 225 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_BAD_CMD; }
#line 2409 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 23:
#line 229 "mon_parse.y" /* yacc.c:1651  */
    { mon_bank(e_default_space, NULL); }
#line 2415 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 24:
#line 231 "mon_parse.y" /* yacc.c:1651  */
    { mon_bank((yyvsp[-1].i), NULL); }
#line 2421 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 25:
#line 233 "mon_parse.y" /* yacc.c:1651  */
    { mon_bank(e_default_space, (yyvsp[-1].str)); }
#line 2427 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 26:
#line 235 "mon_parse.y" /* yacc.c:1651  */
    { mon_bank((yyvsp[-3].i), (yyvsp[-1].str)); }
#line 2433 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 27:
#line 237 "mon_parse.y" /* yacc.c:1651  */
    { mon_jump((yyvsp[-1].a)); }
#line 2439 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 28:
#line 239 "mon_parse.y" /* yacc.c:1651  */
    { mon_go(); }
#line 2445 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 29:
#line 241 "mon_parse.y" /* yacc.c:1651  */
    { mon_display_io_regs(0); }
#line 2451 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 30:
#line 243 "mon_parse.y" /* yacc.c:1651  */
    { mon_display_io_regs((yyvsp[-1].a)); }
#line 2457 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 31:
#line 245 "mon_parse.y" /* yacc.c:1651  */
    { monitor_cpu_type_set(""); }
#line 2463 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 32:
#line 247 "mon_parse.y" /* yacc.c:1651  */
    { monitor_cpu_type_set((yyvsp[-1].str)); }
#line 2469 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 33:
#line 249 "mon_parse.y" /* yacc.c:1651  */
    { mon_cpuhistory(-1); }
#line 2475 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 34:
#line 251 "mon_parse.y" /* yacc.c:1651  */
    { mon_cpuhistory((yyvsp[-1].i)); }
#line 2481 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 35:
#line 253 "mon_parse.y" /* yacc.c:1651  */
    { mon_instruction_return(); }
#line 2487 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 36:
#line 255 "mon_parse.y" /* yacc.c:1651  */
    { machine_write_snapshot((yyvsp[-1].str),0,0,0); /* FIXME */ }
#line 2493 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 37:
#line 257 "mon_parse.y" /* yacc.c:1651  */
    { machine_read_snapshot((yyvsp[-1].str), 0); }
#line 2499 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 38:
#line 259 "mon_parse.y" /* yacc.c:1651  */
    { mon_instructions_step(-1); }
#line 2505 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 39:
#line 261 "mon_parse.y" /* yacc.c:1651  */
    { mon_instructions_step((yyvsp[-1].i)); }
#line 2511 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 40:
#line 263 "mon_parse.y" /* yacc.c:1651  */
    { mon_instructions_next(-1); }
#line 2517 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 41:
#line 265 "mon_parse.y" /* yacc.c:1651  */
    { mon_instructions_next((yyvsp[-1].i)); }
#line 2523 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 42:
#line 267 "mon_parse.y" /* yacc.c:1651  */
    { mon_stack_up(-1); }
#line 2529 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 43:
#line 269 "mon_parse.y" /* yacc.c:1651  */
    { mon_stack_up((yyvsp[-1].i)); }
#line 2535 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 44:
#line 271 "mon_parse.y" /* yacc.c:1651  */
    { mon_stack_down(-1); }
#line 2541 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 45:
#line 273 "mon_parse.y" /* yacc.c:1651  */
    { mon_stack_down((yyvsp[-1].i)); }
#line 2547 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 46:
#line 275 "mon_parse.y" /* yacc.c:1651  */
    { mon_display_screen(); }
#line 2553 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 48:
#line 280 "mon_parse.y" /* yacc.c:1651  */
    { (monitor_cpu_for_memspace[default_memspace]->mon_register_print)(default_memspace); }
#line 2559 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 49:
#line 282 "mon_parse.y" /* yacc.c:1651  */
    { (monitor_cpu_for_memspace[(yyvsp[-1].i)]->mon_register_print)((yyvsp[-1].i)); }
#line 2565 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 51:
#line 287 "mon_parse.y" /* yacc.c:1651  */
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[-1].str));
                    }
#line 2574 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 52:
#line 292 "mon_parse.y" /* yacc.c:1651  */
    {
                        /* What about the memspace? */
                        mon_playback_init((yyvsp[-1].str));
                    }
#line 2583 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 53:
#line 297 "mon_parse.y" /* yacc.c:1651  */
    { mon_save_symbols((yyvsp[-3].i), (yyvsp[-1].str)); }
#line 2589 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 54:
#line 299 "mon_parse.y" /* yacc.c:1651  */
    { mon_save_symbols(e_default_space, (yyvsp[-1].str)); }
#line 2595 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 55:
#line 301 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_name_to_symbol_table((yyvsp[-3].a), (yyvsp[-1].str)); }
#line 2601 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 56:
#line 303 "mon_parse.y" /* yacc.c:1651  */
    { mon_remove_name_from_symbol_table(e_default_space, (yyvsp[-1].str)); }
#line 2607 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 57:
#line 305 "mon_parse.y" /* yacc.c:1651  */
    { mon_remove_name_from_symbol_table((yyvsp[-3].i), (yyvsp[-1].str)); }
#line 2613 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 58:
#line 307 "mon_parse.y" /* yacc.c:1651  */
    { mon_print_symbol_table((yyvsp[-1].i)); }
#line 2619 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 59:
#line 309 "mon_parse.y" /* yacc.c:1651  */
    { mon_print_symbol_table(e_default_space); }
#line 2625 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 60:
#line 311 "mon_parse.y" /* yacc.c:1651  */
    { mon_clear_symbol_table((yyvsp[-1].i)); }
#line 2631 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 61:
#line 313 "mon_parse.y" /* yacc.c:1651  */
    { mon_clear_symbol_table(e_default_space); }
#line 2637 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 62:
#line 315 "mon_parse.y" /* yacc.c:1651  */
    {
                        mon_add_name_to_symbol_table((yyvsp[-1].a), mon_prepend_dot_to_name((yyvsp[-3].str)));
                    }
#line 2645 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 63:
#line 319 "mon_parse.y" /* yacc.c:1651  */
    {
                        mon_add_name_to_symbol_table((yyvsp[-2].a), mon_prepend_dot_to_name((yyvsp[-4].str)));
                    }
#line 2653 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 64:
#line 325 "mon_parse.y" /* yacc.c:1651  */
    { mon_start_assemble_mode((yyvsp[0].a), NULL); }
#line 2659 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 65:
#line 326 "mon_parse.y" /* yacc.c:1651  */
    { }
#line 2665 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 66:
#line 328 "mon_parse.y" /* yacc.c:1651  */
    { mon_start_assemble_mode((yyvsp[-1].a), NULL); }
#line 2671 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 67:
#line 330 "mon_parse.y" /* yacc.c:1651  */
    { mon_disassemble_lines((yyvsp[-1].range)[0], (yyvsp[-1].range)[1]); }
#line 2677 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 68:
#line 332 "mon_parse.y" /* yacc.c:1651  */
    { mon_disassemble_lines(BAD_ADDR, BAD_ADDR); }
#line 2683 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 69:
#line 336 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_move((yyvsp[-3].range)[0], (yyvsp[-3].range)[1], (yyvsp[-1].a)); }
#line 2689 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 70:
#line 338 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_compare((yyvsp[-3].range)[0], (yyvsp[-3].range)[1], (yyvsp[-1].a)); }
#line 2695 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 71:
#line 340 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_fill((yyvsp[-3].range)[0], (yyvsp[-3].range)[1],(unsigned char *)(yyvsp[-1].str)); }
#line 2701 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 72:
#line 342 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_hunt((yyvsp[-3].range)[0], (yyvsp[-3].range)[1],(unsigned char *)(yyvsp[-1].str)); }
#line 2707 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 73:
#line 344 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display((yyvsp[-3].rt), (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], DF_PETSCII); }
#line 2713 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 74:
#line 346 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(default_radix, (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], DF_PETSCII); }
#line 2719 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 75:
#line 348 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(default_radix, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
#line 2725 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 76:
#line 350 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display_data((yyvsp[-1].range)[0], (yyvsp[-1].range)[1], 8, 8); }
#line 2731 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 77:
#line 352 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 8, 8); }
#line 2737 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 78:
#line 354 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display_data((yyvsp[-1].range)[0], (yyvsp[-1].range)[1], 24, 21); }
#line 2743 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 79:
#line 356 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 24, 21); }
#line 2749 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 80:
#line 358 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(0, (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], DF_PETSCII); }
#line 2755 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 81:
#line 360 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
#line 2761 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 82:
#line 362 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(0, (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], DF_SCREEN_CODE); }
#line 2767 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 83:
#line 364 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_SCREEN_CODE); }
#line 2773 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 84:
#line 366 "mon_parse.y" /* yacc.c:1651  */
    { mon_memmap_zap(); }
#line 2779 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 85:
#line 368 "mon_parse.y" /* yacc.c:1651  */
    { mon_memmap_show(-1,BAD_ADDR,BAD_ADDR); }
#line 2785 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 86:
#line 370 "mon_parse.y" /* yacc.c:1651  */
    { mon_memmap_show((yyvsp[-1].i),BAD_ADDR,BAD_ADDR); }
#line 2791 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 87:
#line 372 "mon_parse.y" /* yacc.c:1651  */
    { mon_memmap_show((yyvsp[-2].i),(yyvsp[-1].range)[0],(yyvsp[-1].range)[1]); }
#line 2797 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 88:
#line 374 "mon_parse.y" /* yacc.c:1651  */
    { mon_memmap_save((yyvsp[-3].str),(yyvsp[-1].i)); }
#line 2803 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 89:
#line 378 "mon_parse.y" /* yacc.c:1651  */
    {
                      if ((yyvsp[-3].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], TRUE, (yyvsp[-3].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], TRUE, e_exec, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[-1].cond_node));
                  }
#line 2816 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 90:
#line 387 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_print_checkpoints(); }
#line 2822 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 91:
#line 390 "mon_parse.y" /* yacc.c:1651  */
    {
                      mon_breakpoint_add_checkpoint((yyvsp[-1].range)[0], (yyvsp[-1].range)[1], TRUE, e_exec, TRUE);
                  }
#line 2830 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 92:
#line 394 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_print_checkpoints(); }
#line 2836 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 93:
#line 397 "mon_parse.y" /* yacc.c:1651  */
    {
                      if ((yyvsp[-3].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], TRUE, (yyvsp[-3].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], TRUE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[-1].cond_node));
                  }
#line 2849 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 94:
#line 406 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_print_checkpoints(); }
#line 2855 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 95:
#line 409 "mon_parse.y" /* yacc.c:1651  */
    {
                      if ((yyvsp[-3].i)) {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], FALSE, (yyvsp[-3].i), FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint((yyvsp[-2].range)[0], (yyvsp[-2].range)[1], FALSE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, (yyvsp[-1].cond_node));
                  }
#line 2868 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 96:
#line 418 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_print_checkpoints(); }
#line 2874 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 97:
#line 423 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_switch_checkpoint(e_ON, (yyvsp[-1].i)); }
#line 2880 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 98:
#line 425 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_switch_checkpoint(e_ON, -1); }
#line 2886 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 99:
#line 427 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_switch_checkpoint(e_OFF, (yyvsp[-1].i)); }
#line 2892 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 100:
#line 429 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_switch_checkpoint(e_OFF, -1); }
#line 2898 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 101:
#line 431 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_set_ignore_count((yyvsp[-1].i), -1); }
#line 2904 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 102:
#line 433 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_set_ignore_count((yyvsp[-3].i), (yyvsp[-1].i)); }
#line 2910 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 103:
#line 435 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_delete_checkpoint((yyvsp[-1].i)); }
#line 2916 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 104:
#line 437 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_delete_checkpoint(-1); }
#line 2922 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 105:
#line 439 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_set_checkpoint_condition((yyvsp[-3].i), (yyvsp[-1].cond_node)); }
#line 2928 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 106:
#line 441 "mon_parse.y" /* yacc.c:1651  */
    { mon_breakpoint_set_checkpoint_command((yyvsp[-3].i), (yyvsp[-1].str)); }
#line 2934 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 107:
#line 443 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_STRING; }
#line 2940 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 108:
#line 447 "mon_parse.y" /* yacc.c:1651  */
    { sidefx = (((yyvsp[-1].action) == e_TOGGLE) ? (sidefx ^ 1) : (yyvsp[-1].action)); }
#line 2946 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 109:
#line 449 "mon_parse.y" /* yacc.c:1651  */
    {
                         mon_out("I/O side effects are %s\n",
                                   sidefx ? "enabled" : "disabled");
                     }
#line 2955 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 110:
#line 454 "mon_parse.y" /* yacc.c:1651  */
    { 
                        int logenabled;
                        resources_get_int("MonitorLogEnabled", &logenabled);
                        logenabled = (((yyvsp[-1].action) == e_TOGGLE) ? (logenabled ^ 1) : (yyvsp[-1].action));
                        resources_set_int("MonitorLogEnabled", logenabled);
                     }
#line 2966 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 111:
#line 461 "mon_parse.y" /* yacc.c:1651  */
    {
                         int logenabled;
                         const char *logfilename;
                         resources_get_int("MonitorLogEnabled", &logenabled);
                         resources_get_string("MonitorLogFileName", &logfilename);
                         if (logenabled) {
                            mon_out("Logging to '%s' is enabled.\n", logfilename);
                         } else {
                            mon_out("Logging is disabled.\n");
                         }
                     }
#line 2982 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 112:
#line 473 "mon_parse.y" /* yacc.c:1651  */
    { 
                        resources_set_string("MonitorLogFileName", (yyvsp[-1].str));
                     }
#line 2990 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 113:
#line 477 "mon_parse.y" /* yacc.c:1651  */
    { default_radix = (yyvsp[-1].rt); }
#line 2996 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 114:
#line 479 "mon_parse.y" /* yacc.c:1651  */
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
#line 3017 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 115:
#line 497 "mon_parse.y" /* yacc.c:1651  */
    { monitor_change_device((yyvsp[-1].i)); }
#line 3023 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 116:
#line 499 "mon_parse.y" /* yacc.c:1651  */
    { mon_export(); }
#line 3029 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 117:
#line 501 "mon_parse.y" /* yacc.c:1651  */
    { mon_quit(); YYACCEPT; }
#line 3035 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 118:
#line 503 "mon_parse.y" /* yacc.c:1651  */
    { mon_exit(); YYACCEPT; }
#line 3041 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 119:
#line 505 "mon_parse.y" /* yacc.c:1651  */
    { mon_maincpu_toggle_trace((yyvsp[-1].action)); }
#line 3047 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 120:
#line 509 "mon_parse.y" /* yacc.c:1651  */
    { mon_drive_execute_disk_cmd((yyvsp[-1].str)); }
#line 3053 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 121:
#line 511 "mon_parse.y" /* yacc.c:1651  */
    { mon_out("\t%d\n",(yyvsp[-1].i)); }
#line 3059 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 122:
#line 513 "mon_parse.y" /* yacc.c:1651  */
    { mon_command_print_help(NULL); }
#line 3065 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 123:
#line 515 "mon_parse.y" /* yacc.c:1651  */
    { mon_command_print_help((yyvsp[-1].str)); }
#line 3071 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 124:
#line 517 "mon_parse.y" /* yacc.c:1651  */
    { mon_print_convert((yyvsp[-1].i)); }
#line 3077 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 125:
#line 519 "mon_parse.y" /* yacc.c:1651  */
    { mon_change_dir((yyvsp[-1].str)); }
#line 3083 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 126:
#line 521 "mon_parse.y" /* yacc.c:1651  */
    { mon_keyboard_feed((yyvsp[-1].str)); }
#line 3089 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 127:
#line 523 "mon_parse.y" /* yacc.c:1651  */
    { mon_backtrace(); }
#line 3095 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 128:
#line 525 "mon_parse.y" /* yacc.c:1651  */
    { mon_show_dir((yyvsp[-1].str)); }
#line 3101 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 129:
#line 527 "mon_parse.y" /* yacc.c:1651  */
    { mon_show_pwd(); }
#line 3107 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 130:
#line 529 "mon_parse.y" /* yacc.c:1651  */
    { mon_screenshot_save((yyvsp[-1].str),-1); }
#line 3113 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 131:
#line 531 "mon_parse.y" /* yacc.c:1651  */
    { mon_screenshot_save((yyvsp[-3].str),(yyvsp[-1].i)); }
#line 3119 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 132:
#line 533 "mon_parse.y" /* yacc.c:1651  */
    { mon_resource_get((yyvsp[-1].str)); }
#line 3125 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 133:
#line 535 "mon_parse.y" /* yacc.c:1651  */
    { mon_resource_set((yyvsp[-2].str),(yyvsp[-1].str)); }
#line 3131 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 134:
#line 537 "mon_parse.y" /* yacc.c:1651  */
    { resources_load((yyvsp[-1].str)); }
#line 3137 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 135:
#line 539 "mon_parse.y" /* yacc.c:1651  */
    { resources_save((yyvsp[-1].str)); }
#line 3143 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 136:
#line 541 "mon_parse.y" /* yacc.c:1651  */
    { mon_reset_machine(-1); }
#line 3149 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 137:
#line 543 "mon_parse.y" /* yacc.c:1651  */
    { mon_reset_machine((yyvsp[-1].i)); }
#line 3155 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 138:
#line 545 "mon_parse.y" /* yacc.c:1651  */
    { mon_tape_ctrl((yyvsp[-1].i)); }
#line 3161 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 139:
#line 547 "mon_parse.y" /* yacc.c:1651  */
    { mon_cart_freeze(); }
#line 3167 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 140:
#line 549 "mon_parse.y" /* yacc.c:1651  */
    { }
#line 3173 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 141:
#line 551 "mon_parse.y" /* yacc.c:1651  */
    { mon_stopwatch_reset(); }
#line 3179 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 142:
#line 553 "mon_parse.y" /* yacc.c:1651  */
    { mon_stopwatch_show("Stopwatch: ", "\n"); }
#line 3185 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 143:
#line 557 "mon_parse.y" /* yacc.c:1651  */
    { mon_file_load((yyvsp[-3].str), (yyvsp[-2].i), (yyvsp[-1].a), FALSE); }
#line 3191 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 144:
#line 559 "mon_parse.y" /* yacc.c:1651  */
    { mon_file_load((yyvsp[-3].str), (yyvsp[-2].i), (yyvsp[-1].a), TRUE); }
#line 3197 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 145:
#line 561 "mon_parse.y" /* yacc.c:1651  */
    { mon_file_save((yyvsp[-3].str), (yyvsp[-2].i), (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], FALSE); }
#line 3203 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 146:
#line 563 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_DEVICE_NUM; }
#line 3209 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 147:
#line 565 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_ADDRESS; }
#line 3215 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 148:
#line 567 "mon_parse.y" /* yacc.c:1651  */
    { mon_file_save((yyvsp[-3].str), (yyvsp[-2].i), (yyvsp[-1].range)[0], (yyvsp[-1].range)[1], TRUE); }
#line 3221 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 149:
#line 569 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_ADDRESS; }
#line 3227 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 150:
#line 571 "mon_parse.y" /* yacc.c:1651  */
    { mon_file_verify((yyvsp[-3].str),(yyvsp[-2].i),(yyvsp[-1].a)); }
#line 3233 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 151:
#line 573 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_ADDRESS; }
#line 3239 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 152:
#line 575 "mon_parse.y" /* yacc.c:1651  */
    { mon_drive_block_cmd(0,(yyvsp[-3].i),(yyvsp[-2].i),(yyvsp[-1].a)); }
#line 3245 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 153:
#line 577 "mon_parse.y" /* yacc.c:1651  */
    { mon_drive_block_cmd(1,(yyvsp[-3].i),(yyvsp[-2].i),(yyvsp[-1].a)); }
#line 3251 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 154:
#line 579 "mon_parse.y" /* yacc.c:1651  */
    { mon_drive_list(-1); }
#line 3257 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 155:
#line 581 "mon_parse.y" /* yacc.c:1651  */
    { mon_drive_list((yyvsp[-1].i)); }
#line 3263 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 156:
#line 583 "mon_parse.y" /* yacc.c:1651  */
    { mon_attach((yyvsp[-2].str),(yyvsp[-1].i)); }
#line 3269 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 157:
#line 585 "mon_parse.y" /* yacc.c:1651  */
    { mon_detach((yyvsp[-1].i)); }
#line 3275 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 158:
#line 587 "mon_parse.y" /* yacc.c:1651  */
    { mon_autostart((yyvsp[-1].str),0,1); }
#line 3281 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 159:
#line 589 "mon_parse.y" /* yacc.c:1651  */
    { mon_autostart((yyvsp[-3].str),(yyvsp[-1].i),1); }
#line 3287 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 160:
#line 591 "mon_parse.y" /* yacc.c:1651  */
    { mon_autostart((yyvsp[-1].str),0,0); }
#line 3293 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 161:
#line 593 "mon_parse.y" /* yacc.c:1651  */
    { mon_autostart((yyvsp[-3].str),(yyvsp[-1].i),0); }
#line 3299 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 162:
#line 597 "mon_parse.y" /* yacc.c:1651  */
    { mon_record_commands((yyvsp[-1].str)); }
#line 3305 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 163:
#line 599 "mon_parse.y" /* yacc.c:1651  */
    { mon_end_recording(); }
#line 3311 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 164:
#line 601 "mon_parse.y" /* yacc.c:1651  */
    { mon_playback_init((yyvsp[-1].str)); }
#line 3317 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 165:
#line 605 "mon_parse.y" /* yacc.c:1651  */
    { mon_memory_fill((yyvsp[-2].a), BAD_ADDR, (unsigned char *)(yyvsp[-1].str)); }
#line 3323 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 166:
#line 607 "mon_parse.y" /* yacc.c:1651  */
    { printf("Not yet.\n"); }
#line 3329 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 167:
#line 611 "mon_parse.y" /* yacc.c:1651  */
    { yydebug = 1; }
#line 3335 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 168:
#line 614 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.str) = (yyvsp[0].str); }
#line 3341 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 169:
#line 617 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.str) = (yyvsp[0].str); }
#line 3347 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 170:
#line 618 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.str) = NULL; }
#line 3353 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 172:
#line 622 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_FILENAME; }
#line 3359 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 174:
#line 626 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_DEVICE_NUM; }
#line 3365 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 175:
#line 629 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[-1].i) | (yyvsp[0].i); }
#line 3371 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 176:
#line 630 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3377 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 177:
#line 633 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3383 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 178:
#line 634 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = 0; }
#line 3389 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 179:
#line 637 "mon_parse.y" /* yacc.c:1651  */
    {
                                    if (!mon_register_valid(default_memspace, (yyvsp[0].reg))) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    (yyval.i) = new_reg(default_memspace, (yyvsp[0].reg));
                                }
#line 3400 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 180:
#line 643 "mon_parse.y" /* yacc.c:1651  */
    {
                                    if (!mon_register_valid((yyvsp[-1].i), (yyvsp[0].reg))) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    (yyval.i) = new_reg((yyvsp[-1].i), (yyvsp[0].reg));
                                }
#line 3411 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 183:
#line 656 "mon_parse.y" /* yacc.c:1651  */
    { (monitor_cpu_for_memspace[reg_memspace((yyvsp[-2].i))]->mon_register_set_val)(reg_memspace((yyvsp[-2].i)), reg_regid((yyvsp[-2].i)), (uint16_t) (yyvsp[0].i)); }
#line 3417 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 184:
#line 659 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3423 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 185:
#line 660 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_EXPECT_CHECKNUM; }
#line 3429 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 187:
#line 664 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.range)[0] = (yyvsp[0].a); (yyval.range)[1] = BAD_ADDR; }
#line 3435 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 188:
#line 667 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.range)[0] = (yyvsp[-2].a); (yyval.range)[1] = (yyvsp[0].a); }
#line 3441 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 189:
#line 669 "mon_parse.y" /* yacc.c:1651  */
    { if (resolve_range(e_default_space, (yyval.range), (yyvsp[0].str))) return ERR_ADDR_TOO_BIG; }
#line 3447 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 190:
#line 671 "mon_parse.y" /* yacc.c:1651  */
    { if (resolve_range((yyvsp[-2].i), (yyval.range), (yyvsp[0].str))) return ERR_ADDR_TOO_BIG; }
#line 3453 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 191:
#line 674 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.a) = (yyvsp[0].a); }
#line 3459 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 192:
#line 675 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.a) = BAD_ADDR; }
#line 3465 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 193:
#line 679 "mon_parse.y" /* yacc.c:1651  */
    {
             (yyval.a) = new_addr(e_default_space,(yyvsp[0].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
#line 3474 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 194:
#line 684 "mon_parse.y" /* yacc.c:1651  */
    {
             (yyval.a) = new_addr((yyvsp[-2].i), (yyvsp[0].i));
             if (opt_asm) new_cmd = asm_mode = 1;
         }
#line 3483 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 195:
#line 689 "mon_parse.y" /* yacc.c:1651  */
    {
             temp = mon_symbol_table_lookup_addr(e_default_space, (yyvsp[0].str));
             if (temp >= 0)
                 (yyval.a) = new_addr(e_default_space, temp);
             else
                 return ERR_UNDEFINED_LABEL;
         }
#line 3495 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 198:
#line 702 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = e_comp_space; }
#line 3501 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 199:
#line 703 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = e_disk8_space; }
#line 3507 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 200:
#line 704 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = e_disk9_space; }
#line 3513 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 201:
#line 705 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = e_disk10_space; }
#line 3519 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 202:
#line 706 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = e_disk11_space; }
#line 3525 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 203:
#line 709 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); if (!CHECK_ADDR((yyvsp[0].i))) return ERR_ADDR_TOO_BIG; }
#line 3531 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 204:
#line 711 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3537 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 205:
#line 713 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[-2].i) + (yyvsp[0].i); }
#line 3543 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 206:
#line 714 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[-2].i) - (yyvsp[0].i); }
#line 3549 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 207:
#line 715 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[-2].i) * (yyvsp[0].i); }
#line 3555 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 208:
#line 716 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = ((yyvsp[0].i)) ? ((yyvsp[-2].i) / (yyvsp[0].i)) : 1; }
#line 3561 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 209:
#line 717 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[-1].i); }
#line 3567 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 210:
#line 718 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_MISSING_CLOSE_PAREN; }
#line 3573 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 211:
#line 719 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3579 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 212:
#line 722 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = (yyvsp[0].cond_node); }
#line 3585 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 213:
#line 723 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = 0; }
#line 3591 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 214:
#line 726 "mon_parse.y" /* yacc.c:1651  */
    {
               (yyval.cond_node) = new_cond; (yyval.cond_node)->is_parenthized = FALSE;
               (yyval.cond_node)->child1 = (yyvsp[-2].cond_node); (yyval.cond_node)->child2 = (yyvsp[0].cond_node); (yyval.cond_node)->operation = (yyvsp[-1].cond_op);
           }
#line 3600 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 215:
#line 731 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_INCOMPLETE_COND_OP; }
#line 3606 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 216:
#line 733 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = (yyvsp[-1].cond_node); (yyval.cond_node)->is_parenthized = TRUE; }
#line 3612 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 217:
#line 735 "mon_parse.y" /* yacc.c:1651  */
    { return ERR_MISSING_CLOSE_PAREN; }
#line 3618 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 218:
#line 737 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = (yyvsp[0].cond_node); }
#line 3624 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 219:
#line 740 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->reg_num = (yyvsp[0].i); (yyval.cond_node)->is_reg = TRUE; (yyval.cond_node)->banknum=-1;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
#line 3635 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 220:
#line 746 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.cond_node) = new_cond;
                            (yyval.cond_node)->operation = e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->value = (yyvsp[0].i); (yyval.cond_node)->is_reg = FALSE; (yyval.cond_node)->banknum=-1;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;
                          }
#line 3646 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 221:
#line 752 "mon_parse.y" /* yacc.c:1651  */
    {(yyval.cond_node)=new_cond;
                            (yyval.cond_node)->operation=e_INV;
                            (yyval.cond_node)->is_parenthized = FALSE;
                            (yyval.cond_node)->banknum=mon_banknum_from_bank(e_default_space,(yyvsp[-2].str)); (yyval.cond_node)->value = (yyvsp[0].a); (yyval.cond_node)->is_reg = FALSE;
                            (yyval.cond_node)->child1 = NULL; (yyval.cond_node)->child2 = NULL;  
                        }
#line 3657 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 224:
#line 764 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_number_to_buffer((yyvsp[0].i)); }
#line 3663 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 225:
#line 765 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_string_to_buffer((yyvsp[0].str)); }
#line 3669 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 228:
#line 772 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_number_to_buffer((yyvsp[0].i)); }
#line 3675 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 229:
#line 773 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_number_masked_to_buffer((yyvsp[0].i), 0x00); }
#line 3681 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 230:
#line 774 "mon_parse.y" /* yacc.c:1651  */
    { mon_add_string_to_buffer((yyvsp[0].str)); }
#line 3687 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 231:
#line 777 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3693 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 232:
#line 778 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (monitor_cpu_for_memspace[reg_memspace((yyvsp[0].i))]->mon_register_get_val)(reg_memspace((yyvsp[0].i)), reg_regid((yyvsp[0].i))); }
#line 3699 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 233:
#line 781 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3705 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 234:
#line 782 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = strtol((yyvsp[0].str), NULL, 10); }
#line 3711 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 235:
#line 783 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = strtol((yyvsp[0].str), NULL, 10); }
#line 3717 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 236:
#line 784 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = strtol((yyvsp[0].str), NULL, 10); }
#line 3723 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 237:
#line 787 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = resolve_datatype(B_NUMBER,(yyvsp[0].str)); }
#line 3729 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 238:
#line 788 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = resolve_datatype(O_NUMBER,(yyvsp[0].str)); }
#line 3735 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 239:
#line 789 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = resolve_datatype(D_NUMBER,(yyvsp[0].str)); }
#line 3741 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 240:
#line 792 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3747 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 241:
#line 793 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3753 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 242:
#line 794 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3759 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 243:
#line 795 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3765 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 244:
#line 796 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 3771 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 248:
#line 804 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = 0;
                                                if ((yyvsp[-1].str)) {
                                                    (monitor_cpu_for_memspace[default_memspace]->mon_assemble_instr)((yyvsp[-1].str), (yyvsp[0].mode));
                                                } else {
                                                    new_cmd = 1;
                                                    asm_mode = 0;
                                                }
                                                opt_asm = 0;
                                              }
#line 3785 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 250:
#line 815 "mon_parse.y" /* yacc.c:1651  */
    { asm_mode = 0; }
#line 3791 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 251:
#line 818 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[0].i) > 0xff) {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE_16;
                          (yyval.mode).param = (yyvsp[0].i);
                        } else {
                          (yyval.mode).addr_mode = ASM_ADDR_MODE_IMMEDIATE;
                          (yyval.mode).param = (yyvsp[0].i);
                        } }
#line 3803 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 252:
#line 825 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[0].i) >= 0x10000) {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG;
               (yyval.mode).param = (yyvsp[0].i);
             } else if ((yyvsp[0].i) < 0x100) {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE;
               (yyval.mode).param = (yyvsp[0].i);
             } else {
               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE;
               (yyval.mode).param = (yyvsp[0].i);
             }
           }
#line 3819 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 253:
#line 836 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-2].i) >= 0x10000) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG_X;
                            (yyval.mode).param = (yyvsp[-2].i);
                          } else if ((yyvsp[-2].i) < 0x100) { 
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE_X;
                            (yyval.mode).param = (yyvsp[-2].i);
                          } else {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_X;
                            (yyval.mode).param = (yyvsp[-2].i);
                          }
                        }
#line 3835 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 254:
#line 847 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-2].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ZERO_PAGE_Y;
                            (yyval.mode).param = (yyvsp[-2].i);
                          } else {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_Y;
                            (yyval.mode).param = (yyvsp[-2].i);
                          }
                        }
#line 3848 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 255:
#line 855 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-2].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_STACK_RELATIVE;
                            (yyval.mode).param = (yyvsp[-2].i);
                          } else { /* 6809 */
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
                            if ((yyvsp[-2].i) >= -16 && (yyvsp[-2].i) < 16) {
                                (yyval.mode).addr_submode = (yyvsp[0].i) | ((yyvsp[-2].i) & 0x1F);
                            } else if ((yyvsp[-2].i) >= -128 && (yyvsp[-2].i) < 128) {
                                (yyval.mode).addr_submode = 0x80 | (yyvsp[0].i) | ASM_ADDR_MODE_INDEXED_OFF8;
                                (yyval.mode).param = (yyvsp[-2].i);
                            } else if ((yyvsp[-2].i) >= -32768 && (yyvsp[-2].i) < 32768) {
                                (yyval.mode).addr_submode = 0x80 | (yyvsp[0].i) | ASM_ADDR_MODE_INDEXED_OFF16;
                                (yyval.mode).param = (yyvsp[-2].i);
                            } else {
                                (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
                                mon_out("offset too large even for 16 bits (signed)\n");
                            }
                          }
                        }
#line 3872 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 256:
#line 874 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-2].i) < 0x100) {
                            (yyval.mode).addr_mode = ASM_ADDR_MODE_DOUBLE;
                            (yyval.mode).param = (yyvsp[0].i);
                            (yyval.mode).addr_submode = (yyvsp[-2].i);
                          }
                        }
#line 3883 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 257:
#line 880 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-1].i) < 0x100) {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT;
                               (yyval.mode).param = (yyvsp[-1].i);
                             } else {
                               (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT;
                               (yyval.mode).param = (yyvsp[-1].i);
                             }
                           }
#line 3896 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 258:
#line 888 "mon_parse.y" /* yacc.c:1651  */
    { if ((yyvsp[-3].i) < 0x100) {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[-3].i);
                                         } else {
                                           (yyval.mode).addr_mode = ASM_ADDR_MODE_ABS_INDIRECT_X;
                                           (yyval.mode).param = (yyvsp[-3].i);
                                         }
                                       }
#line 3909 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 259:
#line 897 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_STACK_RELATIVE_Y; (yyval.mode).param = (yyvsp[-5].i); }
#line 3915 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 260:
#line 899 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_Y; (yyval.mode).param = (yyvsp[-3].i); }
#line 3921 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 261:
#line 900 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_BC; }
#line 3927 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 262:
#line 901 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_DE; }
#line 3933 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 263:
#line 902 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_HL; }
#line 3939 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 264:
#line 903 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IX; }
#line 3945 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 265:
#line 904 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_IY; }
#line 3951 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 266:
#line 905 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IND_SP; }
#line 3957 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 267:
#line 907 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_A; (yyval.mode).param = (yyvsp[-3].i); }
#line 3963 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 268:
#line 909 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_HL; (yyval.mode).param = (yyvsp[-3].i); }
#line 3969 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 269:
#line 911 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IX; (yyval.mode).param = (yyvsp[-3].i); }
#line 3975 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 270:
#line 913 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ABSOLUTE_IY; (yyval.mode).param = (yyvsp[-3].i); }
#line 3981 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 271:
#line 914 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_IMPLIED; }
#line 3987 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 272:
#line 915 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_ACCUMULATOR; }
#line 3993 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 273:
#line 916 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_B; }
#line 3999 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 274:
#line 917 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_C; }
#line 4005 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 275:
#line 918 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_D; }
#line 4011 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 276:
#line 919 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_E; }
#line 4017 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 277:
#line 920 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_H; }
#line 4023 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 278:
#line 921 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXH; }
#line 4029 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 279:
#line 922 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYH; }
#line 4035 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 280:
#line 923 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_L; }
#line 4041 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 281:
#line 924 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IXL; }
#line 4047 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 282:
#line 925 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IYL; }
#line 4053 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 283:
#line 926 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_AF; }
#line 4059 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 284:
#line 927 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_BC; }
#line 4065 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 285:
#line 928 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_DE; }
#line 4071 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 286:
#line 929 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_HL; }
#line 4077 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 287:
#line 930 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IX; }
#line 4083 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 288:
#line 931 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_IY; }
#line 4089 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 289:
#line 932 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_REG_SP; }
#line 4095 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 290:
#line 934 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.mode).addr_mode = ASM_ADDR_MODE_DIRECT; (yyval.mode).param = (yyvsp[0].i); }
#line 4101 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 291:
#line 935 "mon_parse.y" /* yacc.c:1651  */
    {    /* Clash with addr,x addr,y addr,s modes! */
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        if ((yyvsp[-2].i) >= -16 && (yyvsp[-2].i) < 16) {
            (yyval.mode).addr_submode = (3 << 5) | ((yyvsp[-2].i) & 0x1F);
        } else if ((yyvsp[-2].i) >= -128 && (yyvsp[-2].i) < 128) {
            (yyval.mode).addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF8;
            (yyval.mode).param = (yyvsp[-2].i);
        } else if ((yyvsp[-2].i) >= -32768 && (yyvsp[-2].i) < 32768) {
            (yyval.mode).addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF16;
            (yyval.mode).param = (yyvsp[-2].i);
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
#line 4121 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 292:
#line 950 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
#line 4130 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 293:
#line 954 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-2].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
#line 4139 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 294:
#line 958 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[0].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
#line 4148 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 295:
#line 962 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[0].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
#line 4157 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 296:
#line 966 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[0].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
#line 4166 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 297:
#line 970 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
#line 4175 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 298:
#line 974 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
#line 4184 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 299:
#line 978 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
#line 4193 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 300:
#line 982 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).param = (yyvsp[-2].i);
        if ((yyvsp[-2].i) >= -128 && (yyvsp[-2].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8;
        } else if ((yyvsp[-2].i) >= -32768 && (yyvsp[-2].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16;
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
#line 4210 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 301:
#line 994 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        if ((yyvsp[-3].i) >= -16 && (yyvsp[-3].i) < 16) {
            (yyval.mode).addr_submode = (yyvsp[-3].i) & 0x1F;
        } else if ((yyvsp[-4].i) >= -128 && (yyvsp[-4].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFF8;
            (yyval.mode).param = (yyvsp[-3].i);
        } else if ((yyvsp[-3].i) >= -32768 && (yyvsp[-3].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFF16;
            (yyval.mode).param = (yyvsp[-3].i);
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
#line 4230 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 302:
#line 1009 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-2].i) | ASM_ADDR_MODE_INDEXED_INC1;
        }
#line 4239 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 303:
#line 1013 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-3].i) | ASM_ADDR_MODE_INDEXED_INC2;
        }
#line 4248 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 304:
#line 1017 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_DEC1;
        }
#line 4257 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 305:
#line 1021 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_DEC2;
        }
#line 4266 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 306:
#line 1025 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-1].i) | ASM_ADDR_MODE_INDEXED_OFF0;
        }
#line 4275 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 307:
#line 1029 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-2].i) | ASM_ADDR_MODE_INDEXED_OFFB;
        }
#line 4284 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 308:
#line 1033 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-2].i) | ASM_ADDR_MODE_INDEXED_OFFA;
        }
#line 4293 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 309:
#line 1037 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | (yyvsp[-2].i) | ASM_ADDR_MODE_INDEXED_OFFD;
        }
#line 4302 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 310:
#line 1041 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).param = (yyvsp[-3].i);
        if ((yyvsp[-3].i) >= -128 && (yyvsp[-3].i) < 128) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8_IND;
        } else if ((yyvsp[-3].i) >= -32768 && (yyvsp[-3].i) < 32768) {
            (yyval.mode).addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16_IND;
        } else {
            (yyval.mode).addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
#line 4319 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 311:
#line 1053 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDEXED;
        (yyval.mode).addr_submode = 0x80 | ASM_ADDR_MODE_EXTENDED_INDIRECT;
        (yyval.mode).param = (yyvsp[-1].i);
        }
#line 4329 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 312:
#line 1058 "mon_parse.y" /* yacc.c:1651  */
    {
        (yyval.mode).addr_mode = ASM_ADDR_MODE_INDIRECT_LONG_Y;
        (yyval.mode).param = (yyvsp[-3].i);
        }
#line 4338 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 313:
#line 1066 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (0 << 5); printf("reg_x\n"); }
#line 4344 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 314:
#line 1067 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (1 << 5); printf("reg_y\n"); }
#line 4350 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 315:
#line 1068 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (yyvsp[0].i); }
#line 4356 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 316:
#line 1069 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (3 << 5); printf("reg_s\n"); }
#line 4362 "mon_parse.c" /* yacc.c:1651  */
    break;

  case 317:
#line 1073 "mon_parse.y" /* yacc.c:1651  */
    { (yyval.i) = (2 << 5); printf("reg_u\n"); }
#line 4368 "mon_parse.c" /* yacc.c:1651  */
    break;


#line 4372 "mon_parse.c" /* yacc.c:1651  */
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

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
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

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

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

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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

#if !defined yyoverflow || YYERROR_VERBOSE
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
  /* Do not reclaim the symbols of the rule whose action triggered
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
  return yyresult;
}
#line 1077 "mon_parse.y" /* yacc.c:1910  */


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
   mon_clear_buffer();
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
         case ERR_INCOMPLETE_COND_OP:
           mon_out("Conditional operation missing an operand:\n");
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
#if 0
   fprintf(stderr, "ERR:%s\n", s);
#endif
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
