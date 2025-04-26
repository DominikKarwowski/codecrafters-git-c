#ifndef GIT_OBJ_HELPERS_H
#define GIT_OBJ_HELPERS_H

#include <stddef.h>
#include <stdio.h>
#include <openssl/sha.h>

typedef struct git_tree_node
{
    char *mode;
    char *name;
    unsigned char *hash;
} git_tree_node;

typedef struct buffer
{
    char *data;
    size_t size;
} buffer;

int get_header_size(const char *content);

void hash_bytes_to_hex(char *hash_hex, const unsigned char *hash);

size_t get_object_content(const char *obj_hash, char **inflated_buffer);

void get_object_type(char *obj_type, const char* object_content);

unsigned char *create_blob(char *filename, FILE *blob_data, unsigned char hash[SHA_DIGEST_LENGTH]);

unsigned char *create_tree(const buffer *tree_buffer, FILE *tree_data, unsigned char hash[SHA_DIGEST_LENGTH]);

char *write_blob_object(char *filename, char *hash_hex);

char *write_tree_object(const buffer *tree_buffer, char *hash_hex);

#endif //GIT_OBJ_HELPERS_H
