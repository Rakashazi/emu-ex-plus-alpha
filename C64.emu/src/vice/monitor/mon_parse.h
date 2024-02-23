#ifndef _yy_defines_h_
#define _yy_defines_h_

#define H_NUMBER 257
#define D_NUMBER 258
#define O_NUMBER 259
#define B_NUMBER 260
#define CONVERT_OP 261
#define B_DATA 262
#define H_RANGE_GUESS 263
#define D_NUMBER_GUESS 264
#define O_NUMBER_GUESS 265
#define B_NUMBER_GUESS 266
#define BAD_CMD 267
#define MEM_OP 268
#define IF 269
#define MEM_COMP 270
#define MEM_DISK8 271
#define MEM_DISK9 272
#define MEM_DISK10 273
#define MEM_DISK11 274
#define EQUALS 275
#define TRAIL 276
#define CMD_SEP 277
#define LABEL_ASGN_COMMENT 278
#define CMD_LOG 279
#define CMD_LOGNAME 280
#define CMD_SIDEFX 281
#define CMD_DUMMY 282
#define CMD_RETURN 283
#define CMD_BLOCK_READ 284
#define CMD_BLOCK_WRITE 285
#define CMD_UP 286
#define CMD_DOWN 287
#define CMD_LOAD 288
#define CMD_BASICLOAD 289
#define CMD_SAVE 290
#define CMD_VERIFY 291
#define CMD_BVERIFY 292
#define CMD_IGNORE 293
#define CMD_HUNT 294
#define CMD_FILL 295
#define CMD_MOVE 296
#define CMD_GOTO 297
#define CMD_REGISTERS 298
#define CMD_READSPACE 299
#define CMD_WRITESPACE 300
#define CMD_RADIX 301
#define CMD_MEM_DISPLAY 302
#define CMD_BREAK 303
#define CMD_TRACE 304
#define CMD_IO 305
#define CMD_BRMON 306
#define CMD_COMPARE 307
#define CMD_DUMP 308
#define CMD_UNDUMP 309
#define CMD_EXIT 310
#define CMD_DELETE 311
#define CMD_CONDITION 312
#define CMD_COMMAND 313
#define CMD_ASSEMBLE 314
#define CMD_DISASSEMBLE 315
#define CMD_NEXT 316
#define CMD_STEP 317
#define CMD_PRINT 318
#define CMD_DEVICE 319
#define CMD_HELP 320
#define CMD_WATCH 321
#define CMD_DISK 322
#define CMD_QUIT 323
#define CMD_CHDIR 324
#define CMD_BANK 325
#define CMD_LOAD_LABELS 326
#define CMD_SAVE_LABELS 327
#define CMD_ADD_LABEL 328
#define CMD_DEL_LABEL 329
#define CMD_SHOW_LABELS 330
#define CMD_CLEAR_LABELS 331
#define CMD_RECORD 332
#define CMD_MON_STOP 333
#define CMD_PLAYBACK 334
#define CMD_CHAR_DISPLAY 335
#define CMD_SPRITE_DISPLAY 336
#define CMD_TEXT_DISPLAY 337
#define CMD_SCREENCODE_DISPLAY 338
#define CMD_ENTER_DATA 339
#define CMD_ENTER_BIN_DATA 340
#define CMD_KEYBUF 341
#define CMD_BLOAD 342
#define CMD_BSAVE 343
#define CMD_SCREEN 344
#define CMD_UNTIL 345
#define CMD_CPU 346
#define CMD_YYDEBUG 347
#define CMD_BACKTRACE 348
#define CMD_SCREENSHOT 349
#define CMD_PWD 350
#define CMD_DIR 351
#define CMD_MKDIR 352
#define CMD_RMDIR 353
#define CMD_RESOURCE_GET 354
#define CMD_RESOURCE_SET 355
#define CMD_LOAD_RESOURCES 356
#define CMD_SAVE_RESOURCES 357
#define CMD_ATTACH 358
#define CMD_DETACH 359
#define CMD_MON_RESET 360
#define CMD_TAPECTRL 361
#define CMD_CARTFREEZE 362
#define CMD_UPDB 363
#define CMD_JPDB 364
#define CMD_CPUHISTORY 365
#define CMD_MEMMAPZAP 366
#define CMD_MEMMAPSHOW 367
#define CMD_MEMMAPSAVE 368
#define CMD_COMMENT 369
#define CMD_LIST 370
#define CMD_STOPWATCH 371
#define RESET 372
#define CMD_EXPORT 373
#define CMD_AUTOSTART 374
#define CMD_AUTOLOAD 375
#define CMD_MAINCPU_TRACE 376
#define CMD_WARP 377
#define CMD_PROFILE 378
#define FLAT 379
#define GRAPH 380
#define FUNC 381
#define DEPTH 382
#define DISASS 383
#define PROFILE_CONTEXT 384
#define CLEAR 385
#define CMD_LABEL_ASGN 386
#define L_PAREN 387
#define R_PAREN 388
#define ARG_IMMEDIATE 389
#define REG_A 390
#define REG_X 391
#define REG_Y 392
#define COMMA 393
#define INST_SEP 394
#define L_BRACKET 395
#define R_BRACKET 396
#define LESS_THAN 397
#define REG_U 398
#define REG_S 399
#define REG_PC 400
#define REG_PCR 401
#define REG_B 402
#define REG_C 403
#define REG_D 404
#define REG_E 405
#define REG_H 406
#define REG_L 407
#define REG_AF 408
#define REG_BC 409
#define REG_DE 410
#define REG_HL 411
#define REG_IX 412
#define REG_IY 413
#define REG_SP 414
#define REG_IXH 415
#define REG_IXL 416
#define REG_IYH 417
#define REG_IYL 418
#define PLUS 419
#define MINUS 420
#define STRING 421
#define FILENAME 422
#define R_O_L 423
#define R_O_L_Q 424
#define OPCODE 425
#define LABEL 426
#define BANKNAME 427
#define CPUTYPE 428
#define MON_REGISTER 429
#define COND_OP 430
#define RADIX_TYPE 431
#define INPUT_SPEC 432
#define CMD_CHECKPT_ON 433
#define CMD_CHECKPT_OFF 434
#define TOGGLE 435
#define MASK 436
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union YYSTYPE {
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
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
extern YYSTYPE yylval;

#endif /* _yy_defines_h_ */
