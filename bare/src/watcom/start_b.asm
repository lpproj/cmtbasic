;
; minimal startup for watcom x86 baremetal
;

		.8086

		INCLUDE	"segdef.inc"

IFDEF BARE_TINY

		PUBLIC	z_bss_offset
		PUBLIC	bare_isdos_: near
		PUBLIC	bare_reboot_: near
		PUBLIC	bare_goto_rombasic_: near
		EXTRN	bare_main_: near

_TEXT		SEGMENT

		ORG	100h

bare_boot_entry:
		cli
		mov	cs: [bare_org_es], es
		; check BootProg
IF 0
		cmp	si, 16381
		jne	@f
ENDIF
		cmp	di, 32749
		jne	@f
		cmp	bp, 65521
		jne	@f
		mov	byte ptr cs: [_isMSDOS_], 0
@@:
		mov	ax, cs
		mov	ss, ax
		mov	sp, offset DGROUP:boot_stack_bottom
		mov	ds, ax
		mov	es, ax
		cld
		sti
		cmp	byte ptr cs: [_isMSDOS_], 0
		jnz	bare_msdos
		xor	ax, ax
		mov	es, ax
		mov	al, byte ptr es: [0501h]
		and	al, 7
		jz	@f		; do not move if main mamory 128k
		dec	al		; reserve topmost 128k in main ram
		mov	byte ptr es: [0501h], al
@@:
		inc	al
		; es = ax * 8192para (128kbytes)
		mov	cl, 13
		shl	ax, cl
		mov	es, ax
		xor	si, si
		xor	di, di
		mov	cx, offset DGROUP: z_bss_offset
		rep	movsb
		cli
		mov	ax, es
		mov	ds, ax
		mov	ss, ax
		push	ax
		mov	ax, offset _TEXT: bare_main_code
		push	ax
		call	clear_bss
		sti
		retf		; jmp es: bare_move_code
bare_msdos:
		mov	es, [bare_org_es]
		mov	bx, offset DGROUP: z_bss_offset
		mov	cl, 4
		shr	bx, cl
		mov	ah, 4ah
		int	21h
		call	clear_bss
bare_main_code:
		push	cs
		pop	es
		call	bare_main_

bare_exit:
		cmp	byte ptr cs: [_isMSDOS_], 0
		jz	@f
		mov	ah, 4ch
		int	21h
@@:
bare_reboot_:
		cli
		mov	sp, 30h
		mov	ss, sp
		mov	sp, 0100h
		db	0eah		; jmp 0ffffh:00000h
		dw	0000h, 0ffffh

bare_goto_rombasic_:
		cli
		cld
		; init stack
		mov	ax, 30h
		mov	ss, ax
		mov	sp, 0100h
		; clear memory (from 0060:0000, at least 4KB, 64KB to be safe)
		mov	ax, 60h
		mov	es, ax
		xor	ax, ax
		xor	di, di
		mov	cx, 8000h
		rep	stosw
		mov	ds, ax
		mov	word ptr ds: [1eh * 4], ax
		mov	word ptr ds: [1eh * 4 + 2], 0e800h
		and	byte ptr ds: [0500h], 7fh	; cold boot (bit7=0) to be safe
		int	1eh		; invoke ROM BASIC (jmp E800:0000)
		; fallback 
		db	0eah		; jmp FFFF:0000
		dw	0000h, 0ffffh

clear_bss:
		mov	di, offset DGROUP: bss_offset
		mov	cx, offset DGROUP: bss2_offset
		sub	cx, di
		xor	ax, ax
		rep	stosb
		ret

; int __watcall bare_isdos(void)
bare_isdos_:
		mov	ax, [_isMSDOS_]
		ret

_TEXT		ENDS

		PUBLIC	_small_code_
_DATA		SEGMENT WORD
_small_code_	dw	1
_isMSDOS_	dw	1
bare_org_es	dw	0
_DATA		ENDS

_BSS		SEGMENT WORD
bss_offset:
_BSS		ENDS
_BSS2		SEGMENT WORD
bss2_offset:
		dw	512 dup (?)
boot_stack_bottom:
_BSS2		ENDS
Z_BSS		SEGMENT PARA
z_bss_offset:
Z_BSS		ENDS

ENDIF	; BARE_TINY

		END	bare_boot_entry

