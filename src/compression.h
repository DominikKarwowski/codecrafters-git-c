#ifndef COMPRESSION_H
#define COMPRESSION_H
#include <stdio.h>

typedef enum
{
    HEADER,
    CONTENT,
} section;

void inflate_object(FILE *source, FILE *dest, section sect);

#endif //COMPRESSION_H
