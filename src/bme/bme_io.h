// BME IO module header file

#ifndef BME_IO_H
#define BME_IO_H

#ifdef __cplusplus
extern "C" {
#endif

int io_open(const char *name);
int io_lseek(int handle, int bytes, int whence);
int io_read(int handle, void *buffer, int size);
void io_close(int handle);
int io_opendatafile(const char *name);
int io_openlinkeddatafile(unsigned char *ptr);
void io_setfilemode(int usedf);
unsigned io_read8(int handle);
unsigned io_readle16(int handle);
unsigned io_readle32(int handle);
unsigned io_readhe16(int handle);
unsigned io_readhe32(int handle);

#ifdef __cplusplus
}
#endif

#endif
