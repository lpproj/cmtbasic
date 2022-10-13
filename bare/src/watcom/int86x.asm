
	include "segdef.inc"

	.8086


		assume cs:_TEXT, ds:DGROUP, es:DGROUP, ss:DGROUP

		PUBLIC	bare_int86x_
_TEXT		SEGMENT

; int __watcall bare_int86x(unsigned intnum, const union bare_REGS near *r1, union bare_REGS near *r2, struct bare_SREGS near *sr);

;struct bare_WORDREGS {
;    unsigned short ax;
;    unsigned short bx;
;    unsigned short cx;
;    unsigned short dx;
;    unsigned short si;
;    unsigned short di;
;    unsigned short bp;
;    unsigned short flags;
;    unsigned short cflag;
;};
;struct bare_SREGS {
;    unsigned short es;
;    unsigned short cs;
;    unsigned short ss;
;    unsigned short ds;
;};

r_x_ax		EQU	(0)
r_x_bx		EQU	(2)
r_x_cx		EQU	(4)
r_x_dx		EQU	(6)
r_x_si		EQU	(8)
r_x_di		EQU	(10)
r_x_bp		EQU	(12)
r_x_flags	EQU	(14)
r_x_cflag	EQU	(16)

sr_es		EQU	(0)
sr_cs		EQU	(2)
sr_ss		EQU	(4)
sr_ds		EQU	(6)



; ax intnum
; dx r1
; bx r2
; cx sr
bare_int86x_:
		pushf		; [+18+18]
		push	dx	; [+18+16]
		push	cx	; [+18+14]
		push	bx	; [+18+12]
		push	si	; [+18+10]
		push	di	; [+18+8]
		push	bp	; [+18+6]
		push	ds	; [+18+4]
		push	es	; [+18+2]

		mov	cs: [int_vector], al

		mov	si, dx
		push	word ptr [si + r_x_ax]
		push	word ptr [si + r_x_bx]
		push	word ptr [si + r_x_cx]
		push	word ptr [si + r_x_dx]
		push	word ptr [si + r_x_si]
		push	word ptr [si + r_x_di]
		push	word ptr [si + r_x_bp]
		mov	bx, cx
		push	word ptr [bx + sr_ds]
		push	word ptr [bx + sr_es]

		pop	es
		pop	ds
		pop	bp
		pop	di
		pop	si
		pop	dx
		pop	cx
		pop	bx
		pop	ax

		db	0cdh		; int xxh
int_vector	db	0

		push	ax	; [+18]
		push	bx	; [+16]
		push	cx	; [+14]
		push	dx	; [+12]
		push	si	; [+10]
		push	di	; [+8]
		push	bp	; [+6]
		pushf		; [+4]
		push	ds	; [+2]
		push	es	; [+0]
		mov	bp, sp
		mov	bx, [bp + 18 + 14]	; fetch sr
		push	ss
		pop	ds
		pop	word ptr [bx + sr_es]
		pop	word ptr [bx + sr_ds]
		mov	bx, [bp + 18 + 12]	; fetch r2
		pop	ax			; flags
		mov	word ptr [bx + r_x_flags], ax
		and	ax, 1
		mov	word ptr [bx + r_x_cflag], ax
		pop	word ptr [bx + r_x_bp]
		pop	word ptr [bx + r_x_di]
		pop	word ptr [bx + r_x_si]
		pop	word ptr [bx + r_x_dx]
		pop	word ptr [bx + r_x_cx]
		pop	word ptr [bx + r_x_bx]
		pop	word ptr [bx + r_x_ax]

		pop	es
		pop	ds
		pop	bp
		pop	di
		pop	si
		pop	bx
		pop	cx
		pop	dx
		popf
		ret


_TEXT		ENDS

_DATA		SEGMENT
_DATA		ENDS

	END


