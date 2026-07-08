//
// BME (Blasphemous Multimedia Engine) datafile IO main module
//

#include "bme_io.h"

#include "bme_main.h"

#include <SDL3/SDL.h>

#include <new>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

// Up to 16 simultaneous files open from the datafile
constexpr int MAX_HANDLES = 16;

typedef struct
{
    Uint32 offset;
    Sint32 length;
    char name[13];
} HEADER;

typedef struct
{
    HEADER *currentheader;
    int filepos;
    bool open;
} HANDLE;

static const char *idstring = "DAT!";

static bool io_datafileopen = false;
static HEADER *fileheaders;
static unsigned files;
static HANDLE handle[MAX_HANDLES];
static FILE *datafilehandle = nullptr;
static unsigned char *datafileptr;
static unsigned char *datafilestart;

static void linkedseek(unsigned pos);
static void linkedread(void *buffer, int length);
static unsigned linkedreadle32(void);

int io_openlinkeddatafile(unsigned char *ptr)
{
    if (datafilehandle) std::fclose(datafilehandle);
    datafilehandle = nullptr;

    datafilestart = ptr;
    linkedseek(0);

    char ident[4];
    linkedread(ident, 4);
    if (std::memcmp(ident, idstring, 4))
    {
        bme_error = BME_WRONG_FORMAT;
        return BME_ERROR;
    }

    files = linkedreadle32();
    fileheaders = new (std::nothrow) HEADER[files];
    if (!fileheaders)
    {
        bme_error = BME_OUT_OF_MEMORY;
        return BME_ERROR;
    }
    for (unsigned index = 0; index < files; index++)
    {
        fileheaders[index].offset = linkedreadle32();
        fileheaders[index].length = linkedreadle32();
        linkedread(&fileheaders[index].name, 13);
    }

    for (unsigned index = 0; index < MAX_HANDLES; index++) handle[index].open = false;
    io_datafileopen = true;
    bme_error = BME_OK;
    return BME_OK;
}

void io_closelinkeddatafile()
{
    delete [] fileheaders;
}

// Returns nonnegative file handle if successful, -1 on error

int io_open(const char *name)
{
    if (!name || !io_datafileopen) return -1;

    size_t namelength = std::strlen(name);
    if (namelength > 12) namelength = 12;
    char namecopy[13];
    std::memcpy(namecopy, name, namelength + 1);
    for (size_t index = 0; index < std::strlen(namecopy); index++)
    {
        namecopy[index] = toupper(namecopy[index]);
    }

    for (size_t index = 0; index < MAX_HANDLES; index++)
    {
        if (!handle[index].open)
        {
            int count = files;
            handle[index].currentheader = fileheaders;

            while (count)
            {
                if (!std::strcmp(namecopy, handle[index].currentheader->name))
                {
                        handle[index].open = true;
                        handle[index].filepos = 0;
                        return index;
                }
                count--;
                handle[index].currentheader++;
            }
            return -1;
        }
    }
    return -1;
}

// Returns file position after seek or -1 on error

int io_lseek(int index, int offset, int whence)
{
    if (!io_datafileopen)  return -1;

    if ((index < 0) || (index >= MAX_HANDLES)) return -1;

    if (!handle[index].open) return -1;
    int newpos;
    switch(whence)
    {
        default:
        case SEEK_SET:
        newpos = offset;
        break;

        case SEEK_CUR:
        newpos = offset + handle[index].filepos;
        break;

        case SEEK_END:
        newpos = offset + handle[index].currentheader->length;
        break;
    }
    if (newpos < 0) newpos = 0;
    if (newpos > handle[index].currentheader->length) newpos = handle[index].currentheader->length;
    handle[index].filepos = newpos;
    return newpos;
}

// Returns number of bytes actually read, -1 on error

int io_read(int index, void *buffer, int length)
{
    if (!io_datafileopen) return -1;

    if ((index < 0) || (index >= MAX_HANDLES)) return -1;

    if (!handle[index].open) return -1;
    if (length + handle[index].filepos > handle[index].currentheader->length)
    length = handle[index].currentheader->length - handle[index].filepos;

    int readbytes;
    if (datafilehandle)
    {
        std::fseek(datafilehandle, handle[index].currentheader->offset + handle[index].filepos, SEEK_SET);
        readbytes = std::fread(buffer, 1, length, datafilehandle);
    }
    else
    {
        linkedseek(handle[index].currentheader->offset + handle[index].filepos);
        linkedread(buffer, length);
        readbytes = length;
    }
    handle[index].filepos += readbytes;
    return readbytes;
}

// Returns nothing

void io_close(int index)
{
    if (io_datafileopen)
    {
        if ((index < 0) || (index >= MAX_HANDLES)) return;

        handle[index].open = false;
    }
}

unsigned io_read8(int index)
{
    unsigned char byte;

    io_read(index, &byte, 1);
    return byte;
}

unsigned io_readle16(int index)
{
    unsigned char bytes[2];

    io_read(index, bytes, 2);
    return (bytes[1] << 8) | bytes[0];
}

unsigned io_readhe16(int index)
{
    unsigned char bytes[2];

    io_read(index, bytes, 2);
    return (bytes[0] << 8) | bytes[1];
}

unsigned io_readle32(int index)
{
    unsigned char bytes[4];

    io_read(index, bytes, 4);
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

unsigned io_readhe32(int index)
{
    unsigned char bytes[4];

    io_read(index, bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

static void linkedseek(unsigned pos)
{
    datafileptr = &datafilestart[pos];
}

static void linkedread(void *buffer, int length)
{
    unsigned char *dest = (unsigned char *)buffer;
    while (length--)
    {
        *dest++ = *datafileptr++;
    }
}

static unsigned linkedreadle32()
{
    unsigned char bytes[4];

    linkedread(&bytes, 4);
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}
