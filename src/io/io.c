#include "io.h"

#include <err.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void read_file(char *file_name, char *buff)
{
    FILE *fptr = fopen(file_name, "r");
    if (fptr == NULL)
        errx(1, "not a file");
    int i = 0;
    int c = fgetc(fptr);
    while (c != EOF)
    {
        buff[i] = (char)c;
        i += 1;
        c = fgetc(fptr);
    }
    fclose(fptr);
}

void read_input(char *buff)
{
    int i = 0;
    int c = getchar();
    while (c != EOF)
    {
        buff[i] = (char)c;
        i += 1;
        c = getchar();
    }
}
