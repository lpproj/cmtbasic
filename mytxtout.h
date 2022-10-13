#ifndef MY_TEXTOUT_H
#define MY_TEXTOUT_H

int con_initscreeninfo(void);
int con_getscreensize(int *w, int *h);
int con_columns(void);
int con_rows(void);
int con_puts(const char *s);

int con_getattr(void);
int con_setattr(int attr);
int con_posx0(void);
int con_posy0(void);
void con_gotoxy0(int x0, int y0);
void con_putc(int c);

void con_writestr(int x, int y, unsigned char attr, const char *str);
void con_fill(int x0, int y0, int x1, int y1, unsigned chr, unsigned char attr);
void con_fillattr(int x0, int y0, int x1, int y1, unsigned char attr);
void con_cls(void);

int con_getscreen(void BARE_FAR *m, int x0, int y0, int x1, int y1);
int con_putscreen(const void BARE_FAR *m, int x0, int y0, int x1, int y1);

#endif

