/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
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

/* Line 2068 of yacc.c  */
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



/* Line 2068 of yacc.c  */
#line 387 "mon_parse.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


