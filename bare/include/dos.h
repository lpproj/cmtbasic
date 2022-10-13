#ifndef BARE_DOS_H
#define BARE_DOS_H

# ifndef BARE_DEFS_H
#  include "_defs.h"
# endif

# define _INC_DOS
# define __DOS_H
# define _DOS_H_INCLUDED

# ifdef __cplusplus
extern "C" {
# endif


# if defined(_MSC_VER) || defined(__SC__) || defined(__WATCOMC__)
#  pragma pack(1)
# endif

struct bare_WORDREGS {
    unsigned short ax;
    unsigned short bx;
    unsigned short cx;
    unsigned short dx;
    unsigned short si;
    unsigned short di;
    unsigned short bp;
    unsigned short flags;
    unsigned short cflag;
};
struct bare_BYTEREGS {
    unsigned char al, ah;
    unsigned char bl, bh;
    unsigned char cl, ch;
    unsigned char dl, dh;
    unsigned char si_l, si_h;
    unsigned char di_l, di_h;
    unsigned char bp_l, bp_h;
};

union bare_REGS {
    struct bare_WORDREGS x;
    struct bare_BYTEREGS h;
};

struct bare_SREGS { /* same order in many compilers ... is it important? */
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
};

struct bare_INTFRAME {
    unsigned short ax;
    unsigned short bx;
    unsigned short cx;
    unsigned short dx;
    unsigned short si;
    unsigned short di;
    unsigned short bp;
    unsigned short ds;
    unsigned short es;
};

union bare_INTREGS {
    struct bare_INTFRAME x;
    struct bare_BYTEREGS h;
};

struct bare_ORGSTACK {
    unsigned short tmp_flags;
    unsigned short org_ip;
    unsigned short org_cs;
    unsigned short org_flags;
};

# define bare_MK_FP(s,o)    ((void BARE_FAR *)(((unsigned long)(s) << 16) | ((unsigned long)(unsigned)(o) & 0xffffU)))
# define bare_FP_SEG(p)     ((unsigned short)((unsigned long)(void BARE_FAR *)(p) >> 16))
# define bare_FP_OFF(p)     ((unsigned short)((unsigned long)(void BARE_FAR *)(p) & 0xffffU))


int BARE_CALL bare_int86x(unsigned intnum, const union bare_REGS BARE_NEAR *r1, union bare_REGS BARE_NEAR *r2, struct bare_SREGS BARE_NEAR *sr);
void BARE_CALL bare_segread(struct bare_SREGS BARE_NEAR *sr);


# ifdef __WATCOMC__
unsigned char bare_inp(unsigned port);
unsigned bare_inpw(unsigned port);
#  pragma aux bare_inp = "in al, dx" parm [dx] value [al] modify exact [al];
#  pragma aux bare_inpw = "in ax, dx" parm [dx] value [ax] modify exact [ax];
unsigned bare_outp(unsigned port, unsigned v);
unsigned bare_outpw(unsigned port, unsigned v);
#  pragma aux bare_outp = "out dx, al" parm [dx] [ax] value [ax] modify exact [ax];
#  pragma aux bare_outpw = "out dx, ax" parm [dx] [ax] value [ax] modify exact [ax];
extern void bare_enable(void);
extern void bare_disable(void);
#  pragma aux bare_enable = "sti";
#  pragma aux bare_disable = "cli";
extern unsigned bare_cs(void);
extern unsigned bare_ds(void);
extern unsigned bare_ss(void);
#  pragma aux bare_cs = "mov ax,cs" value [ax] modify exact [ax];
#  pragma aux bare_ds = "mov ax,ds" value [ax] modify exact [ax];
#  pragma aux bare_ss = "mov ax,ss" value [ax] modify exact [ax];
# else
unsigned char BARE_CALL bare_inp(unsigned port);
unsigned BARE_CALL bare_inpw(unsigned port);
unsigned BARE_CALL bare_outp(unsigned port, unsigned v);
unsigned BARE_CALL bare_outpw(unsigned port, unsigned v);
void BARE_CALL bare_enable(void);
void BARE_CALL bare_disable(void);
void BARE_CALL bare_cs(void);
void BARE_CALL bare_ds(void);
void BARE_CALL bare_ss(void);
# endif

void BARE_FAR * BARE_CALL bare_getvect(unsigned vect);
void BARE_CALL bare_setvect(unsigned vect, const void BARE_FAR *p);


# define REGS   bare_REGS
# define SREGS  bare_SREGS
# define int86x bare_int86x
# define segread    bare_segread
# define MK_FP(s,o) bare_MK_FP(s,o)
# define FP_SEG(p)  bare_FP_SEG(p)
# define FP_OFF(p)  bare_FP_OFF(p)
# define _enable    bare_enable
# define _disable   bare_disable
# define inp        bare_inp
# define inpw       bare_inpw
# define outp       bare_outp
# define outpw      bare_outpw

# if defined(_MSC_VER) || defined(__SC__) || defined(__WATCOMC__)
#  pragma pack()
# endif

# ifdef __cplusplus
}
# endif

#endif
