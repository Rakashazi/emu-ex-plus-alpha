/* A Bison parser, made by GNU Bison 3.1.  */

/* Bison interface for Yacc-like parsers in C

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
#line 135 "mon_parse.y" /* yacc.c:1913  */

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

#line 397 "mon_parse.h" /* yacc.c:1913  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MON_PARSE_H_INCLUDED  */
