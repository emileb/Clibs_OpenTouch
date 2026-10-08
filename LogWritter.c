
#include <stdio.h>

#include "LogWritter.h"


static FILE *file;

void LogWritter_Init(const char *filename)
{
    file = fopen(filename, "w");
}

void LogWritter_Write(const char *msg)
{
    if (!file)
        return;
    fprintf(file, "%s\n", msg);
    fflush(file);
}

void LogWritter_Close()
{
    if (file)
        fclose(file);
    file = NULL;
}