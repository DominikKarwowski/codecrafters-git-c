#include "hash_object.h"

#include "debug_helpers.h"
#include "git_dir_helpers.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>
#include <openssl/sha.h>
#include <sys/stat.h>

#include "compression.h"
#include "git_obj_helpers.h"

bool write_opt = false;

static bool try_resolve_hash_object_opts(const int argc, char *argv[])
{
    opterr = 0;

    int opt;
    while ((opt = getopt(argc, argv, "w")) != -1)
    {
        switch (opt)
        {
            case 'w':
                write_opt = true;
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: '%c'\n", optopt);
        }
    }

    return true;

error:
    return false;
}

static unsigned char *calculate_hash(FILE *source, const size_t src_size, unsigned char *hash)
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

static int write_blob(char *filename)
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

    FILE *blob_data = fmemopen(NULL, blob_size, "r+");
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

    rewind(blob_data);

    unsigned char hash[SHA_DIGEST_LENGTH];

    unsigned char *result = calculate_hash(blob_data, blob_size, hash);
    validate(result, "Failed to calculate hash.");

    char hash_hex[40];
    hash_bytes_to_hex(hash_hex, hash);

    struct object_path path = get_object_path(hash_hex);

    char repo_root_path[PATH_MAX];
    char *root = find_repository_root_dir(repo_root_path, PATH_MAX);
    validate(root, "Not a git repository.");

    char full_path[PATH_MAX];
    (void)sprintf(full_path, "%s/.git/objects/%s", root, path.subdir);

    if (!dir_exists(full_path))
    {
        const int mkdir_result = mkdir(full_path, 0755);
        validate(mkdir_result == 0, "Failed to create directory '%s'.", full_path);
    }

    strcat(full_path, "/");
    strcat(full_path, path.name);

    FILE *deflated_file = fopen(full_path, "w+");
    validate(deflated_file, "Failed to open file '%s'.", full_path);

    rewind(blob_data);
    deflate_blob(blob_data, deflated_file);

    fclose(src_file);
    fclose(blob_data);
    fclose(deflated_file);

    printf("%s", hash_hex);

    return 0;

error:
    if (src_file) fclose(src_file);
    if (blob_data) fclose(blob_data);
    if (deflated_file) fclose(deflated_file);

    return 1;
}

int hash_object(const int argc, char *argv[])
{
    char *filename = argv[3];

    bool opt_result = try_resolve_hash_object_opts(argc, argv);
    validate(opt_result, "Failed to resolve options.");

    if (write_opt)
    {
        write_blob(filename);
    }

    return 0;

error:
    return 1;
}
