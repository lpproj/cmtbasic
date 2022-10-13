#include "dos.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "int_hndl.h"

#include "ff.h"
#include "myprintf.h"
#include "mytxtout.h"

#include "necio.h"

#define SCRNBUF_MAX (80*2*31)
#define FENTRY_MAX (256)        /* FYI: rootent for 1.44mFD = 224 */

static FILINFO fentry[FENTRY_MAX];
static int fcount;
static int browser_frame_pos = 2;
static int browser_frame_columns = 80;
static int browser_indexes = 10;

static int menu_base_pos = 1;
static int save_input_pos = 0;

static unsigned char attr_normal = 0xe1;    /* white G+R+B */

static unsigned char attr_frame = 0xa1;     /* cyan G+B */
static unsigned char attr_fentry = 0xe1;    /* white G+R+B */
static unsigned short frame_ul = 0x9c;
static unsigned short frame_ur = 0x9d;
static unsigned short frame_dl = 0x9e;
static unsigned short frame_dr = 0x9f;
static unsigned short frame_h = 0x95;
static unsigned short frame_v = 0x96;

static unsigned char attr_status = 0xc5;    /* yellow G+R, reverse */
static unsigned char attr_newtape = 0x85;   /* green G, reverse */
static unsigned char attr_menu = 0xe1;    /* white G+R+B */
static unsigned char attr_menu_key = 0xe5;  /* white G+R+B, reverse */
static unsigned char attr_overwrite = 0xc5; /* yellow G+R, reverse */

static FATFS ff;
static FIL fil;
char tape_name[12];

enum {
    NO_TAPE = 0,
    TAPE_STOPPED,
    TAPE_LOAD,
    TAPE_LOADING,
    TAPE_SAVE,
    TAPE_SAVING
};

enum {
    MENU_MODE_NEUTRAL = 0,
    MENU_MODE_LOAD,
    MENU_MODE_SAVE,
    MENU_MODE_EJECT,

    MENU_MODE_BOUNDARY
};

int tape_changed = 1;
int tape_state = NO_TAPE;
FRESULT tape_error = FR_OK;

const unsigned GDC_BASE = 0x60;

unsigned short
gdc_get_csradd(void)
{
    unsigned char f[5];
    unsigned n;

    _disable();
    nec98_gdc_cmd(GDC_BASE, 0xe0);  /* CSRR */
    nec98_iowait();
    nec98_iowait();
    for(n=0; n<sizeof(n); ++n) {
        f[n] = nec98_gdc_readdata(GDC_BASE);
    }
    _enable();
    return (((unsigned short)(f[1]) << 8) | (f[0])) * 2;
}

static int fentry_reload(void)
{
    FILINFO *f;
    DIR dr;

    bzero(&dr, sizeof(dr));
    if (f_opendir(&dr, "/") != FR_OK) return -1;
    for (fcount=0, f=&(fentry[0]); fcount<FENTRY_MAX; ++fcount, ++f) {
        f->fname[0] = '\0';
        if (f_readdir(&dr, f) != FR_OK || f->fname[0] == '\0') break;
    }
    if (fcount >= 0 && fcount < (FENTRY_MAX - 1)) {
        f[1].fname[0] = '\0';
    }
    return fcount;
}

static
void disp_frame(int w, int h)
{
    int y = browser_frame_pos;

    con_fill(0, y, 0, y, frame_ul, attr_frame);
    con_fill(1, y, w-2, y, frame_h, attr_frame);
    con_fill(w-1, y, w-1, y, frame_ur, attr_frame);
    h = h + browser_frame_pos - 1;
    while(++y < h) {
        con_fill(0, y, 0, y, frame_v, attr_frame);
        con_fill(1, y, w-2, y, ' ', attr_fentry);
        con_fill(w-1, y, w-1, y, frame_v, attr_frame);
    }
    con_fill(0, y, 0, y, frame_dl, attr_frame);
    con_fill(1, y, w-2, y, frame_h, attr_frame);
    con_fill(w-1, y, w-1, y, frame_dr, attr_frame);

}

static
int disp_fentry(int frame_index, int disp_entry)
{
    unsigned n;
    char s[82];
    char fn[14];
    char flen[12];
    const FILINFO *f;

    if (disp_entry >= fcount || frame_index >= browser_indexes) return -1;
    f = &(fentry[disp_entry]);
    n = strlen(f->fname);
    strcpy(fn, "            ");
    memcpy(fn, f->fname, n);
    if (f->fattrib & AM_DIR) strcpy(flen, "<DIR>     ");
    else sprintf(flen, "%10lu", (unsigned long)(f->fsize));
    if (browser_frame_columns <= 40) {
        sprintf(s, "%s %s %02u-%02u-%02u %02u:%02u"
             , fn
             , flen
             , ((f->fdate >> 9) + 80) % 100
             , (f->fdate >> 5) & 15
             , f->fdate & 31
             , f->ftime >> 11
             , (f->ftime >> 5) & 63
           );
    }
    else {
        sprintf(s, "%s %s %04u-%02u-%02u %02u:%02u:%02u"
             , fn
             , flen
             , (f->fdate >> 9) + 1980
             , (f->fdate >> 5) & 15
             , f->fdate & 31
             , f->ftime >> 11
             , (f->ftime >> 5) & 63
             , (f->ftime & 31) << 1
           );
    }
    con_gotoxy0(1, browser_frame_pos + 1 + frame_index);
    con_puts(s);
    return 0;
}


static int disp_fentries(int base_index)
{
    int ibase, iwin;
    int i;

    ibase = iwin = base_index;
    if (iwin >= browser_indexes) {
        ibase = iwin - browser_indexes;
        iwin = browser_indexes - 1;
    }
    for(i=0; i<browser_indexes && ibase < fcount; ++i, ++ibase) {
        disp_fentry(i, ibase);
    }
    return iwin;
}

static void select_entry(int frame_index, int selected)
{
    int x = browser_frame_columns - 2;
    int y = browser_frame_pos + 1 + frame_index;
    con_fillattr(1, y, x, y, selected ? attr_fentry | 0x4 : attr_fentry);
}

#if 1
static unsigned short con_getch_x(void)
{
    unsigned short BARE_FAR *KB_BUF_HEAD = MK_FP(0, 0x524);
    unsigned short BARE_FAR *KB_BUF_TAIL = MK_FP(0, 0x526);
    unsigned char BARE_FAR *KB_COUNT = MK_FP(0, 0x528);
    unsigned short h, t;
    unsigned short k;

    while ((h = *KB_BUF_HEAD) == (t = *KB_BUF_TAIL)) {
        _asm {
            pushf
            sti
            nop
            hlt
            popf
        }
    }
    k = *(unsigned short BARE_FAR *)MK_FP(0, h);
    h += 2;
    if (h > 0x520) h = 0x502;
    *KB_BUF_HEAD = h;
    --(*KB_COUNT);
    return k;
}
#else
static unsigned short con_getch_x(void)
{
    return (unsigned short)(kbdbios(0)) & 0xffffU;
}
#endif
static unsigned char con_getch(void)
{
    return con_getch_x() & 0xffU;
}

const char * query_fname(int i)
{
    return (i >= 0 && i < fcount) ? fentry[i].fname : NULL;
}

int browse_fentry(void)
{
    static unsigned char scrnbuf[80*2*20*2];
    int rc = -1;
    int org_x, org_y;

    browser_frame_columns = con_columns();
    org_x = con_posx0();
    org_y = con_posy0();
    con_getscreen(scrnbuf, 0, org_y, browser_frame_columns - 1, org_y + browser_indexes + 2);
    disp_frame(browser_frame_columns, browser_indexes + 2);
    fentry_reload();
    if (fcount <= 0) {
        con_gotoxy0(4, browser_frame_pos + 1);
        con_puts("--- NO FILE ---");
        while(con_getch() != 0x1b)
            ;
    }
    else {
        int fwinpos = 0;
        int fbase = 0;
        int do_reload = 1;
        unsigned short k;
        while(1) {
            if (do_reload) {
                disp_fentries(fbase);
                select_entry(fwinpos, 1);
                do_reload = 0;
            }
            k = con_getch_x();
            if ((k & 0xff) == 0x1b) break;
            if ((k & 0xff) == 0x0d && (fentry[fbase + fwinpos].fattrib & AM_DIR) == 0) {
                rc = fbase + fwinpos;
                break;
            }
            if ((k >> 8) == 0x3a) { /* uparrow */
                select_entry(fwinpos, 0);
                if (fwinpos > 0) {
                    --fwinpos;
                }
                else if (fbase > 0) {
                    --fbase;
                    do_reload = 1;
                    continue;
                }
                select_entry(fwinpos, 1);
            }
            if ((k >> 8) == 0x3d) { /* downarrow */
                select_entry(fwinpos, 0);
                if (fwinpos >= browser_indexes - 1) {
                    if (fbase + fwinpos + 1 < fcount) {
                        ++fbase;
                        do_reload = 1;
                        continue;
                    }
                }
                else if (fbase + fwinpos + 1 < fcount) {
                    ++fwinpos;
                }
                select_entry(fwinpos, 1);
            }
        }
    }
    con_putscreen(scrnbuf, 0, org_y, browser_frame_columns - 1, org_y + browser_indexes + 2);
    con_gotoxy0(org_x, org_y);

    return rc;
}


static void disp_csr(unsigned char do_disp)
{
    crtbios(do_disp ? 0x1100 : 0x1200, 0);
}

int disp_status(void)
{
    const char *s;

    con_fill(0, 0, con_columns() -1, 0, ' ', attr_status);
    con_gotoxy0(0, 0);
    con_puts("TAPE: ");
    switch(tape_state) {
        case NO_TAPE:
            con_puts("[NO TAPE]");
            return 0;
        case TAPE_STOPPED:
            s = "STOP";
            break;
        case TAPE_LOAD:
            s = "LOAD";
            break;
        case TAPE_LOADING:
            s = "LOADING";
            break;
        case TAPE_SAVE:
            s = "SAVE";
            break;
        case TAPE_SAVING:
            s = "SAVING";
            break;
        default:
            s = "???";
            break;
    }
    con_puts(tape_name);
    con_puts(" (");
    con_puts(s);
    if (tape_error) con_puts(" ERROR");
    con_puts(")");
    return 0;
}


FRESULT do_remount(void)
{
    FRESULT fr = FR_OK;
    if (tape_changed) {
        memset(&ff, 0, sizeof(ff));
        fr = f_mount(&ff, "", 0);
        if (fr == FR_OK)
            tape_changed = 0;
    }
    return fr;
}


int do_stop(void)
{
    FRESULT fr = FR_NOT_READY;

    if (tape_state == TAPE_LOADING) {
        fr = f_close(&fil);
        tape_state = TAPE_STOPPED;
    }
    else if (tape_state == TAPE_SAVING) {
        fr = f_close(&fil);
        tape_state = TAPE_STOPPED;
    }
    return fr == FR_OK ? 0 : -1;
}


int do_eject(void)
{
    do_stop();
    tape_state = NO_TAPE;
    tape_name[0] = '\0';
    tape_changed = 1;
    disp_status();
    return 0;
}

int do_line_input(char *s, int max_chars_with_null)
{
    int col = con_columns();
    int y = con_posy0();
    int x_base = con_posx0();
    int cpos;

    for(cpos=0; s[cpos] && cpos < (max_chars_with_null - 1); ++cpos) {
        con_putc(s[cpos]);
    }
    while(1) {
        unsigned ks;
        char key, scan;

        con_gotoxy0(x_base + cpos, y);
        ks = con_getch_x();
        key = ks & 0xff;
        scan = (ks >> 8);
        if (key == 0x1b) return -1;
        if (key == 0x0d) break;
        if (key == 0x08 || scan == 0x3b) {
            if (cpos > 0) {
                --cpos;
                con_gotoxy0(x_base + cpos, y);
                con_putc(' ');
                s[cpos] = '\0';
            }
            // continue;
        }
        else if (key >= 0x20) {
            if (key >= 'a' && key <= 'z') key -= ('a' - 'A');
            if (cpos < (max_chars_with_null - 1)) {
                con_putc(key);
                s[cpos] = key;
                ++cpos;
            }
            // continue;
        }
    }
    s[cpos] = '\0';
    return cpos;
}

int do_save(void)
{
    int rc;
    int prev_x, prev_y;
    char tape[12];

    prev_x = con_posx0();
    prev_y = con_posy0();
    con_gotoxy0(0, save_input_pos);
    disp_csr(1);
    con_puts("NEW TAPE:");
    con_fillattr(0, save_input_pos, con_posx0() -1, save_input_pos, attr_newtape);
    con_fill(con_posx0(), save_input_pos, con_columns() -1, save_input_pos, ' ', attr_normal);
    con_puts(" ");
    strcpy(tape, "" /* tape_name */);
    rc = do_line_input(tape, sizeof(tape));
    if (rc >= 0) {
        FILINFO fno;
        FRESULT fr;
        fr = do_remount();
        if (fr == FR_OK)
            fr = f_stat(tape, &fno);
        if (fr == FR_OK) {
            if (fno.fattrib & (AM_RDO|AM_SYS|AM_DIR)) {
                fr = FR_INVALID_NAME;
            }
            else {
                static unsigned char statbuf[80*2*2];
                con_getscreen(statbuf, 0, save_input_pos, con_columns() - 1, save_input_pos);
                con_fill(0, save_input_pos, con_columns() - 1, save_input_pos, ' ', attr_overwrite);
                con_gotoxy0(0, save_input_pos);
                con_puts(tape);
                con_puts(" is exist. Overwrite?(y/n)");
                while(1) {
                    unsigned k = con_getch();
                    if (k == 0x1b || k == 'N' || k == 'n') {
                        rc = -1;
                        fr = FR_INVALID_NAME;
                        break;
                    }
                    if (k == 'Y' || k == 'y') {
                        rc = 0;
                        fr = FR_OK;
                        break;
                    }
                }
                con_putscreen(statbuf, 0, save_input_pos, con_columns() - 1, save_input_pos);
            }
        }
        if (fr == FR_NO_FILE || fr == FR_OK) {
            do_stop();
            fr = f_open(&fil, tape, FA_WRITE | FA_CREATE_ALWAYS);
            if (fr == FR_OK) {
                strcpy(tape_name, tape);
                tape_state = TAPE_SAVING;
                rc = 0;
            }
        }
        disp_status();
    }
    con_gotoxy0(prev_x, prev_y);

    return rc;
}


int do_load(void)
{
    FRESULT fr;

    fr = do_remount();
    if (fr == FR_OK) {
        int n;
        con_gotoxy0(0, browser_frame_pos);
        n = browse_fentry();
        if (n >= 0) {
            const char *fn = query_fname(n);
            do_stop();
            fr = f_open(&fil, fn, FA_READ);
            if (fr == FR_OK) {
                strcpy(tape_name, fn);
                tape_state = TAPE_LOADING;
            }
            disp_status();
        }
    }

    return fr == FR_OK ? 0 : -1;
}


int do_menu(int mode)
{
    static unsigned char scrnbuf[80*2*2*2];
    int rc = -1;
    unsigned char prev_csr;
    unsigned short prev_csradd;
    int x0_max;

    prev_csr = csr_display_scratchpad;
    con_initscreeninfo();
    if (bare_isdos()) {
        unsigned short x0, y0;
        x0 = *(unsigned char BARE_FAR *)MK_FP(0x60, 0x11c);
        y0 = *(unsigned char BARE_FAR *)MK_FP(0x60, 0x110);
        prev_csradd = (y0 * x0_max + x0) * 2;
    }
    else {
        prev_csradd = gdc_get_csradd();
    }
    x0_max = con_columns();
    --x0_max;
    con_getscreen(scrnbuf, 0, 0, x0_max, menu_base_pos);
    disp_status();
    con_gotoxy0(0, menu_base_pos);
    disp_csr(0);
    if (mode == MENU_MODE_NEUTRAL) {
        unsigned char prev_attr = con_getattr();
        con_fill(0, menu_base_pos, x0_max, menu_base_pos, ' ', attr_menu);
        con_fillattr(con_posx0(), con_posy0(), con_posx0(), con_posy0(), attr_menu_key);
        con_puts("L:LOAD ");
        con_fillattr(con_posx0(), con_posy0(), con_posx0(), con_posy0(), attr_menu_key);
        con_puts("S:SAVE ");
        con_fillattr(con_posx0(), con_posy0(), con_posx0(), con_posy0(), attr_menu_key);
        con_puts("E:EJECT ");
        // disp_status();
        while (mode == MENU_MODE_NEUTRAL) {
            unsigned k = con_getch();
            if (k == 0x1b) break;
            else if (k == 'L' || k == 'l') {
                mode = MENU_MODE_LOAD;
                break;
            }
            else if (k == 'S' || k == 's') {
                mode = MENU_MODE_SAVE;
                break;
            }
            else if (k == 'E' || k == 'e') {
                do_eject();
                /* continue; */
            }
        }
    }

    switch(mode) {
        case MENU_MODE_NEUTRAL:
            rc = 0;
            break;
        case MENU_MODE_SAVE:
            rc = do_save();
            break;
        case MENU_MODE_LOAD:
            rc = do_load();
        default:
            break;
    }

    con_putscreen(scrnbuf, 0, 0, x0_max, menu_base_pos);
    disp_csr(prev_csr);
    crtbios(0x1300, prev_csradd);
    return rc;
}


unsigned char do_cmt_write_1(unsigned char v)
{
    UINT n = 1;
    FRESULT fr = FR_NOT_READY;

    if (tape_state == TAPE_SAVING) fr = f_write(&fil, &v, n, &n);
    return (fr == FR_OK && n == 1) ? 0 : 0x02;
}

unsigned short do_cmt_read_1(unsigned char ignore_err)
{
    UINT n = 1;
    FRESULT fr = FR_NOT_READY;
    unsigned char v;

    if (tape_state == TAPE_LOADING) fr = f_read(&fil, &v, n, &n);
    if (fr == FR_OK && n == 1) return (unsigned short)v;
    if (ignore_err) return 0;
    return 0x2700;
}



int BARE_INTERRUPT_CALL cmt_handler(union bare_INTREGS *r)
{
    int rc;
    switch (r->h.ah) {
        case 0x01: {    /* motor off */
            do_stop();
            r->h.ah = 0;
            break;
        }
        case 0x02: /* motor on (load) */
        {
            if (tape_state == TAPE_LOADING) {
                r->h.ah = 0;
            }
            else {
                rc = do_menu(MENU_MODE_LOAD);
                r->h.ah = (rc < 0);
            }
            break;
        }
        case 0x03: /* motor on (save) */
        {    
            if (tape_state == TAPE_SAVING) {
                r->h.ah = 0;
            }
            else {
                rc = do_menu(MENU_MODE_SAVE);
                r->h.ah = (rc < 0);
            }
            break;
        }
        case 0x04: /* data write */
        {
            r->h.ah = do_cmt_write_1(r->h.al);
            break;
        }
        case 0x05: /* data read */
        {
            r->x.ax = do_cmt_read_1(r->h.al);
            break;
        }
        
        default:
            break;
    }
    return 1;
}

#if 1
int BARE_INTERRUPT_CALL kbd_menu_handler(union bare_INTREGS *r)
{
    do_menu(MENU_MODE_NEUTRAL);
    return 1;
}
#endif




static int cmtbios(unsigned char func, unsigned char param)
{
    union REGS r;
    struct SREGS sr;
    r.h.ah = func;
    r.h.al = param;
    int86x(0x1a, &r, &r, &sr);
    return (((unsigned)(- r.h.ah))<< 8) | r.h.al;
}

int
cmtbasic_test(int isdos)
{
    int rc = 0;
    int x, y;
    if (!isdos) return 0;

    con_gotoxy0(0, 15 + 0);
    con_puts("esc - quit");
    con_gotoxy0(0, 15 + 1);
    con_puts("1 - load");
    con_gotoxy0(0, 15 + 2);
    con_puts("2 - save");

    while(1) {
        unsigned k = con_getch();
        int rcm;
        if (k == 0x1b) break;
        if (k == '1') {
            int n;
            con_gotoxy0(0, 15 + 4);
            con_puts("                                ");
            con_gotoxy0(0, 15 + 4);
            rcm = cmtbios(2, 0x80); /* motor on, read mode, 1200bps */
            printf("load=%04x:", (unsigned)rcm);
            for(n=0; n<4 && rcm >=0; ++n) {
                rcm = cmtbios(5, 0);
                if (rcm >= 0) printf(" %02X", rcm & 0xff);
            }
            cmtbios(1, 0);      /* motor off */
        }
        if (k == '2') {
            con_gotoxy0(0, 15 + 4);
            con_puts("                                ");
            con_gotoxy0(0, 15 + 4);
            rcm = cmtbios(3, 0x80);/* motor on, write mode, 1200bps */
            printf("save=%04x:", (unsigned)rcm);
            cmtbios(4, 0);
            cmtbios(4, 1);
            cmtbios(4, 2);
            cmtbios(4, 3);
            cmtbios(1, 0);      /* motor off */
        }
    }

    return rc;
}

