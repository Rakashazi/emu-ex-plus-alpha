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
#define CMD_SAVE 289
#define CMD_VERIFY 290
#define CMD_BVERIFY 291
#define CMD_IGNORE 292
#define CMD_HUNT 293
#define CMD_FILL 294
#define CMD_MOVE 295
#define CMD_GOTO 296
#define CMD_REGISTERS 297
#define CMD_READSPACE 298
#define CMD_WRITESPACE 299
#define CMD_RADIX 300
#define CMD_MEM_DISPLAY 301
#define CMD_BREAK 302
#define CMD_TRACE 303
#define CMD_IO 304
#define CMD_BRMON 305
#define CMD_COMPARE 306
#define CMD_DUMP 307
#define CMD_UNDUMP 308
#define CMD_EXIT 309
#define CMD_DELETE 310
#define CMD_CONDITION 311
#define CMD_COMMAND 312
#define CMD_ASSEMBLE 313
#define CMD_DISASSEMBLE 314
#define CMD_NEXT 315
#define CMD_STEP 316
#define CMD_PRINT 317
#define CMD_DEVICE 318
#define CMD_HELP 319
#define CMD_WATCH 320
#define CMD_DISK 321
#define CMD_QUIT 322
#define CMD_CHDIR 323
#define CMD_BANK 324
#define CMD_LOAD_LABELS 325
#define CMD_SAVE_LABELS 326
#define CMD_ADD_LABEL 327
#define CMD_DEL_LABEL 328
#define CMD_SHOW_LABELS 329
#define CMD_CLEAR_LABELS 330
#define CMD_RECORD 331
#define CMD_MON_STOP 332
#define CMD_PLAYBACK 333
#define CMD_CHAR_DISPLAY 334
#define CMD_SPRITE_DISPLAY 335
#define CMD_TEXT_DISPLAY 336
#define CMD_SCREENCODE_DISPLAY 337
#define CMD_ENTER_DATA 338
#define CMD_ENTER_BIN_DATA 339
#define CMD_KEYBUF 340
#define CMD_BLOAD 341
#define CMD_BSAVE 342
#define CMD_SCREEN 343
#define CMD_UNTIL 344
#define CMD_CPU 345
#define CMD_YYDEBUG 346
#define CMD_BACKTRACE 347
#define CMD_SCREENSHOT 348
#define CMD_PWD 349
#define CMD_DIR 350
#define CMD_MKDIR 351
#define CMD_RMDIR 352
#define CMD_RESOURCE_GET 353
#define CMD_RESOURCE_SET 354
#define CMD_LOAD_RESOURCES 355
#define CMD_SAVE_RESOURCES 356
#define CMD_ATTACH 357
#define CMD_DETACH 358
#define CMD_MON_RESET 359
#define CMD_TAPECTRL 360
#define CMD_CARTFREEZE 361
#define CMD_UPDB 362
#define CMD_JPDB 363
#define CMD_CPUHISTORY 364
#define CMD_MEMMAPZAP 365
#define CMD_MEMMAPSHOW 366
#define CMD_MEMMAPSAVE 367
#define CMD_COMMENT 368
#define CMD_LIST 369
#define CMD_STOPWATCH 370
#define RESET 371
#define CMD_EXPORT 372
#define CMD_AUTOSTART 373
#define CMD_AUTOLOAD 374
#define CMD_MAINCPU_TRACE 375
#define CMD_WARP 376
#define CMD_LABEL_ASGN 377
#define L_PAREN 378
#define R_PAREN 379
#define ARG_IMMEDIATE 380
#define REG_A 381
#define REG_X 382
#define REG_Y 383
#define COMMA 384
#define INST_SEP 385
#define L_BRACKET 386
#define R_BRACKET 387
#define LESS_THAN 388
#define REG_U 389
#define REG_S 390
#define REG_PC 391
#define REG_PCR 392
#define REG_B 393
#define REG_C 394
#define REG_D 395
#define REG_E 396
#define REG_H 397
#define REG_L 398
#define REG_AF 399
#define REG_BC 400
#define REG_DE 401
#define REG_HL 402
#define REG_IX 403
#define REG_IY 404
#define REG_SP 405
#define REG_IXH 406
#define REG_IXL 407
#define REG_IYH 408
#define REG_IYL 409
#define PLUS 410
#define MINUS 411
#define STRING 412
#define FILENAME 413
#define R_O_L 414
#define R_O_L_Q 415
#define OPCODE 416
#define LABEL 417
#define BANKNAME 418
#define CPUTYPE 419
#define MON_REGISTER 420
#define COND_OP 421
#define RADIX_TYPE 422
#define INPUT_SPEC 423
#define CMD_CHECKPT_ON 424
#define CMD_CHECKPT_OFF 425
#define TOGGLE 426
#define MASK 427
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
