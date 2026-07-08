// BME IO module header file

#ifndef BME_IO_H
#define BME_IO_H

int io_openlinkeddatafile(unsigned char *ptr);
void io_closelinkeddatafile();
int io_open(const char *name);
void io_close(int handle);

int io_lseek(int handle, int bytes, int whence);
int io_read(int handle, void *buffer, int size);
unsigned io_read8(int handle);
unsigned io_readle16(int handle);
unsigned io_readle32(int handle);
unsigned io_readhe16(int handle);
unsigned io_readhe32(int handle);

#endif
