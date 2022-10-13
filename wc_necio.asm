		.8086

		INCLUDE	"segdef.inc"

_TEXT		SEGMENT WORD

		PUBLIC	nec98_iowait_

; void __watcall nec98_iowait(void);
iowait_handler_init:
		mov	word ptr cs: [iowait_handler], offset iowait_handler_5f
		push	ax
		push	ds
		xor	ax, ax
		mov	ds, ax
		mov	ax, byte ptr [0500h]
		and	ax, 3801h
		cmp	ax, 2001h	; PC98x1 normal (V30 or above)
		je	@f
		cmp	ax, 2800h	; hi-res (except 98XA)
		je	@f
		mov	word ptr cs: [iowait_handler], offset iowait_handler_legacy
@@:
		pop	ds
		pop	ax
nec98_iowait_:
		jmp	word ptr cs: [iowait_handler]

iowait_handler_legacy:
		push	cs
		call	iowait_handler_retf
		push	cs
		call	iowait_handler_retf
		ret
iowait_handler_retf:
		retf

iowait_handler_5f:
		out	5fh, al
		ret

iowait_handler	dw	iowait_handler_init


; void __watcall nec98_gdc_cmd(unsigned port_base /*ax*/, unsigned char cmd /*dx*/)
		PUBLIC	nec98_gdc_cmd_
nec98_gdc_cmd_:
		push	ax
		push	dx
		xchg	ax, dx
		mov	ah, al
@@:
		in	al, dx
		call	nec98_iowait_
		test	al, 2
		jnz	@b
		add	dx, 2
		mov	al, ah
		out	dx, al
		pop	dx
		pop	ax
		ret

; unsigned short __watcall nec98_gdc_readdata(unsigned port_base /*ax*/)
		PUBLIC	nec98_gdc_readdata_
nec98_gdc_readdata_:
		push	dx
		mov	dx, ax
@@:
		in	al, dx
		call	nec98_iowait_
		test	al, 1
		jz	@b
		mov	ax, 2		; clear AH
		add	dx, ax
		in	al, dx
		pop	dx
		ret



_TEXT		ENDS


	END
