
#ifndef BME_END_H
#define BME_END_H

#ifdef __cplusplus
extern "C" {
#endif

void fwrite8(FILE *file, unsigned data);
void fwritele16(FILE *file, unsigned data);
void fwritele32(FILE *file, unsigned data);
unsigned fread8(FILE *file);
unsigned freadle16(FILE *file);
unsigned freadle32(FILE *file);
unsigned freadhe16(FILE *file);
unsigned freadhe32(FILE *file);

#ifdef __cplusplus
}
#endif

#endif
