#ifndef GIT_OBJ_HELPERS_H
#define GIT_OBJ_HELPERS_H

#include <stddef.h>
#include <stdio.h>

int get_header_size(const char *content);

void hash_bytes_to_hex(char *hash_hex, const unsigned char *hash);

size_t get_object_content(const char *obj_hash, char **inflated_buffer);

void get_object_type(char *obj_type, const char* object_content);

char *create_blob(char *filename, FILE *blob_data, char *hash_hex);

char *write_blob(char *filename, char *hash_hex);

#endif //GIT_OBJ_HELPERS_H
