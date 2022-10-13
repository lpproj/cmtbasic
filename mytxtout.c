#include "dos.h"
#include "string.h"
#include "mytxtout.h"
#include "int_hndl.h"

static int rows = 25;
static int columns = 80;
static unsigned line_bytes = 160;   /* columns * char_bytes */
static unsigned char_bytes = 2;
static int is_hires = 0;
static unsigned vram_seg = 0xa000;

static int disp_cursor = 1;
static int is_graph = 1;
static int posx = 0;
static int posy = 0;
static unsigned posoff = 0;
#define OUTOFSCREEN (0xffffU)
static unsigned char char_attr = 0xe1;
static unsigned char sjis_lead = 0;

#define BIOSMEM(o) *((unsigned char far *)(unsigned long)((unsigned)(o) & 0xffffU))
#define BIOSMEMW(o) *((unsigned short far *)(unsigned long)((unsigned)(o) & 0xffffU))

#define VADDR(o) ((unsigned char far *)MK_FP(vram_seg,o))
#define U16FP(p) ((unsigned short far *)(p))

int con_initscreeninfo(void)
{
    union REGS r;
    struct SREGS sr;

    is_hires = (BIOSMEM(0x0501) & 8) != 0;
    vram_seg = is_hires ? 0xe000: 0xa000;
    r.x.ax = 0x0b00;
    int86x(0x18, &r, &r, &sr);
    if (r.h.al & 2) {
        columns = 40;
        char_bytes = 4;
    }
    else {
        columns = 80;
        char_bytes = 2;
    }
    if (r.h.al & 1) {
        rows = (r.h.al & 0x10) ? 30 : 20;
    }
    else
        rows = 25;

    return 0;
}

int con_getscreensize(int *w, int *h)
{
    if (columns && rows) {
        if (w) *w = columns;
        if (h) *h = rows;
        return 0;
    }
    return -1;
}

int con_columns(void)
{
    return columns;
}

int con_rows(void)
{
    return rows;
}


static unsigned vram_off(int x0, int y0)
{
    if (posx >= 0 && posx < columns && posy >= 0 && posy < rows) {
        return (unsigned)y0 * line_bytes + (unsigned)x0 * char_bytes;
    }
    return OUTOFSCREEN;
}

static void v_writeca(unsigned off, unsigned code, unsigned char attr)
{
    register unsigned char far *vp = VADDR(off);

    *U16FP(vp) = (unsigned short)code;
    *(vp + 0x2000U) = attr;
    return;
}
static void v_writec(unsigned off, unsigned code, unsigned char attr)
{
    register unsigned char far *vp = VADDR(off);

    *U16FP(vp) = (unsigned short)code;
    return;
}
static void v_writea(unsigned off, unsigned code, unsigned char attr)
{
    register unsigned char far *vp = VADDR(off);

    *(vp + 0x2000U) = attr;
    return;
}

static void v_update_curpos(void)
{
#if 1
    crtbios(0x1300, vram_off(posx, posy));
#else
    union REGS r;
    struct SREGS sr;
    r.h.ah = 0x13;
    r.x.dx = vram_off(posx, posy);
    int86x(0x18, &r, &r, &sr);
#endif
}

static void v_curpos_cr(void)
{
    posx = 0;
}

static int v_curpos_lf(void)
{
    int need_roll = 0;
    ++posy;
    if (posy >= rows) {
        need_roll = posy - rows + 1;
    }
    return need_roll;
}

static int v_curpos_next(void)
{
    int need_roll = 0;

    ++posx;
    if (posx >= columns) {
        posx = 0;
        need_roll = v_curpos_lf();
    }
    return need_roll;
}

static int v_curpos_right(void)
{
    ++posx;
    if (posx >= columns) {
        ++posy;
        if (posy >= rows) {
            /* do not scroll */
            posy = rows - 1;
            posx = columns - 1;
        }
        else {
            posx = 0;
        }
    }
    return 0;
}

static void v_rawputc(unsigned char c)
{
    posoff = vram_off(posx, posy);
    if (posoff != OUTOFSCREEN) {
        v_writec(posoff, c, char_attr);
        v_curpos_next();
    }
}

void con_writestr(int x, int y, unsigned char attr, const char *str)
{
    unsigned poff = vram_off(x, y);
    unsigned char c;

    if (poff == OUTOFSCREEN) return;
    while((c = *(const unsigned char *)str++) != '\0') {
        v_writec(poff, c, attr);
        ++x;
        if (x >= columns) break;
        poff += char_bytes;
    }
}

void con_fill_sub(int x0, int y0, int x1, int y1, unsigned chr, unsigned char attr, void (*con_write_sub)(unsigned poff, unsigned chr, unsigned char attr))
{
    unsigned poff, x, y;

    if (x0 < 0) x0 = 0;
    else if (x0 >= columns) x0 = columns - 1;
    if (y0 < 0) y0 = 0;
    else if (y0 >= rows) y0 = rows - 1;
    if (x1 < 0) x1 = 0;
    else if (x1 >= columns) x1 = columns - 1;
    if (y1 < 0) y1 = 0;
    else if (y1 >= rows) y1 = rows - 1;

    for(y = (unsigned)y0; y<=(unsigned)y1; ++y) {
        x = (unsigned)x0;
        poff = vram_off(x, y);
        for(; x<=(unsigned)x1; ++x) {
            con_write_sub(poff, chr, attr);
            poff += char_bytes;
        }
    }
}
void con_fill(int x0, int y0, int x1, int y1, unsigned chr, unsigned char attr)
{
    con_fill_sub(x0, y0, x1, y1, chr, attr, v_writeca);
}

void con_fillattr(int x0, int y0, int x1, int y1, unsigned char attr)
{
    con_fill_sub(x0, y0, x1, y1, '\0', attr, v_writea);
}

void con_cls(void)
{
    con_fill(0, 0, columns - 1, rows - 1, ' ', char_attr);
    posx = posy = 0;
}


void con_putc(int c)
{
    switch(c) {
        case 13:
            v_curpos_cr();
            break;
        case 10:
            v_curpos_lf();
            break;
        default:
            v_rawputc((unsigned char)c);
    }
    if (disp_cursor) v_update_curpos();
}

int con_puts(const char *s)
{
    int n = 0;
    while(*s) {
        v_rawputc(*s++);
        ++n;
    }
    if (disp_cursor)
        v_update_curpos();
    return n;
}

int con_getattr(void)
{
    return char_attr;
}

int con_setattr(int a)
{
    unsigned int prev_attr = char_attr;
    char_attr = (unsigned char)a;
    return prev_attr;
}

int con_posx0(void)
{
    return posx;
}
int con_posy0(void)
{
    return posy;
}

void con_gotoxy0(int x0, int y0)
{
    if (x0 < 0) x0 = 0;
    else if (x0 >= columns) x0 = columns - 1;
    if (y0 < 0) y0 = 0;
    else if (y0 >= rows) y0 = rows - 1;

    posx = x0;
    posy = y0;
    if (disp_cursor) v_update_curpos();
}

int con_getscreen(void BARE_FAR *m, int x0, int y0, int x1, int y1)
{
    int slen = (x1-x0+1) * char_bytes;
    long mlen = (y1-y0+1) * slen;
    int y;

    if (slen < 0 || mlen < 0 || mlen > 0x3fff) return -1;
    if (m && slen > 0) {
        char BARE_FAR *scrn = MK_FP(vram_seg, y0 * line_bytes + x0 * char_bytes);;
        for (y=y0; y<=y1; ++y) {
            _fmemcpy(m, scrn, slen);
            m = (char BARE_FAR *)m + slen;
            _fmemcpy(m, scrn + 0x2000U, slen);
            m = (char BARE_FAR *)m + slen;
            scrn += line_bytes;
        }
    }

    return (int)mlen;
}

int con_putscreen(const void BARE_FAR *m, int x0, int y0, int x1, int y1)
{
    int slen = (x1-x0+1) * char_bytes;
    long mlen = (y1-y0+1) * slen;
    int y;

    if (slen < 0 || mlen < 0 || mlen > 0x3fff) return -1;
    if (m && slen > 0) {
        char BARE_FAR *scrn = MK_FP(vram_seg, y0 * line_bytes + x0 * char_bytes);
        for (y=y0; y<=y1; ++y) {
            _fmemcpy(scrn, m, slen);
            m = (const char BARE_FAR *)m + slen;
            _fmemcpy(scrn + 0x2000U, m, slen);
            m = (const char BARE_FAR *)m + slen;
            scrn += line_bytes;
        }
    }

    return (int)mlen;
}


