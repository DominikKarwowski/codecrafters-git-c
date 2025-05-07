#ifndef GIT_OBJ_HELPERS_H
#define GIT_OBJ_HELPERS_H

#include <stddef.h>
#include <stdio.h>
#include <openssl/sha.h>

#define SHA_HEX_LENGTH 40

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

typedef struct commit_info
{
    char *tree_sha;
    char *parent_sha;
    char *author_name;
    char *author_email;
    char *author_date;
    char *author_timezone;
    char *committer_name;
    char *committer_email;
    char *committer_date;
    char *commiter_timezone;
    char *message;
} commit_info;

void init_commit_tree_info(commit_info *commit_opts);

void destroy_commit_tree_info(const commit_info *commit_opts);

int get_header_size(const char *content);

void hash_bytes_to_hex(char *hash_hex, const unsigned char *hash);

size_t get_object_content(const char *obj_hash, char **inflated_buffer);

void get_object_type(char *obj_type, const char* object_content);

unsigned char *create_blob(char *filename, FILE **blob_data, unsigned char hash[SHA_DIGEST_LENGTH]);

unsigned char *create_tree(const buffer *tree_buffer, FILE **tree_data, unsigned char hash[SHA_DIGEST_LENGTH]);

char *write_blob_object(char *filename, char *hash_hex);

char *write_tree_object(const buffer *tree_buffer, char *hash_hex);

char *write_commit_object(const commit_info *commit_info, char *hash_hex);

#endif //GIT_OBJ_HELPERS_H
