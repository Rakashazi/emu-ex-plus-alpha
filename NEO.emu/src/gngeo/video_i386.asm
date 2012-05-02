	;; video.asm :	asm video function
SECTION .text

	;; nasm macro
	%include 'c32.mac'

	;; global definition
	global draw_tile_i386
	global draw_tile_i386_50
	global draw_one_char_i386
	global draw_scanline_tile_i386_norm
	global draw_scanline_tile_i386_50
	
	;; extern definition
	extern dda_y_skip	; y zoom table
	extern dda_x_skip	; x zoom table
	extern current_pc_pal	; palette
	extern mem_gfx		; &memory.gfx
	extern mem_video	; &memory.vid.ram
	extern current_fix
	extern fix_usage


	;; macro/constant/variable definiton
	align 2
full_y_skip :	db 	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
	
	%define BUFX 352
	%define BUFY 256

	%define BUFX_2 704
	%define BUFX_2_MINUS_32 672
	%define BUFX_2_PLUS_32  736
	%define	BUFX_2_MINUS_16 688


	%include 'video_i386.inc'

	draw_tile_i386 norm
	draw_tile_i386 50
	draw_scanline_tile_i386 norm
	draw_scanline_tile_i386 50


;;; draw one char
	align 2
	%macro draw_char_line 1
		mov eax,[ecx + %1]
		draw_pixel_norm 0,0
		draw_pixel_norm 4,0
		draw_pixel_norm 8,0
		draw_pixel_norm 12,0
		draw_pixel_norm 16,0
		draw_pixel_norm 20,0
		draw_pixel_norm 24,0
		draw_pixel_norm 28,0
		add edi,BUFX_2_MINUS_16
	%endmacro
	
	proc draw_one_char_i386
	%$byte1   arg
	%$byte2   arg
	%$dest    arg	; destination buffer 352x256x16
	
	pusha

	mov edi,[ebp + %$dest]	; esi=pointer on dest buffer
	
	mov ebx,[current_pc_pal]; pointer on char pal
	mov eax,[ebp + %$byte2]
	shl eax,6
	add ebx,eax

	mov ecx,[current_fix]	;  pointer on char data
	mov eax,[ebp + %$byte1]
	shl eax,5
	add ecx,eax
	
	draw_char_line 0
	draw_char_line 4
	draw_char_line 8
	draw_char_line 12
	draw_char_line 16
	draw_char_line 20
	draw_char_line 24
	draw_char_line 28
	
	popa
	endproc
