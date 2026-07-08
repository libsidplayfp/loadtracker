#include <cstdint>
#include <cstdio>

void fwrite8(FILE *file, unsigned data)
{
    uint8_t bytes[1];

    bytes[0] = data;
    std::fwrite(bytes, 1, 1, file);
}

void fwritele16(FILE *file, unsigned data)
{
    uint8_t bytes[2];

    bytes[0] = data;
    bytes[1] = data >> 8;
    std::fwrite(bytes, 2, 1, file);
}

void fwritele32(FILE *file, unsigned data)
{
    uint8_t bytes[4];

    bytes[0] = data;
    bytes[1] = data >> 8;
    bytes[2] = data >> 16;
    bytes[3] = data >> 24;
    std::fwrite(bytes, 4, 1, file);
}

unsigned fread8(FILE *file)
{
    uint8_t bytes[1];

    std::fread(bytes, 1, 1, file);
    return bytes[0];
}
