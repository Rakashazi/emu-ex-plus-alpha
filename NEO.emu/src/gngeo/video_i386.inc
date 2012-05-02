	
	%macro draw_pixel_norm 2
		mov edx,eax
		shr edx,%1
		and edx,0x0000000F
		jz %%end
		mov dx,[edx*4+ebx]
		mov word [edi],dx
align 2
%%end:
		add edi,2
	%endmacro

	%macro draw_pixel_50 2
		mov edx,eax
		shr edx,%1
		and edx,0x0000000F
		jz %%end
		mov dx,[edx*4+ebx]
		and word [edi],0xf7de
		and dx,0xf7de
		shr word [edi],1
		shr dx,1
		add word [edi],dx
align 2
%%end:
		add edi,2
	%endmacro

	%macro draw_pixel_xzoom_norm 2
		;; x zoom processing
		mov edx,%2
		add edx,[dda_x_skip]
		mov dl,byte [edx]
		cmp dl,0
		jz %%zoom_end

		draw_pixel_norm %1,0
align 2
%%zoom_end:
	%endmacro

	%macro draw_pixel_xzoom_50 2
		;; x zoom processing
		mov edx,%2
		add edx,[dda_x_skip]
		mov dl,byte [edx]
		cmp dl,0
		jz %%zoom_end

		draw_pixel_50 %1,0
align 2
%%zoom_end:
	%endmacro

	%macro inc_edi_std_norm 0
		add edi,BUFX_2_MINUS_32
	%endmacro
	%macro inc_edi_std_xzoom_norm 0
		add edi,BUFX_2
		sub edi,[ebp + %$zx]
		sub edi,[ebp + %$zx]
	%endmacro

	%macro inc_edi_yflip_norm 0
		sub edi,BUFX_2_PLUS_32
	%endmacro
	%macro inc_edi_yflip_xzoom_norm 0
		sub edi,BUFX_2
		sub edi,[ebp + %$zx]
		sub edi,[ebp + %$zx]
	%endmacro

	%macro inc_edi_std_50 0
		inc_edi_std_norm
	%endmacro
	%macro inc_edi_std_xzoom_50 0
		inc_edi_std_xzoom_norm
	%endmacro

	%macro inc_edi_yflip_50 0
		inc_edi_yflip_norm
	%endmacro
	%macro inc_edi_yflip_xzoom_50 0
		inc_edi_yflip_xzoom_norm
	%endmacro

	%macro draw_line_std 1
		mov eax,[ebp + %$zy]
		mov esi,full_y_skip
		cmp eax,16
		jz %%full_zoom
		mov esi,dda_y_skip
align 2
%%full_zoom:

		push edx
		neg edx
		add edx,eax
		xor eax,eax
		mov al, byte [esi + edx]	; 1 ou 0
		shl eax,3
		add ecx,eax
	
		mov eax,[ecx]
		draw_pixel_%1 28,0
		draw_pixel_%1 24,1
		draw_pixel_%1 20,2
		draw_pixel_%1 16,3
		draw_pixel_%1 12,4
		draw_pixel_%1 8,5
		draw_pixel_%1 4,6
		draw_pixel_%1 0,7

		mov eax,[ecx+4]
		draw_pixel_%1 28,8
		draw_pixel_%1 24,9
		draw_pixel_%1 20,10
		draw_pixel_%1 16,11
		draw_pixel_%1 12,12
		draw_pixel_%1 8,13
		draw_pixel_%1 4,14
		draw_pixel_%1 0,15
	;;		add edi,BUFX_2_MINUS_32
		inc_edi_std_%1
		pop edx
	%endmacro


	%macro draw_line_yflip 1
		mov eax,[ebp + %$zy]
		mov esi,full_y_skip
		cmp eax,16
		jz %%full_zoom
		mov esi,dda_y_skip
align 2
%%full_zoom:
		push edx
		neg edx
		add edx,eax
		xor eax,eax
		mov al, byte [esi + edx]	; 1 ou 0
		shl eax,3
		add ecx,eax
	
		mov eax,[ecx]
		draw_pixel_%1 28,0
		draw_pixel_%1 24,1
		draw_pixel_%1 20,2
		draw_pixel_%1 16,3
		draw_pixel_%1 12,4
		draw_pixel_%1 8,5
		draw_pixel_%1 4,6
		draw_pixel_%1 0,7

		mov eax,[ecx+4]
		draw_pixel_%1 28,8
		draw_pixel_%1 24,9
		draw_pixel_%1 20,10
		draw_pixel_%1 16,11
		draw_pixel_%1 12,12
		draw_pixel_%1 8,13
		draw_pixel_%1 4,14
		draw_pixel_%1 0,15
		
	;;		sub edi,BUFX_2_PLUS_32
		inc_edi_yflip_%1
		pop edx
	%endmacro

	%macro draw_line_xflip 1
		mov eax,[ebp + %$zy]
		mov esi,full_y_skip
		cmp eax,16
		jz %%full_zoom
		mov esi,dda_y_skip
align 2
%%full_zoom:
		push edx
		neg edx
		add edx,eax
		xor eax,eax
		mov al, byte [esi + edx]	; 1 ou 0
		shl eax,3
		add ecx,eax
	
		mov eax,[ecx+4]
		draw_pixel_%1 0,0
		draw_pixel_%1 4,1
		draw_pixel_%1 8,2
		draw_pixel_%1 12,3
		draw_pixel_%1 16,4
		draw_pixel_%1 20,5
		draw_pixel_%1 24,6
		draw_pixel_%1 28,7

		mov eax,[ecx]
		draw_pixel_%1 0,8
		draw_pixel_%1 4,9
		draw_pixel_%1 8,10
		draw_pixel_%1 12,11
		draw_pixel_%1 16,12
		draw_pixel_%1 20,13
		draw_pixel_%1 24,14
		draw_pixel_%1 28,15
	
	;;	add edi,BUFX_2_MINUS_32
		inc_edi_std_%1
		pop edx
	%endmacro

	%macro draw_line_xflip_yflip 1
		mov eax,[ebp + %$zy]
		mov esi,full_y_skip
		cmp eax,16
		jz %%full_zoom
		mov esi,dda_y_skip
align 2
%%full_zoom:
		push edx
		neg edx
		add edx,eax
		xor eax,eax
		mov al, byte [esi + edx]	; 1 ou 0
		shl eax,3
		add ecx,eax
	
		mov eax,[ecx+4]
		draw_pixel_%1 0,0
		draw_pixel_%1 4,1
		draw_pixel_%1 8,2
		draw_pixel_%1 12,3
		draw_pixel_%1 16,4
		draw_pixel_%1 20,5
		draw_pixel_%1 24,6
		draw_pixel_%1 28,7

		mov eax,[ecx]
		draw_pixel_%1 0,8
		draw_pixel_%1 4,9
		draw_pixel_%1 8,10
		draw_pixel_%1 12,11
		draw_pixel_%1 16,12
		draw_pixel_%1 20,13
		draw_pixel_%1 24,14
		draw_pixel_%1 28,15
		;;sub edi,BUFX_2_PLUS_32
		inc_edi_yflip_%1
		pop edx
	%endmacro

	%macro get_dest_buf 0
		mov edx,BUFX_2
		mov eax,[ebp + %$sy]
		mov edi,[ebp + %$sx]
		mul edx
		shl edi,1
		add edi,eax
		add edi,[ebp + %$dest]	;  edi = poiteur sur la ligne a dessiner
	%endmacro

	%macro get_dest_buf_yflip 0
		mov eax,[ebp + %$zy]
		mov edx,BUFX_2
		sub eax,1
		mov edi,[ebp + %$sx]
		add eax,[ebp + %$sy]
		mul edx
		shl edi,1
		add edi,eax
		add edi,[ebp + %$dest]	;  edi = poiteur sur la ligne a dessiner
	%endmacro

	%macro draw_tile_i386 1
			;; start of the function
	proc draw_tile_i386_%1
	%$tileno arg
	%$sx     arg
	%$sy     arg
	%$zx     arg
	%$zy     arg
	%$color  arg
	%$xflip  arg
	%$yflip  arg
	%$dest   arg		; destination buffer (352x256x16)

	pusha
	
	;; pointeur sur la tile
	mov eax,[ebp + %$tileno]
	mov ecx,[mem_gfx]
	shl eax,7
	mov ecx,[ecx]
	add ecx,eax	; ecx = pointeur sur la tile

	;; pointeur sur la palette
	mov eax,[ebp + %$color]
	mov ebx,[current_pc_pal]
	shl eax,6
	add ebx,eax	; ebx = pointeur sur la palette

	;;  xzoom?
	cmp byte [ebp + %$zx],16
	jnz NEAR %%.draw_xzoom
	
	;; xflip?
	cmp byte [ebp + %$xflip],0
	jnz NEAR %%.draw_xflip	

	;; yflip?
	cmp byte [ebp + %$yflip],0
	jnz NEAR %%.draw_yflip


;;; draw a tile with yzoom
	get_dest_buf
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.loop_next_y:
	draw_line_std %1
	
	dec dx
	jnz NEAR %%.loop_next_y
	jmp %%.end

;;; draw a tile with yzoom+yflip
align 2
%%.draw_yflip
	get_dest_buf_yflip
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.yflip_loop_next_y:
	draw_line_yflip %1
	
	dec dx
	jnz NEAR %%.yflip_loop_next_y
	jmp %%.end

align 2
;;; draw a tile with yzoom+xflip
%%.draw_xflip:

	cmp byte [ebp + %$yflip],0
	jnz NEAR %%.draw_xflip_yflip

	get_dest_buf
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.xflip_loop_next_y:
	draw_line_xflip %1
	
	dec dx
	jnz NEAR %%.xflip_loop_next_y
	jmp %%.end

align 2
%%.draw_xflip_yflip
	get_dest_buf_yflip
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.xflip_yflip_loop_next_y:
	draw_line_xflip_yflip %1
	
	dec dx
	jnz NEAR %%.xflip_yflip_loop_next_y
	jmp %%.end

align 2
%%.draw_xzoom
	cmp byte [ebp + %$xflip],0
	jnz NEAR %%.xzoom_draw_xflip	

	cmp byte [ebp + %$yflip],0
	jnz NEAR %%.xzoom_draw_yflip


;;; draw a tile with yzoom+xzoom
	get_dest_buf
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.xzoom_loop_next_y:
	draw_line_std xzoom_%1
	
	dec dx
	jnz NEAR %%.xzoom_loop_next_y
	jmp %%.end

align 2
;;; draw a tile with xzoom+yzoom+yflip
%%.xzoom_draw_yflip
	get_dest_buf_yflip
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end
align 2
%%.xzoom_yflip_loop_next_y:
	draw_line_yflip xzoom_%1
	
	dec dx
	jnz NEAR %%.xzoom_yflip_loop_next_y
	jmp %%.end

align 2
;;; draw a tile with xzoom_yzoom+xflip
%%.xzoom_draw_xflip:

	cmp byte [ebp + %$yflip],0
	jnz NEAR %%.xzoom_draw_xflip_yflip

	get_dest_buf
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end

align 2
%%.xzoom_xflip_loop_next_y:
	draw_line_xflip xzoom_%1
	dec dx
	jnz NEAR %%.xzoom_xflip_loop_next_y
	jmp %%.end

align 2
%%.xzoom_draw_xflip_yflip
	get_dest_buf_yflip
	mov edx,[ebp + %$zy]	; dx = conteur de zx a 0
	cmp dx,0
	jz NEAR %%.end

align 2
%%.xzoom_xflip_yflip_loop_next_y:
	draw_line_xflip_yflip xzoom_%1
	dec dx
	jnz NEAR %%.xzoom_xflip_yflip_loop_next_y
	jmp %%.end

align 2
%%.end:
	popa
	endproc
%endmacro



;; scanline rendering
	%macro scan_draw_line_std 1

		mov eax,[ecx]
		draw_pixel_%1 28,0
		draw_pixel_%1 24,1
		draw_pixel_%1 20,2
		draw_pixel_%1 16,3
		draw_pixel_%1 12,4
		draw_pixel_%1 8,5
		draw_pixel_%1 4,6
		draw_pixel_%1 0,7

		mov eax,[ecx+4]
		draw_pixel_%1 28,8
		draw_pixel_%1 24,9
		draw_pixel_%1 20,10
		draw_pixel_%1 16,11
		draw_pixel_%1 12,12
		draw_pixel_%1 8,13
		draw_pixel_%1 4,14
		draw_pixel_%1 0,15
	%endmacro

	%macro scan_draw_line_xflip 1

		mov eax,[ecx+4]
		draw_pixel_%1 0,0
		draw_pixel_%1 4,1
		draw_pixel_%1 8,2
		draw_pixel_%1 12,3
		draw_pixel_%1 16,4
		draw_pixel_%1 20,5
		draw_pixel_%1 24,6
		draw_pixel_%1 28,7

		mov eax,[ecx]
		draw_pixel_%1 0,8
		draw_pixel_%1 4,9
		draw_pixel_%1 8,10
		draw_pixel_%1 12,11
		draw_pixel_%1 16,12
		draw_pixel_%1 20,13
		draw_pixel_%1 24,14
		draw_pixel_%1 28,15
	%endmacro


	%macro draw_scanline_tile_i386 1
			;; start of the function
	proc draw_scanline_tile_i386_%1
	%$tileno arg
	%$yoffs  arg
	%$sx     arg
	%$sy     arg
	%$zx     arg
	%$color  arg
	%$xflip  arg
	%$dest   arg		; destination buffer (352x256x16)

	pusha
	
	;; pointeur sur la tile
	mov eax,[ebp + %$tileno]
	mov ecx,[mem_gfx]
	mov edx,[ebp + %$yoffs]
	shl eax,7
	mov ecx,[ecx]
	shl edx,3
	add ecx,eax	; ecx = pointeur sur la tile
	add ecx,edx

	;; pointeur sur la palette
	mov eax,[ebp + %$color]
	mov ebx,[current_pc_pal]
	shl eax,6
	add ebx,eax	; ebx = pointeur sur la palette

	get_dest_buf


	;;  xzoom?
	cmp byte [ebp + %$zx],16
	jnz NEAR %%.draw_xzoom
	
	;; xflip?
	cmp byte [ebp + %$xflip],0
	jnz NEAR %%.draw_xflip	

	;; draw a line wit zx==16
	scan_draw_line_std %1
	jmp %%.end

%%.draw_xflip:
	;; draw a line wit zx==16 and xflip
	scan_draw_line_xflip %1
	jmp %%.end
	
%%.draw_xzoom:
	cmp byte [ebp + %$xflip],0
	jnz NEAR %%.xzoom_draw_xflip

	;; draw a line with zx!=16
	scan_draw_line_std xzoom_%1
	jmp %%.end

%%.xzoom_draw_xflip
	;; draw a line with zx!=16 and xflip
	scan_draw_line_xflip xzoom_%1

align 2
%%.end:
	popa
	endproc
%endmacro
