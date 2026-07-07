// BME mouse functions header file

#ifndef BME_MOU__H
#define BME_MOU__H

#ifdef __cplusplus
extern "C" {
#endif

void mou_init();
void mou_getpos(unsigned *x, unsigned *y);
unsigned mou_getbuttons();

#ifdef __cplusplus
}
#endif

#endif
