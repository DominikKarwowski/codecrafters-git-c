#include "git_obj_helpers.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/stat.h>

#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

int get_header_size(const char *content)
{
    int i = 0;

    while (content[i] != '\0') i++;

    return i;
}

void hash_bytes_to_hex(char *hash_hex, const unsigned char *hash)
{
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&hash_hex[2 * i], "%02x", hash[i]);
    }
}

size_t get_object_content(const char *obj_hash, char **inflated_buffer)
{
    struct object_path obj_path = get_object_path(obj_hash);

    char repo_root[PATH_MAX];
    find_repository_root_dir(repo_root, PATH_MAX);

    char git_obj_path[PATH_MAX];
    snprintf(git_obj_path, PATH_MAX, "%s/.git/objects/%s/%s", repo_root, obj_path.subdir, obj_path.name);

    FILE *obj_file = fopen(git_obj_path, "r");
    validate(obj_file, "Failed to open object file: %s", git_obj_path);

    size_t inflated_buffer_size;
    FILE *obj_inflated = open_memstream(inflated_buffer, &inflated_buffer_size);
    validate(obj_inflated, "Failed to allocate memory for object content.");

    inflate_object(obj_file, obj_inflated);

    fclose(obj_file);
    fclose(obj_inflated);

    return inflated_buffer_size;

error:
    if (obj_inflated) fclose(obj_inflated);
    if (obj_file) fclose(obj_file);
    if (inflated_buffer) free(inflated_buffer);

    return 0;
}

void get_object_type(char *obj_type, const char* object_content)
{
    int i = 0;

    while (object_content[i] != ' ')
    {
        obj_type[i] = object_content[i];
        i++;
    }

    obj_type[i] = '\0';
}

static unsigned char *calculate_hash(FILE *source, const size_t src_size, unsigned char hash[20])
{
    unsigned char *input = malloc(src_size);
    validate(input, "Failed to allocate memory for input.");

    (void)fread(input, 1, src_size, source);
    rewind(source);

    unsigned char *result = SHA1(input, src_size, hash);
    validate(result, "Failed to compute hash.");

    free(input);

    return result;

error:
    if (input) free(input);

    return nullptr;
}

unsigned char *create_blob(char *filename, FILE *blob_data, unsigned char hash[SHA_DIGEST_LENGTH])
{
    FILE *src_file = fopen(filename, "r");
    validate(src_file, "Failed to open file: %s", filename);

    struct stat fs;
    const int stat_result = stat(filename, &fs);
    validate(stat_result == 0, "Failed to stat file: %s", filename);

    const size_t content_size = fs.st_size;

    char blob_header[24];

    const int header_size = sprintf(blob_header, "blob %lu", content_size) + 1;
    const size_t blob_size = header_size + content_size;

    blob_data = fmemopen(NULL, blob_size, "r+");
    validate(blob_data, "Failed to allocate memory for blob_data");

    size_t write_size = fwrite(blob_header, sizeof(char), header_size, blob_data);
    validate(write_size == header_size, "Failed to write blob header.");

    char copy_buffer[BUFSIZ];
    size_t n;
    while ((n = fread(copy_buffer, sizeof(char), sizeof(copy_buffer), src_file)) > 0)
    {
        write_size = fwrite(copy_buffer, sizeof(char), n, blob_data);
        validate(write_size == n, "Failed to write blob content.");
    }

    fclose(src_file);
    rewind(blob_data);

    unsigned char *result = calculate_hash(blob_data, blob_size, hash);
    validate(result, "Failed to calculate hash.");

    return hash;

error:
    if (src_file) fclose(src_file);
    if (blob_data) fclose(blob_data);

    return nullptr;
}

char *write_blob(char *filename, char *hash_hex)
{
    FILE *blob_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_blob(filename, blob_data, hash), "Failed to create a blob object.");

    hash_bytes_to_hex(hash_hex, hash);

    struct object_path path = get_object_path(hash_hex);

    char repo_root_path[PATH_MAX];
    char *root = find_repository_root_dir(repo_root_path, PATH_MAX);
    validate(root, "Not a git repository.");

    char full_path[PATH_MAX];
    (void)snprintf(full_path, PATH_MAX, "%s/.git/objects/%s", root, path.subdir);

    if (!dir_exists(full_path))
    {
        const int mkdir_result = mkdir(full_path, 0755);
        validate(mkdir_result == 0, "Failed to create directory '%s'.", full_path);
    }

    strcat(full_path, "/");
    strcat(full_path, path.name);

    FILE *deflated_file = fopen(full_path, "w+");
    validate(deflated_file, "Failed to open file '%s'.", full_path);

    deflate_blob(blob_data, deflated_file);

    fclose(blob_data);
    fclose(deflated_file);

    return hash_hex;

error:
    if (blob_data) fclose(blob_data);
    if (deflated_file) fclose(deflated_file);

    return nullptr;
}
