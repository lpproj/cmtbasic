		.8086

		INCLUDE	"segdef.inc"


_TEXT		SEGMENT

; void far * __watcall bare_getvect(unsigned vect)
; ax vect
		PUBLIC	bare_getvect_
bare_getvect_:
		push	bx
		push	ds
		xor	bx, bx
		mov	ds, bx
		mov	bl, al
		add	bx, bx
		add	bx, bx
		mov	ax, word ptr [bx]
		mov	dx, word ptr [bx + 2]
		pop	ds
		pop	bx
		ret

; void __watcall bare_setvect(unsigned vect, const void far *p)
; ax    vect
; cx:bx p
		PUBLIC	bare_setvect_
bare_setvect_:
		push	ax
		push	si
		push	ds
		pushf
		xor	si, si
		mov	ds, si
		xor	ah, ah
		add	ax, ax
		add	ax, ax
		cli
		mov	si, ax
		mov	word ptr [si], bx
		mov	word ptr [si + 2], cx
		popf
		pop	ds
		pop	si
		pop	ax
		ret


_TEXT		ENDS


	END

