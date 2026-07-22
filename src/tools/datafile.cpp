//
// Datafile creator
//

#include "bme_end.h"

#include <new>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

constexpr int MAXFILES = 16384;
constexpr int MAXFILENAME = 64;

struct Header
{
    uint32_t offset;
    uint32_t length;
    char name[13];
};

static Header header[MAXFILES];
static char fullname[MAXFILES][MAXFILENAME];
static int files;

bool addfile(Header *header, FILE *dest, char *name);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::printf("Usage: DATAFILE <datafile> <filelistfile>\n\n"
               "The purpose of this program is to gather many files into one datafile, like\n"
               "usually seen in games. The files can be read by BME's IO functions after\n"
               "opening the datafile first with io_opendatafile(). The filelistfile must\n"
               "contain name (use only 8+3 chars) of each file on its own row, for example:\n\n"
               "editor.spr\n"
               "fonts.spr\n");
        return EXIT_SUCCESS;
    }
    FILE *listfile = std::fopen(argv[2], "rt");
    if (!listfile)
    {
        std::printf("ERROR: Couldn't open filelist.\n");
        return EXIT_FAILURE;
    }
    FILE *datafile = std::fopen(argv[1], "wb");
    if (!datafile)
    {
        std::printf("ERROR: Couldn't create datafile.\n");
        std::fclose(listfile);
        return EXIT_FAILURE;
    }
    std::memset(&header[0], 0, MAXFILES * sizeof(Header));
    // Get names from list
    for (;;)
    {
        char searchname[64];

        if (std::fscanf(listfile, "%63s", searchname) == EOF) break;
        FILE *test = std::fopen(searchname, "rb");
        if (test)
        {
            std::fclose(test);

            std::strcpy(fullname[files], searchname);

            int c;
            for (c = std::strlen(fullname[files]) - 1; c >= 0; c--)
            {
                if (fullname[files][c] == '\\')
                {
                    c++;
                    break;
                }
            }
            if (c < 0) c = 0;
            std::memset(header[files].name, 0, 13);
            int d = 0;
            while (fullname[files][c])
            {
                header[files].name[d] = std::toupper(fullname[files][c]);
                c++;
                d++;
            }
            files++;
            if (files == MAXFILES) break;
        }
        if (files == MAXFILES) break;
    }

    std::fclose(listfile);
    // Write datafile header
    std::fwrite("DAT!", 4, 1, datafile);
    fwritele32(datafile, files);
    // Write incomplete fileheaders
    for (int c = 0; c < files; c++)
    {
        fwritele32(datafile, header[c].offset);
        fwritele32(datafile, header[c].length);
        std::fwrite(header[c].name, 13, 1, datafile);
    }
    // Process each file
    for (int c = 0; c < files; c++)
    {
        std::printf("Adding %s...\n", header[c].name);
        if (!addfile(&header[c], datafile, fullname[c]))
        {
            std::printf("Terminating & deleting datafile...\n");
            std::fclose(datafile);
            remove(argv[1]);
            return EXIT_FAILURE;
        }
    }
    // Seek back to start & write correct headers
    std::fseek(datafile, sizeof files + 4, SEEK_SET);
    for (int c = 0; c < files; c++)
    {
        fwritele32(datafile, header[c].offset);
        fwritele32(datafile, header[c].length);
        std::fwrite(header[c].name, 13, 1, datafile);
    }
    std::fclose(datafile);
    std::printf("Everything OK!\n");
    return EXIT_SUCCESS;
}

bool addfile(Header *header, FILE *dest, char *name)
{
    FILE *src = std::fopen(name, "rb");
    if (!src)
    {
        std::printf("ERROR: Couldn't open file %s\n", name);
        return false;
    }
    header->offset = std::ftell(dest);
    std::fseek(src, 0, SEEK_END);
    header->length = std::ftell(src);
    std::fseek(src, 0, SEEK_SET);
    unsigned char *originalbuf = new (std::nothrow) unsigned char[header->length];
    if (!originalbuf)
    {
        std::printf("ERROR: No memory to load file!\n");
        std::fclose(src);
        return false;
    }
    std::printf("* Loading file\n");
    std::fread(originalbuf, header->length, 1, src);
    std::fclose(src);
    std::printf("* Writing file (size was %d)\n", header->length);
    std::fwrite(originalbuf, header->length, 1, dest);
    delete [] originalbuf;
    return true;
}
