#ifndef MY_NECIO_H
#define MY_NECIO_H

# ifdef __cplusplus
extern "C" {
# endif

void __watcall nec98_iowait(void);
void __watcall nec98_gdc_cmd(unsigned port_base, unsigned char cmd);
unsigned short __watcall nec98_gdc_readdata(unsigned port_base);


# ifdef __cplusplus
}
# endif


#endif

