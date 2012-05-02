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
.global Gp2x_ClearBuffer
.global myuname
.extern current_pc_pal
.extern current_fix
.extern dda_y_skip_i
.extern dda_x_skip_i
	
	.align 4
	
full_y_skip :
	.byte 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1

	
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
	
	ldr r3, = current_pc_pal
	ldr r3,[r3]
	add r3,r3,r1, lsl #6 	;@ r3=pal

	ldr r4, = current_fix
	ldr r4,[r4]
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

       ldr r3, = current_pc_pal
       ldr r3,[r3]
       add r3,r3,r1, lsl #6    ;@ r3=pal

	ldr r4, = mem_gfx
	ldr r4,[r4]
	add r4,r4,r0, lsl #7    ;@ r4=gfx
                               ;@ r2=bmp

       ldr r7, = ldda_y_skip
       ldr r7,[r7]

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

       ldr r3, = current_pc_pal
       ldr r3,[r3]
       add r3,r3,r1, lsl #6    ;@ r3=pal

       ldr r4, = mem_gfx
       ldr r4,[r4]
       add r4,r4,r0, lsl #7    ;@ r4=gfx
                               ;@ r2=bmp

       ldr r7, = ldda_y_skip
       ldr r7,[r7]

	
.lineloop_\name:

	ldr r9, = dda_x_skip_i
	ldr r9,[r9]

       ldrb r5,[r7], #1
       add r4,r4,r5, lsl #3

       DRAW_LINE_\name

       @add r7,r7,#1
       subs r8, r8, #1
       bne .lineloop_\name

.end_\name:

       ldmia sp!,{r4-r10}
       mov pc,lr               ;@ return
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

	@ r0=buffer r1=color
Gp2x_ClearBuffer:
  stmfd sp!, {r4-r10}  @ remember registers 4-10
  mov r2, #4928        @ we will run the loop 4800 times to copy the screen
        mov r3, r1           @ load up the registers with zeros
        mov r4, r1
        mov r5, r1
        mov r6, r1
        mov r7, r1
        mov r8, r1
        mov r9, r1
        mov r10, r1
.ClearScreenLoop:
  stmia r0!, {r3-r10}  @ write the 32 bytes of zeros to the destination
  subs r2, r2, #1      @ decrement the loop counter
  bne .ClearScreenLoop  @ if we're not done, do it again
  ldmfd sp!, {r4-r10}  @ restore the registers
        mov pc, lr           @ return

myuname:
	swi #0x90007a
	mov pc, lr

.global spend_cycles @ c

spend_cycles:
    mov     r0, r0, lsr #2  @ 4 cycles/iteration
    sub     r0, r0, #2      @ entry/exit/init
.sc_loop:
    subs    r0, r0, #1
    bpl     .sc_loop

    bx      lr
