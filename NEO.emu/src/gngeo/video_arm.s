	;@ sprite drawing

.global draw_one_char_arm 	;@ (int byte1,int byte2,unsigned short *br)
.global draw_tile_arm_norm
.global draw_tile_arm_xyflip_norm
.global draw_tile_arm_xflip_norm
.global draw_tile_arm_yflip_norm
.global draw_tile_arm_xzoom
.global draw_tile_arm_xyflip_xzoom
.global draw_tile_arm_xflip_xzoom
.global draw_tile_arm_yflip_xzoom

.macro LOAD_GLOBAL_VAR reg prefix name
	ldr \reg, .L\prefix\()_addr_\name
.L\prefix\()_offset_\name :
	ldr \reg, [pc, \reg]
	ldr \reg, [\reg]
.endm

.macro GLOBAL_VAR_LOADER prefix name
.L\prefix\()_addr_\name :
.long \name(GOT_PREL)-((.L\prefix\()_offset_\name+8)-.L\prefix\()_addr_\name)
.endm

.macro DRAW_LINE
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #2]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #4]
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #6]

	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #8]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #10]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #12]
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #14]
	add r2,r2,#704
.endm

	.ltorg	
	.align 4
draw_one_char_arm:
	stmdb sp!,{r4-r6}
	
	LOAD_GLOBAL_VAR r3 draw_one_char_arm current_pc_pal
	add r3,r3,r1, lsl #6 	;@ r3=pal

	LOAD_GLOBAL_VAR r4 draw_one_char_arm current_fix
	add r4,r4,r0, lsl #5       ;@ r4=gfx

	mov r5,#0xF		
	
	ldr r0,[r4]
	DRAW_LINE
	ldr r0,[r4, #4]
	DRAW_LINE
	ldr r0,[r4, #8]
	DRAW_LINE
	ldr r0,[r4, #12]
	DRAW_LINE
	ldr r0,[r4, #16]
	DRAW_LINE
	ldr r0,[r4, #20]
	DRAW_LINE
	ldr r0,[r4, #24]
	DRAW_LINE
	ldr r0,[r4, #28]
	DRAW_LINE

	ldmia sp!,{r4-r6}	
	mov pc,lr 		;@ return
GLOBAL_VAR_LOADER draw_one_char_arm current_pc_pal
GLOBAL_VAR_LOADER draw_one_char_arm current_fix

.macro DRAW_SPRITE_LINE_FULL
       mov r5,#0xF
       ldr r0,[r4]
       and r1,r5,r0, lsr #24
       and r9,r5,r0, lsr #28
       and r6,r5,r0, lsr #16
       and r10,r5,r0, lsr #20
       ldr r1,[r3, r1, lsl #2]
       ldr r9,[r3, r9, lsl #2]
       ldr r6,[r3, r6, lsl #2]
       ldr r10,[r3, r10, lsl #2]
       orr r9,r9,r1,lsl #16
       orr r10,r10,r6,lsl #16

       and r1,r5,r0, lsr #8
       and r11,r5,r0, lsr #12
       and r6,r5,r0
       and r12,r5,r0, lsr #4
       ldr r1,[r3, r1, lsl #2]
       ldr r11,[r3, r11, lsl #2]
       ldr r6,[r3, r6, lsl #2]
       ldr r12,[r3, r12, lsl #2]
       orr r11,r11,r1,lsl #16
       orr r12,r12,r6,lsl #16
       stmia r2,{r9-r12}

       ldr r0,[r4, #4]
       and r1,r5,r0, lsr #24
       and r9,r5,r0, lsr #28
       and r6,r5,r0, lsr #16
       and r10,r5,r0, lsr #20
       ldr r1,[r3, r1, lsl #2]
       ldr r9,[r3, r9, lsl #2]
       ldr r6,[r3, r6, lsl #2]
       ldr r10,[r3, r10, lsl #2]
       orr r9,r9,r1,lsl #16
       orr r10,r10,r6,lsl #16

	add r2,r2, #16
	
       and r1,r5,r0, lsr #8
       and r11,r5,r0, lsr #12
       and r6,r5,r0
       and r12,r5,r0, lsr #4
       ldr r1,[r3, r1, lsl #2]
       ldr r11,[r3, r11, lsl #2]
       ldr r6,[r3, r6, lsl #2]
       ldr r12,[r3, r12, lsl #2]
       orr r11,r11,r1,lsl #16
       orr r12,r12,r6,lsl #16
       stmia r2,{r9-r12} 

	sub r2,r2, #16
.endm
	
	;;@ Draw a line of a sprite.
.macro DRAW_SPRITE_LINE
	mov r5,#0xF
	ldr r0,[r4]
	@cmp r0,#0
	@beq 1f
	
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #2]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #4]
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #6]

	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #8]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #10]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #12]
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #14]

@1:		
	ldr r0,[r4, #4]
	@cmp r0,#0
	@beq 2f

	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #16]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #18]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #20]
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #22]
 
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #24]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #26]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #28]
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #30]
@2:	
.endm

.macro DRAW_SPRITE_LINE_WIP
	mov r5,#0xF
	ldr r0,[r4]
	cmp r0,#0
	beq 1f
	
	ands r1,r5,r0, lsr #28
	beq 3f
	ands r9,r5,r0, lsr #24
	ldr r1,[r3, r1, lsl #2]
	ldrne r6,[r3, r9, lsl #2]
	streqh r1,[r2]
	orrne r1,r1,r6, lsl #16
	strne r1,[r2]
	bne 4f
3:
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #2]
4:	

	ands r1,r5,r0, lsr #20
	beq 5f
	ands r9,r5,r0, lsr #16
	ldr r1,[r3, r1, lsl #2]
	ldrne r6,[r3, r9, lsl #2]
	streqh r1,[r2,#2]
	orrne r1,r1,r6, lsl #16
	strne r1,[r2, #4]
	bne 6f
5:
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #6]
6:	

	
	@ands r1,r5,r0, lsr #20
	@ldrne r6,[r3, r1, lsl #2]
	@strneh r6,[r2, #4]
	@ands r1,r5,r0, lsr #16
	@ldrne r6,[r3, r1, lsl #2]
	@strneh r6,[r2, #6]

	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #8]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #10]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #12]
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #14]

1:		
	ldr r0,[r4, #4]
	cmp r0,#0
	beq 2f

	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #16]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #18]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #20]
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #22]
 
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #24]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #26]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #28]
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #30]
2:	
.endm
	
	;;@ Draw a line of a sprite xflipped.	
.macro DRAW_SPRITE_LINE_XFLIP
	mov r5,#0xF
	ldr r0,[r4,#4]
	@cmp r0,#0
	@beq 1f
	
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #2]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #4]
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #6]

	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #8]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #10]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #12]
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #14]

@1:	
	ldr r0,[r4]
	@cmp r0,#0
	@beq 2f
	
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #16]
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #18]
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #20]
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #22]
 
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #24]
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #26]
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #28]
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, #30]
2:	
.endm

.macro DRAW_LINE_XFLIP
       DRAW_SPRITE_LINE_XFLIP
       add r2,r2,#704
	@add r2,r2,#32
.endm
.macro DRAW_LINE_XYFLIP
       DRAW_SPRITE_LINE_XFLIP
       sub r2,r2,#704
	@sub r2,r2,#32
.endm
.macro DRAW_LINE_YFLIP
       DRAW_SPRITE_LINE
       sub r2,r2,#704
	@sub r2,r2,#32
.endm
.macro DRAW_LINE_NORM
       DRAW_SPRITE_LINE
       add r2,r2,#704
	@add r2,r2,#32
.endm

.macro DRAW_TILE name field
       stmdb sp!,{r4-r8}

       movs r8,r3               ;@ r8=zy
       beq .end_\name

       LOAD_GLOBAL_VAR r3 \name current_pc_pal
       add r3,r3,r1, lsl #6    ;@ r3=pal

	LOAD_GLOBAL_VAR r4 \name mem_gfx
	add r4,r4,r0, lsl #7    ;@ r4=gfx
                               ;@ r2=bmp

       LOAD_GLOBAL_VAR r7 \name ldda_y_skip

.lineloop_\name:

       ldrb r5,[r7], #1
       add r4,r4,r5, lsl #3
	@add r4,r4,r5, lsl #4

       DRAW_LINE_\name

       @add r7,r7,#1
       subs r8, r8, #1
       @subnes r8, r8, #1
       bne .lineloop_\name

.end_\name:

       ldmia sp!,{r4-r8}
       mov pc,lr               ;@ return
GLOBAL_VAR_LOADER \name current_pc_pal
GLOBAL_VAR_LOADER \name mem_gfx
GLOBAL_VAR_LOADER \name ldda_y_skip
.endm

       .align 4
draw_tile_arm_xyflip_norm:
	DRAW_TILE XYFLIP 0
	
	.align 4
draw_tile_arm_xflip_norm:
	DRAW_TILE XFLIP 0

	.align 4
draw_tile_arm_yflip_norm:
	DRAW_TILE YFLIP 0

	.align 4	
draw_tile_arm_norm:
	DRAW_TILE NORM 0	


	;;@ Xzoom draw
				;@ r9 == xskip
.macro DRAW_SPRITE_LINE_XZOOM
	mov r5,#0xF
	ldr r0,[r4]
	mov r10,#0
	
	movs r9,r9, lsr #1
	bcc 1f
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	ldr r0,[r4, #4]

	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
 	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
1:	
.endm
		
.macro DRAW_SPRITE_LINE_XFLIP_XZOOM
	mov r5,#0xF
	ldr r0,[r4,#4]
	mov r10,#0
	
	movs r9,r9, lsr #1
	bcc 1f
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	ldr r0,[r4]
	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #4
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #8
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #12
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
 	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f	
	ands r1,r5,r0, lsr #16
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #20
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #24
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
	add r10,r10,#2
1:	
	movs r9,r9, lsr #1
	bcc 1f		
	ands r1,r5,r0, lsr #28
	ldrne r6,[r3, r1, lsl #2]
	strneh r6,[r2, r10]
1:		
.endm

.macro DRAW_LINE_XZ_XFLIP
       DRAW_SPRITE_LINE_XFLIP_XZOOM
       add r2,r2,#704
.endm
.macro DRAW_LINE_XZ_XYFLIP
       DRAW_SPRITE_LINE_XFLIP_XZOOM
       sub r2,r2,#704
.endm
.macro DRAW_LINE_XZ_YFLIP
       DRAW_SPRITE_LINE_XZOOM
       sub r2,r2,#704
.endm
.macro DRAW_LINE_XZ_NORM
       DRAW_SPRITE_LINE_XZOOM
       add r2,r2,#704
.endm

.macro DRAW_TILE_XZOOM name field
       stmdb sp!,{r4-r10}

       movs r8,r3               ;@ r8=zy
       beq .end_\name

       LOAD_GLOBAL_VAR r3 \name current_pc_pal
       add r3,r3,r1, lsl #6    ;@ r3=pal

       LOAD_GLOBAL_VAR r4 \name mem_gfx
       add r4,r4,r0, lsl #7    ;@ r4=gfx
                               ;@ r2=bmp

       LOAD_GLOBAL_VAR r7 \name ldda_y_skip

	
.lineloop_\name:

	LOAD_GLOBAL_VAR r9 \name dda_x_skip_i

       ldrb r5,[r7], #1
       add r4,r4,r5, lsl #3

       DRAW_LINE_\name

       @add r7,r7,#1
       subs r8, r8, #1
       bne .lineloop_\name

.end_\name:

       ldmia sp!,{r4-r10}
       mov pc,lr               ;@ return
GLOBAL_VAR_LOADER \name current_pc_pal
GLOBAL_VAR_LOADER \name mem_gfx
GLOBAL_VAR_LOADER \name ldda_y_skip
GLOBAL_VAR_LOADER \name dda_x_skip_i
.endm

       .align 4
draw_tile_arm_xyflip_xzoom:
	DRAW_TILE_XZOOM XZ_XYFLIP 0
	
	.align 4
draw_tile_arm_xflip_xzoom:
	DRAW_TILE_XZOOM XZ_XFLIP 0

	.align 4
draw_tile_arm_yflip_xzoom:
	DRAW_TILE_XZOOM XZ_YFLIP 0

	.align 4	
draw_tile_arm_xzoom:
	DRAW_TILE_XZOOM XZ_NORM 0	
