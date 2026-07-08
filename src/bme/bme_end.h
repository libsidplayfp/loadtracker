
#ifndef BME_END_H
#define BME_END_H

#include <cstdio>

void fwrite8(FILE *file, unsigned data);
void fwritele16(FILE *file, unsigned data);
void fwritele32(FILE *file, unsigned data);
unsigned fread8(FILE *file);

#endif
