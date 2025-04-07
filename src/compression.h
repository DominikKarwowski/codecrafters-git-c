#ifndef COMPRESSION_H
#define COMPRESSION_H
#include <stdio.h>

void inflate_object(FILE *source, FILE *dest);

void inflate_header(FILE *source, FILE *dest);

#endif //COMPRESSION_H
