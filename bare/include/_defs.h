#ifndef BARE_DEFS_H
#define BARE_DEFS_H

#ifdef __WATCOMC__
# define BARE_CALL      __watcall
# define BARE_INTERRUPT_CALL      __watcall
# define BARE_NEAR      near
# define BARE_FAR       far
# define STRUCT_PACK
#else
# define BARE_CALL
# define BARE_INTERRUPT_CALL
# define STRUCT_PACK
# define BARE_NEAR
# define BARE_FAR       far
#endif

#ifdef __cplusplus
extern "C" {
#endif
int BARE_CALL bare_main(void);
int BARE_CALL bare_isdos(void);
void BARE_CALL bare_reboot(void);
void BARE_CALL bare_goto_rombasic(void);

#ifdef __cplusplus
}
#endif

#endif

