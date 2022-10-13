#ifndef BARE_INT_HNDL_H
#define BARE_INT_HNDL_H

# ifndef BARE_DEFS_H
#  include "_defs.h"
# endif

# include "dos.h"

# ifdef __cplusplus
extern "C" {
# endif

extern struct bare_ORGSTACK BARE_FAR *kbd_stack_top;
extern struct bare_ORGSTACK BARE_FAR *cmt_stack_top;
extern unsigned char in_kbd;
extern unsigned char in_cmt;

extern void BARE_FAR *int09_org_handler;
void BARE_FAR int09_new_handler(void);
extern void BARE_FAR *cmt_org_handler;
void BARE_FAR cmt_new_handler(void);

extern void BARE_FAR *int18_org_handler;
void BARE_FAR int18_new_handler(void);
extern unsigned short csr_address_scratchpad;
extern unsigned char csr_display_scratchpad;

int BARE_INTERRUPT_CALL kbd_menu_handler(union bare_INTREGS *);
int BARE_INTERRUPT_CALL cmt_handler(union bare_INTREGS *);

unsigned long BARE_CALL kbdbios(unsigned to_reg_ax);
unsigned BARE_CALL crtbios(unsigned to_reg_ax, unsigned to_reg_dx);

# ifdef __cplusplus
}
# endif

#endif
