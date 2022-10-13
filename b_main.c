#include <limits.h>

#include "dos.h"
#include "int_hndl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "mytxtout.h"
#include "myprintf.h"

#include "ff.h"
#include "diskio.h"

#if 0
int BARE_INTERRUPT_CALL cmt_handler(union bare_INTREGS *r)
{
    (void)r;
    return 0;
}
#endif

#if 0
int BARE_INTERRUPT_CALL kbd_menu_handler(union bare_INTREGS *r)
{
    /* test */
    (void)r;
    (*(unsigned char BARE_FAR *)MK_FP(0xa000, 79*2))++;
    return 1;
}
#endif

static unsigned prf_limit = UINT_MAX;
static unsigned prf_count = 0;

static void output_dummy(int c)
{
    (void)c;
}

void output_dos(int c)
{
    union REGS r;
    struct SREGS sr;
    r.h.ah = 0x40;
    r.x.bx = 1;
    r.x.cx = 1;
    r.x.dx = FP_OFF(&c);
    sr.ds = FP_SEG(&c);
    int86x(0x21, &r, &r, &sr);
}
void outputn_dos(int c)
{
    if (prf_count < prf_limit || prf_count == UINT_MAX) {
        output_dos(c);
    }
    if (prf_count < UINT_MAX) ++prf_count;
}

#define output_bare  con_putc

void outputn_bare(int c)
{
    if (prf_count < prf_limit || prf_count == UINT_MAX) {
        output_bare(c);
    }
    if (prf_count < UINT_MAX) ++prf_count;
}

int is_rombasic_exist(void)
{
    const unsigned char BARE_FAR *e8 = MK_FP(0xe800U, 0);
    // printf("e800:0000 %02X %02X %02X %02X\n", e8[0], e8[1], e8[2], e8[3]);
    return (e8[0] == 0xeb && e8[1] < 0x80 && e8[2] == 0xeb && e8[3] < 0x80);
}

static int is_daua_fd(unsigned char daua)
{
    switch(daua & 0xf0) {
        case 0x10:
        case 0x30:
        /* case 0x50: */
        case 0x70:
        case 0x90:
        case 0xb0:
        case 0xf0:
            return 1;
    }
    return 0;
}

int bare_main(void)
{
    extern unsigned char setDaua(unsigned char daua);
    extern int cmtbasic_test(int isdos);
    int rc = 1;
    union REGS r;
    struct SREGS sr;
    unsigned char boot_daua;

    xfunc_output = output_dummy;

    cmt_org_handler = bare_getvect(0x1a);
    int18_org_handler = bare_getvect(0x18);
    int09_org_handler = bare_getvect(0x09);

    con_initscreeninfo();
    xfunc_output = bare_isdos() ? output_dos : output_bare;
    xfunc_output = output_bare;
    bare_setvect(0x09, MK_FP(bare_cs(), int09_new_handler));

    boot_daua = *(unsigned char BARE_FAR *)MK_FP(0, 0x0584); /* DISK_BOOT */
    setDaua(boot_daua);

    bare_setvect(0x18, MK_FP(bare_cs(), int18_new_handler));
    bare_setvect(0x1a, MK_FP(bare_cs(), cmt_new_handler));

    if (bare_isdos()) {
        unsigned i;

        for(i=0; i<26; ++i) {
            unsigned char daua_A = *(unsigned char BARE_FAR *)MK_FP(0x60, 0x2c86 + 1 + i*2);
            if (is_daua_fd(daua_A)) {
                setDaua(daua_A);
                rc = 0;
                break;
            }
        }
        if (rc == 0) {
            rc = cmtbasic_test(1 /* bare_isdos() */);
        }
    }

    if (!bare_isdos() && is_rombasic_exist() && is_daua_fd(boot_daua)) {
        bare_goto_rombasic();
    }

    bare_setvect(0x1a, cmt_org_handler);
    bare_setvect(0x18, int18_org_handler);
    bare_setvect(0x09, int09_org_handler);
    return rc;
}

