#include "git_obj_helpers.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/stat.h>

#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

void init_commit_tree_info(commit_info *commit_opts)
{
    commit_opts->tree_sha = nullptr;
    commit_opts->parent_sha = nullptr;
    commit_opts->author_name = nullptr;
    commit_opts->author_email = nullptr;
    commit_opts->author_date = nullptr;
    commit_opts->author_timezone = nullptr;
    commit_opts->committer_name = nullptr;
    commit_opts->committer_email = nullptr;
    commit_opts->committer_date = nullptr;
    commit_opts->commiter_timezone = nullptr;
    commit_opts->message = nullptr;
}

void destroy_commit_tree_info(const commit_info *commit_opts)
{
    if (commit_opts->tree_sha) free(commit_opts->tree_sha);
    if (commit_opts->parent_sha) free(commit_opts->parent_sha);
    if (commit_opts->message) free(commit_opts->message);
    if (commit_opts->author_name) free(commit_opts->author_name);
    if (commit_opts->author_email) free(commit_opts->author_email);
    if (commit_opts->author_date) free(commit_opts->author_date);
    if (commit_opts->author_timezone) free(commit_opts->author_timezone);
    if (commit_opts->committer_name) free(commit_opts->committer_name);
    if (commit_opts->committer_email) free(commit_opts->committer_email);
    if (commit_opts->committer_date) free(commit_opts->committer_date);
    if (commit_opts->commiter_timezone) free(commit_opts->commiter_timezone);
}

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
        printf("%02x ", hash[i]);
    }
}

size_t get_object_content(const char *obj_hash, char **inflated_buffer)
{
    struct object_path obj_path = get_object_path(obj_hash);

    char *repo_root = malloc(sizeof(char) * PATH_MAX);
    validate(repo_root, "Failed to allocate memory.");

    find_repository_root_dir(repo_root, PATH_MAX);

    char *git_obj_path = malloc(sizeof(char) * PATH_MAX);
    validate(git_obj_path, "Failed to allocate memory.");

    (void)snprintf(
        git_obj_path,
        PATH_MAX,
        "%s/.git/objects/%s/%s",
        repo_root,
        obj_path.subdir,
        obj_path.name);

    FILE *obj_file = fopen(git_obj_path, "r");
    validate(obj_file, "Failed to open object file: %s", git_obj_path);

    size_t inflated_buffer_size;
    FILE *obj_inflated = open_memstream(inflated_buffer, &inflated_buffer_size);
    validate(obj_inflated, "Failed to allocate memory for object content.");

    inflate_object(obj_file, obj_inflated);

    free(repo_root);
    free(git_obj_path);
    fclose(obj_file);
    fclose(obj_inflated);

    return inflated_buffer_size;

error:
    if (repo_root) free(repo_root);
    if (git_obj_path) free(git_obj_path);
    if (obj_inflated) fclose(obj_inflated);
    if (obj_file) fclose(obj_file);
    if (inflated_buffer) free(inflated_buffer);

    return 0;
}

void get_object_type(char *obj_type, const char *object_content)
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

unsigned char *create_blob(char *filename, FILE **blob_data, unsigned char hash[SHA_DIGEST_LENGTH])
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

    *blob_data = fmemopen(NULL, blob_size, "r+");
    validate(*blob_data, "Failed to allocate memory for blob_data");

    size_t write_size = fwrite(blob_header, sizeof(char), header_size, *blob_data);
    validate(write_size == header_size, "Failed to write blob header.");

    char copy_buffer[BUFSIZ];
    size_t n;
    while ((n = fread(copy_buffer, sizeof(char), sizeof(copy_buffer), src_file)) > 0)
    {
        write_size = fwrite(copy_buffer, sizeof(char), n, *blob_data);
        validate(write_size == n, "Failed to write blob content.");
    }

    fclose(src_file);
    rewind(*blob_data);

    unsigned char *result = calculate_hash(*blob_data, blob_size, hash);
    validate(result, "Failed to calculate blob hash.");

    return hash;

error:
    if (src_file) fclose(src_file);
    if (blob_data) fclose(*blob_data);

    return nullptr;
}

unsigned char *create_tree(const buffer *tree_buffer, FILE **tree_data, unsigned char hash[SHA_DIGEST_LENGTH])
{
    char tree_header[24];

    const int header_size = sprintf(tree_header, "tree %lu", tree_buffer->size) + 1;
    const size_t tree_size = header_size + tree_buffer->size;

    *tree_data = fmemopen(NULL, tree_size, "r+");
    validate(*tree_data, "Failed to allocate memory for tree_data");

    size_t write_size = fwrite(tree_header, sizeof(char), header_size, *tree_data);
    validate(write_size == header_size, "Failed to write tree header.");

    write_size = fwrite(tree_buffer->data, sizeof(char), tree_buffer->size, *tree_data);
    validate(write_size == tree_buffer->size, "Failed to write tree content.");

    rewind(*tree_data);

    unsigned char *result = calculate_hash(*tree_data, tree_size, hash);
    validate(result, "Failed to calculate tree hash.");

    return hash;

error:
    if (*tree_data) fclose(*tree_data);

    return nullptr;
}

unsigned char *create_commit(const commit_info *commit_info, FILE **commit_data, unsigned char hash[SHA_DIGEST_LENGTH])
{
    buffer buffer;
    FILE *commit_content = open_memstream(&buffer.data, &buffer.size);
    validate(commit_content, "Failed to allocate memory for commit_content");

    size_t content_size = fwrite("tree ", sizeof(char), 5, commit_content);
    content_size += fwrite(commit_info->tree_sha, sizeof(char), SHA_HEX_LENGTH, commit_content);
    content_size += fwrite("\0", sizeof(char), 1, commit_content);

    if (commit_info->parent_sha)
    {
        content_size += fwrite("parent ", sizeof(char), 7, commit_content);
        content_size += fwrite(commit_info->parent_sha, sizeof(char), SHA_HEX_LENGTH, commit_content);
        content_size += fwrite("\0", sizeof(char), 1, commit_content);
    }

    content_size += fwrite("author ", sizeof(char), 7, commit_content);
    content_size += fwrite(commit_info->author_name, sizeof(char), strlen(commit_info->author_name), commit_content);
    content_size += fwrite(" <", sizeof(char), 2, commit_content);
    content_size += fwrite(commit_info->author_email, sizeof(char), strlen(commit_info->author_email), commit_content);
    content_size += fwrite("< >", sizeof(char), 2, commit_content);
    content_size += fwrite(commit_info->author_date, sizeof(char), strlen(commit_info->author_date), commit_content);
    content_size += fwrite(" ", sizeof(char), 1, commit_content);
    content_size += fwrite(commit_info->author_timezone, sizeof(char), strlen(commit_info->author_timezone), commit_content);
    content_size += fwrite("\0", sizeof(char), 1, commit_content);

    content_size += fwrite("committer ", sizeof(char), 10, commit_content);
    content_size += fwrite(commit_info->author_name, sizeof(char), strlen(commit_info->author_name), commit_content);
    content_size += fwrite(" <", sizeof(char), 2, commit_content);
    content_size += fwrite(commit_info->author_email, sizeof(char), strlen(commit_info->author_email), commit_content);
    content_size += fwrite("< >", sizeof(char), 2, commit_content);
    content_size += fwrite(commit_info->author_date, sizeof(char), strlen(commit_info->author_date), commit_content);
    content_size += fwrite(" ", sizeof(char), 1, commit_content);
    content_size += fwrite(commit_info->author_timezone, sizeof(char), strlen(commit_info->author_timezone), commit_content);
    content_size += fwrite("\0\0", sizeof(char), 2, commit_content);

    content_size += fwrite(commit_info->message, sizeof(char), strlen(commit_info->message), commit_content);
    fputc('\0', commit_content);

    rewind(commit_content);

    char commit_header[24];

    const int header_size = sprintf(commit_header, "commit %lu", content_size) + 1;
    const size_t commit_size = header_size + content_size;

    *commit_data = fmemopen(NULL, commit_size, "r+");
    validate(*commit_data, "Failed to allocate memory for commit_data");

    size_t write_size = fwrite(commit_header, sizeof(char), header_size, *commit_data);
    validate(write_size == header_size, "Failed to write tree header.");

    char copy_buffer[BUFSIZ];
    size_t n;
    while ((n = fread(copy_buffer, sizeof(char), sizeof(copy_buffer), commit_content)) > 0)
    {
        write_size = fwrite(copy_buffer, sizeof(char), n, *commit_data);
        validate(write_size == n, "Failed to write blob content.");
    }

    rewind(*commit_data);

    unsigned char *result = calculate_hash(*commit_data, commit_size, hash);
    validate(result, "Failed to calculate tree hash.");

    fclose(commit_content);
    free(buffer.data);

    return hash;

error:
    if (buffer.data) free(buffer.data);
    if (commit_content) fclose(commit_content);

    return nullptr;
}

static char *write_git_object(char *hash_hex, FILE *object_data, unsigned char hash[20])
{
    hash_bytes_to_hex(hash_hex, hash);

    printf("Get path...\n");
    struct object_path path = get_object_path(hash_hex);

    char *repo_root_path = malloc(sizeof(char) * PATH_MAX);
    validate(repo_root_path, "Failed to allocate memory.");

    printf("Find root...\n");
    char *root = find_repository_root_dir(repo_root_path, PATH_MAX);
    validate(root, "Not a git repository.");

    char *full_path = malloc(sizeof(char) * PATH_MAX);
    validate(full_path, "Failed to allocate memory.");

    const int size = snprintf(full_path, PATH_MAX, "%s/.git/objects/%s", root, path.subdir);
    validate(size <= PATH_MAX, "Failed to generate object path for '%s'. Exceeded PATH_MAX", path.subdir);

    if (!dir_exists(full_path))
    {
        const int mkdir_result = mkdir(full_path, 0755);
        validate(mkdir_result == 0, "Failed to create directory '%s'.", full_path);
    }

    strcat(full_path, "/");
    strcat(full_path, path.name);

    printf("Deflating...\n");
    FILE *deflated_file = fopen(full_path, "w+");
    validate(deflated_file, "Failed to open file '%s'.", full_path);

    deflate_object(object_data, deflated_file);

    free(repo_root_path);
    free(full_path);
    fclose(deflated_file);

    return hash_hex;

error:
    if (repo_root_path) free(repo_root_path);
    if (root) free(root);
    if (deflated_file) fclose(deflated_file);

    return nullptr;
}

char *write_blob_object(char *filename, char *hash_hex)
{
    FILE *blob_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_blob(filename, &blob_data, hash), "Failed to create a blob object.");

    validate(write_git_object(hash_hex, blob_data, hash), "Failed to write a blob object.");

    fclose(blob_data);

    return hash_hex;

error:
    if (blob_data) fclose(blob_data);

    return nullptr;
}

char *write_tree_object(const buffer *tree_buffer, char *hash_hex)
{
    FILE *tree_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_tree(tree_buffer, &tree_data, hash), "Failed to create a tree object.");

    validate(write_git_object(hash_hex, tree_data, hash), "Failed to write a tree object.");

    fclose(tree_data);

    return hash_hex;

error:
    if (tree_data) fclose(tree_data);

    return nullptr;
}

char *write_commit_object(const commit_info *commit_info, char *hash_hex)
{
    FILE *commit_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_commit(commit_info, &commit_data, hash), "Failed to create a commit object.");

    validate(write_git_object(hash_hex, commit_data, hash), "Failed to write a commit object.");

    fclose(commit_data);

    return hash_hex;

error:
    if (commit_data) fclose(commit_data);
    return nullptr;
}
