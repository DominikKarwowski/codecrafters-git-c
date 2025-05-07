#ifndef COMPRESSION_H
#define COMPRESSION_H
#include <stdio.h>

#define CHUNK 65536

void deflate_object(FILE *source, FILE *dest);

void inflate_object(FILE *source, FILE *dest);

#endif //COMPRESSION_H
