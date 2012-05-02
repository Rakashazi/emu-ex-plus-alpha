;;; screen asm function

decal :	dd 	0x00010001,0x00010001
scan_mask16 : dd	0xf7def7de,0xf7def7de
	;; nasm macro
	%include 'c32.mac'

	;; global definition
	global do_inner_interpolation_i386

	extern scan_mask		
SECTION .text
	
	proc do_inner_interpolation_i386
	%$dst   arg
	%$src   arg

	push edi
	push esi
	push eax
	push ebx
	
	mov edi,[ebp + %$dst]	; edi=pointer on dest buffer
	mov esi,[ebp + %$src]	; esi=pointer on src buffer

		;; 	movq mm2, [scan_mask16]
		;; 	punpckldq mm2,mm2

	
	mov eax,224		; loop counter for y
loop_x:	
	mov ebx,20		; loop counter for x ((320/4)/4)
loop_y:


	movq mm0,[esi]
	movq mm1,[edi]
	movq mm2,[esi+8]
	movq mm3,[edi+8]
	movq mm4,[esi+16]
	movq mm5,[edi+16]
	movq mm6,[esi+24]
	movq mm7,[edi+24]

	pand mm0,[scan_mask16]
	pand mm1,[scan_mask16]
	pand mm2,[scan_mask16]
	pand mm3,[scan_mask16]
	pand mm4,[scan_mask16]
	pand mm5,[scan_mask16]
	pand mm6,[scan_mask16]
	pand mm7,[scan_mask16]
	
	psrlw mm0,1
	psrlw mm1,1
	psrlw mm2,1
	psrlw mm3,1
	psrlw mm4,1
	psrlw mm5,1
	psrlw mm6,1
	psrlw mm7,1
	
	paddw mm0,mm1
	paddw mm2,mm3
	paddw mm4,mm5
	paddw mm6,mm7
	
	
	movq [edi],mm0
	movq [edi+8],mm2
	movq [edi+16],mm4
	movq [edi+24],mm6
	
	add esi,32
	add edi,32
	
	dec ebx
	cmp ebx,0
	jne NEAR loop_y

 	add esi,64
 	add edi,64

	
	dec eax
	cmp eax,0
	jne NEAR loop_x


	pop ebx
	pop eax
	pop esi
	pop edi

	emms
	
	endproc


