//
// Datafile -> C include file
//

#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: dat2inc <datafile> <includefile>\n\n");
        return EXIT_FAILURE;
    }
    FILE *in = std::fopen(argv[1], "rb");
    if (!in)
    {
        printf("Datafile open error!\n");
        return EXIT_FAILURE;
    }
    FILE *out = std::fopen(argv[2], "wt");
    if (!out)
    {
        printf("Includefile open error!\n");
        std::fclose(in);
        return EXIT_FAILURE;
    }
    std::fseek(in, 0, SEEK_END);
    int length = ftell(in);
    std::fseek(in, 0, SEEK_SET);

    fprintf(out, "unsigned char datafile[] = {\n");
    for (int c = 0; c < length; c++)
    {
        if (c)
        {
            fprintf(out, ", ");
            if (!(c % 10)) fprintf(out, "\n");
        }
        fprintf(out, "0x%02x", fgetc(in));
    }
    fprintf(out, "};\n");
    std::fclose(in);
    std::fclose(out);
    return EXIT_SUCCESS;
}
