		.8086

		INCLUDE	"segdef.inc"

INT_STACK_COUNT		EQU	1024

WAIT_SYSTEM	MACRO
		hlt
ENDM

		PUBLIC	_int09_org_handler
		PUBLIC	_cmt_org_handler
		PUBLIC	int09_new_handler_
		PUBLIC	cmt_new_handler_
		PUBLIC	_int09_stack_top
		PUBLIC	_cmt_stack_top
		PUBLIC	_in_int09chk
		PUBLIC	_in_cmt
		PUBLIC	_int18_org_handler
		PUBLIC	int18_new_handler_
		PUBLIC	_csr_address_scratchpad
		PUBLIC	_csr_display_scratchpad


		PUBLIC	kbdbios_
		PUBLIC	crtbios_

		EXTRN	kbd_menu_handler_: near
		EXTRN	cmt_handler_: near

_TEXT		SEGMENT WORD


; unsigned long __watcall kbdbios(unsigned r_ax)
; ax r_ax
kbdbios_:
		push	bx
		xor	bx, bx
		int	18h
		mov	dx, bx
		pop	bx
		ret

; unsigned long __watcall crtbios(unsigned r_ax, unsigned r_dx)
; ax r_ax
orgkbdbios:
crtbios_:
		pushf
		call	dword ptr cs: [_int18_org_handler]
		ret


int09_new_handler_:
		pushf
		call	dword ptr cs: [_int09_org_handler]
		cli
		sub	byte ptr cs: [_in_int09chk], 1
		jc	int09_chk
int09_handler_exit:
		add	byte ptr cs: [_in_int09chk], 1
		iret
int09_chk:
		push	ax
		push	bx
		push	ds
		xor	ax, ax
		mov	ds, ax
		mov	bx, word ptr ds: [0524h]
		cmp	bx, word ptr ds: [0526h]
		je	int09_chk_brk
		mov	ax, word ptr [bx]
		cmp	ax, 3f00h			; HELP
		jne	int09_chk_brk
		test	byte ptr ds: [053ah], 10h	; CTRL
		jz	int09_chk_brk
		mov	ah, 05h				; remove HELP in kbdbuff
		int	18h
		cli
		mov	word ptr cs: [_int09_stack_top], sp
		mov	word ptr cs: [_int09_stack_top + 2], ss
		mov	sp, cs
		mov	ss, sp
		mov	sp, offset DGROUP: int09_stack_bottom
		sti
		call	invoke_kbd_menu
		cli
		mov	ss, word ptr cs: [_int09_stack_top + 2]
		mov	sp, word ptr cs: [_int09_stack_top]
int09_chk_brk:
		pop	ds
		pop	bx
		pop	ax
		jmp	short int09_handler_exit
;		add	byte ptr cs: [_in_int09chk], 1
;		iret

invoke_kbd_menu:
		push	es
		push	ds
		push	bp
		push	di
		push	si
		push	dx
		push	cx
		push	bx
		push	ax
		mov	ax, cs
		mov	ds, ax
		mov	es, ax
		mov	ax, sp
		call	kbd_menu_handler_
		pop	ax
		pop	bx
		pop	cx
		pop	dx
		pop	si
		pop	di
		pop	bp
		pop	ds
		pop	es
		ret

;

cmt_new_handler_:
		pushf
		cmp	ah, 0
		je	cmt_chain
		cmp	ah, 5
		jbe	cmt01
cmt_chain:
		popf
		jmp	dword ptr cs: [_cmt_org_handler]
cmt01:
		cli
		sub	byte ptr cs: [_in_cmt], 1
		jnc	cmt01_2
		mov	word ptr cs: [_cmt_stack_top], sp
		mov	word ptr cs: [_cmt_stack_top + 2], ss
		mov	sp, cs
		mov	ss, sp
		mov	sp, offset DGROUP: cmt_stack_bottom
cmt01_2:
		sti
		push	es
		push	ds
		push	bp
		push	di
		push	si
		push	dx
		push	cx
		push	bx
		push	ax
		mov	ax, cs
		mov	ds, ax
		mov	es, ax
		mov	ax, sp
		call	cmt_handler_
		mov	[cs: _in_cmt_result], ax
		pop	ax
		pop	bx
		pop	cx
		pop	dx
		pop	si
		pop	di
		pop	bp
		pop	ds
		pop	es
		cli
		add	byte ptr cs: [_in_cmt], 1
		jnc	cmt01_ret
		mov	ss, word ptr cs: [_cmt_stack_top + 2]
		mov	sp, word ptr cs: [_cmt_stack_top]
cmt01_ret:
		popf
		iret


		EVEN
int18_new_handler_:
		cmp	ah, 13h
		je	int18_csraddr
		cmp	ah, 11h
		je	int18_csron
		cmp	ah, 12h
		je	int18_csroff
int18_chain:
		jmp	dword ptr cs: [_int18_org_handler]
int18_csraddr:
		mov	cs: [_csr_address_scratchpad], dx
		jmp	short int18_chain
int18_csron:
		mov	byte ptr cs: [_csr_display_scratchpad], 1
		jmp	short int18_chain
int18_csroff:
		mov	byte ptr cs: [_csr_display_scratchpad], 0
		jmp	short int18_chain


_TEXT		ENDS

_DATA		SEGMENT WORD
_int18_org_handler	dd	0
_int09_stack_top	dd	0
_int09_org_handler	dd	0
_cmt_stack_top	dd	0
_cmt_org_handler	dd	0
_in_int09chk		db	0
_in_cmt		db	0
_in_cmt_result		dw	0
_csr_address_scratchpad	dw	0
_csr_display_scratchpad	db	1

_DATA		ENDS

_BSS		SEGMENT WORD
;
		dw	INT_STACK_COUNT dup (?)
cmt_stack_bottom:
;
		dw	INT_STACK_COUNT dup (?)
int09_stack_bottom:
;
		dw	64 dup (?)
boot_stack_bottom:
_BSS		ENDS


		END
