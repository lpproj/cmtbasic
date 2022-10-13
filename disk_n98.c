#include "dos.h"
#include "myprintf.h"
#include "ff.h"
#include "diskio.h"
#include "string.h"

#ifndef STA_OK
# define STA_OK 0x00
#endif

#define peekw(p)  (*((const unsigned short *)(p)))
#define peekdw(p)  (*((const unsigned long *)(p)))

enum {
    DISKCMD_SENSE = 0x04,
    DISKCMD_WRITE = 0x05,
    DISKCMD_READ = 0x06,
    DISKCMD_RECALIBRATE = 0x07,
    DISKCMD_READID = 0x0a,
    DISKCMD_WRITE_MFM = 0x45,
    DISKCMD_READ_MFM = 0x46,
    DISKCMD_READID_MFM = 0x4a,
};

typedef struct DISKGEOM {
    unsigned daua;
    unsigned long offset;
    unsigned long total_sectors;
    unsigned cylinders;
    unsigned heads;
    unsigned sectors_per_track;
    unsigned sector_bytes;
    unsigned sector_bytes_phys;
    unsigned sector_bytes_id;
    unsigned sector_base;
    int current_track;
} DISKGEOM;


#define OUTOFLBA    0xffffffffUL

BYTE diskBuf[FF_MAX_SS];

static int isDiskChanged = 0;
static int nCurrentTrack = -1;         /* need recalibrate ... less than 0 */
static unsigned short biosError = 0;
static unsigned char daua = 0xff;
static DISKGEOM geom;

unsigned char
setDaua(unsigned char newdaua)
{
    unsigned char olddaua = geom.daua;
    geom.daua = newdaua;
    return olddaua;
}

static int is640(const DISKGEOM *g)
{
    const unsigned char da = g->daua & 0xf0;
    return da == 0x10 || da == 0x70;
}
static int is1M(const DISKGEOM *g)
{
    const unsigned char da = g->daua & 0xf0;
    return da == 0x30 || da == 0x90 || da == 0xf0;
}


static
unsigned short
invoke1b(union REGS *r, struct SREGS *sr)
{
    int86x(0x1b, r, r, sr);
    if (r->x.cflag) {
        return (biosError = r->x.ax);
    }
    return 0;
}

static
unsigned short
diskbios_recalibrate(DISKGEOM *g)
{
    if (g->current_track < 0) {
        union REGS r;
        struct SREGS sr;
        r.h.ah = DISKCMD_RECALIBRATE;
        r.h.al = g->daua;
        if (invoke1b(&r, &sr)) {
            return biosError;
        }
        g->current_track = 0;
    }
    return 0;
}

static
unsigned short
diskbios_p0(DISKGEOM *g, unsigned char cmd)
{
    union REGS r;
    struct SREGS sr;

    r.h.ah = cmd;
    r.h.al = g->daua;
    return invoke1b(&r, &sr);
}


static
unsigned short
diskbios_rw_fd(DISKGEOM *g, unsigned char cmd, void BARE_FAR *buf, unsigned bytes, unsigned cylinder, unsigned head, unsigned sector)
{
    union REGS r;
    struct SREGS sr;

    r.h.ah = cmd;
    if (g->current_track != (int)cylinder) {
        r.h.ah |= 0x10;     /* enable SEEK */
    }
    r.h.al = g->daua;
    r.x.bx = bytes;
    r.h.ch = g->sector_bytes_id;
    r.h.cl = cylinder;
    r.h.dh = head;
    r.h.dl = sector;
    r.x.bp = FP_OFF(buf);
    sr.es = FP_SEG(buf);
    int86x(0x1b, &r, &r, &sr);
    if (r.x.cflag || (r.h.ah & 0xf0)==0x10) {
        return (biosError = r.x.ax);
    }
    g->current_track = cylinder;

    return 0;
}

static
unsigned short
diskbios_rw_fd_lba(DISKGEOM *g, unsigned char cmd, void BARE_FAR *buf, unsigned bytes, unsigned long lba)
{
    unsigned c, hs, h, s;
    unsigned long spc = ((unsigned long)(g->sectors_per_track) * g->heads);
    c = lba / spc;
    hs = lba % spc;
    h = hs / g->sectors_per_track;
    s = hs % g->sectors_per_track;
    return diskbios_rw_fd(g, cmd, buf, bytes, c, h, s + g->sector_base);
}


static
unsigned short
diskbios_readid_track0(unsigned char daua, int is_mfm, unsigned *sector_length)
{
    union REGS r;
    struct SREGS sr;

    /* diskbios_recalibrate(); */
    r.h.ah = is_mfm ? DISKCMD_READID_MFM : DISKCMD_READID;
    r.h.al = daua;
    r.h.cl = 0;     /* cylinder = 0 */
    r.h.dh = 0;     /* head = 0 */
    if (invoke1b(&r, &sr)) {
        return r.x.ax;
    }
    if (sector_length) {
        *sector_length = r.h.ch;
    }

    return 0;
}


static
int
DRV_AVAIL(BYTE pdrv)
{
    return (pdrv == 0);
}


DSTATUS
disk_initialize(BYTE pdrv)
{
    unsigned rc;
    union REGS r;
    struct SREGS sr;
    unsigned sct_id;

    if (!DRV_AVAIL(pdrv)) return STA_NOINIT | STA_NODISK;

    if (geom.daua == 0xff) {
        geom.daua = *(unsigned char BARE_FAR *)MK_FP(0, 0x584);  /* DISK_BOOT */
    }
    geom.current_track = -1;
    if (diskbios_p0(&geom, DISKCMD_RECALIBRATE)) { /* RECALIBRATE (and check the drive exists) */
        return STA_NODISK | STA_NOINIT;
    }
    if (diskbios_p0(&geom, DISKCMD_SENSE)) {   /* SENSE (check a medium exists in the drive) */
        return STA_NODISK | STA_NOINIT;
    }
    if (diskbios_readid_track0(geom.daua, 1, &sct_id)) {
        return STA_NOINIT;
    }
    if (sct_id != 2 && sct_id != 3) {
        return STA_NOINIT;      /* not either 512bytes or 1024bytes per sector */
    }

    geom.sector_bytes_id = sct_id;
    geom.sector_bytes_phys = 128U << sct_id;
    geom.sector_base = 1;   /* fd=1, nec98_hd=0 */
    geom.offset = 0;
    /* read LBA 0 */
    geom.current_track = 0;
    rc = diskbios_rw_fd(&geom, DISKCMD_READ_MFM, diskBuf, geom.sector_bytes_phys, 0, 0, geom.sector_base);
    if (rc == 0) {
        geom.sector_bytes = peekw(&(diskBuf[0x0b]));
        if (geom.sector_bytes != geom.sector_bytes_phys) {
            rc = (unsigned)-1;
        }
        if ((geom.total_sectors = peekdw(&(diskBuf[0x13]))) == 0) {
            geom.total_sectors = peekdw(&(diskBuf[0x20]));
        }
        geom.sectors_per_track = peekw(&(diskBuf[0x18]));
        geom.heads = peekw(&(diskBuf[0x1a]));
        geom.offset = 0; /* peekdw(&(diskBuf[0x1c])); */
        if (geom.sectors_per_track == 0 || geom.heads == 0) {
            rc = (unsigned)-1;
        }
    }

    if (rc != 0) {
        geom.current_track = -1;
        return STA_NOINIT;
    }
    return STA_OK;
}


DRESULT
disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_OK;
    BYTE *b = buff;

    if (!DRV_AVAIL(pdrv)) return RES_NOTRDY;

    while(count > 0) {
        if (diskbios_rw_fd_lba(&geom, DISKCMD_READ_MFM, diskBuf, geom.sector_bytes, sector) != 0) {
            res = RES_ERROR;
            break;
        }
        memcpy(b, diskBuf, geom.sector_bytes);
        b += geom.sector_bytes;
        ++sector;
        --count;
    }
    return res;
}


DRESULT
disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    DRESULT res = RES_OK;
    const BYTE *b = buff;

    if (!DRV_AVAIL(pdrv)) return RES_NOTRDY;

    while(count > 0) {
        unsigned r;
        memcpy(diskBuf, b, geom.sector_bytes);
        r = diskbios_rw_fd_lba(&geom, DISKCMD_WRITE_MFM, diskBuf, geom.sector_bytes, sector);
        if (r != 0) {
            res = ((r & 0xf000) == 0x7000) ? RES_WRPRT : RES_ERROR;
            break;
        }
        b += geom.sector_bytes;
        ++sector;
        --count;
    }
    return res;
}


DRESULT
disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    DRESULT res = RES_ERROR;

    if (!DRV_AVAIL(pdrv)) return RES_NOTRDY;
    switch(cmd) {
        case CTRL_SYNC:
            res = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = geom.sector_bytes;
            res = RES_OK;
            break;
        
        default:
            break;
    }
    return res;
}

DSTATUS
disk_status(BYTE pdrv)
{
    if (!DRV_AVAIL(pdrv)) return STA_NODISK;
    if (geom.sector_bytes == 0) return STA_NOINIT;
    return STA_OK;
}


static
unsigned
btoi(unsigned char bcd)
{
    return (bcd >> 4) * 10U + (bcd & 0x0f);
}

DWORD
get_fattime(void)
{
    union REGS r;
    struct SREGS sr;
    unsigned char b[6];
    unsigned y, d, t;

    r.h.ah = 0;
    r.x.bx = FP_OFF(b);
    sr.es = FP_SEG(b);
    int86x(0x1c, &r, &r, &sr);

    y = btoi(b[0]);
    y = (y >= 80 && y <= 99) ? (y - 80) : (y + 20);
    d = btoi(b[2]) | ((unsigned)(b[1] & 0xf0) << 1) | (y << 9);
    t = ((btoi(b[3])&31)<<11) | ((btoi(b[4])&63)<<5) | ((btoi(b[5])&63)>>1);
    return ((DWORD)d << 16) | t;
}


