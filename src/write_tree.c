#include "write_tree.h"

#include <limits.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <sys/stat.h>

#include "debug_helpers.h"
#include "git_dir_helpers.h"
#include "git_obj_helpers.h"
#include "stack.h"

static bool is_executable(const mode_t filemode)
{
    return S_IXOTH & filemode || S_IXGRP & filemode || S_IXUSR & filemode;
}

static bool append_blob_entry(
    FILE *tree_content,
    const char *dir_entry_name,
    const mode_t filemode,
    char *file_full_path)
{
    FILE *blob_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_blob(file_full_path, &blob_data, hash), "Failed to create a blob object.");

    const char *permissions = is_executable(filemode)
        ? "100755"
        : "100644";

    fwrite(permissions, sizeof(char), 6, tree_content);
    fputc(' ', tree_content);
    fwrite(dir_entry_name, sizeof(char), strlen(dir_entry_name), tree_content);
    fputc('\0', tree_content);
    fwrite(hash, sizeof(char), SHA_DIGEST_LENGTH, tree_content);

    fclose(blob_data);

    return true;

error:
    if (blob_data) fclose(blob_data);

    return false;
}

static bool append_tree_entry(
    FILE *parent_tree_content,
    const char *dir_entry_name,
    const buffer *tree_data_buffer)
{
    FILE *tree_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_tree(tree_data_buffer, &tree_data, hash), "Failed to create a tree object.");

    const char *permissions = "40000";

    fwrite(permissions, sizeof(char), 5, parent_tree_content);
    fputc(' ', parent_tree_content);
    fwrite(dir_entry_name, sizeof(char), strlen(dir_entry_name), parent_tree_content);
    fputc('\0', parent_tree_content);
    fwrite(hash, sizeof(char), SHA_DIGEST_LENGTH, parent_tree_content);

    return true;

error:
    if (tree_data) fclose(tree_data);

    return false;
}

typedef struct dir_processing_frame
{
    char *path;

    // dir_content
    struct dirent **dir_entries;
    int dir_entries_count;
    int current_dir_index;

    // git_dir_data
    FILE *data_stream;
    buffer *buffer;
} dir_processing_frame;

static void destroy_dir_processing_frame(dir_processing_frame *frame)
{
    if (!frame) return;

    if (frame->path) free(frame->path);

    if (frame->dir_entries)
    {
        for (int i = 0; i < frame->dir_entries_count; i++)
        {
            if (frame->dir_entries[i]) free(frame->dir_entries[i]);
        }

        free(frame->dir_entries);
    }

    if (frame->data_stream) fclose(frame->data_stream);

    if (frame->buffer)
    {
        if (frame->buffer->data) free(frame->buffer->data);
        free(frame->buffer);
    }

    free(frame);
}

static int include_dir(const struct dirent *dir_entry)
{
    if (is_excluded_dir(dir_entry))
    {
        return 0;
    }

    return 1;
}

static bool push_dir_for_processing(Stack *dirs, char *path)
{
    dir_processing_frame *frame = malloc(sizeof(dir_processing_frame));
    validate(frame, "Failed to allocate memory.");

    frame->path = path;

    frame->buffer = malloc(sizeof(buffer));
    validate(frame->buffer, "Failed to allocate memory.");
    frame->buffer->data = nullptr;
    frame->buffer->size = 0;

    frame->data_stream = open_memstream(&frame->buffer->data, &frame->buffer->size);
    validate(frame->data_stream, "Failed to open memory stream.");

    frame->current_dir_index = 0;
    frame->dir_entries_count = scandir(path, &frame->dir_entries, include_dir, alphasort);
    validate(frame->dir_entries_count != -1, "Failed to scan directory entries.");

    Stack_push(dirs, frame);

    return true;

error:
    if (frame->data_stream) fclose(frame->data_stream);
    destroy_dir_processing_frame(frame);

    return false;
}

static bool process_dir(Stack *dirs, dir_processing_frame *frame)
{
    while (frame->current_dir_index < frame->dir_entries_count)
    {
        char *file_full_path = malloc(sizeof(char) * PATH_MAX);
        validate(file_full_path, "Failed to allocate memory");

        const size_t dir_name_len = strlen(frame->dir_entries[frame->current_dir_index]->d_name);

        char dir_name[dir_name_len + 1];
        strncpy(dir_name, frame->dir_entries[frame->current_dir_index]->d_name, dir_name_len);
        dir_name[dir_name_len] = '\0';

        (void)snprintf(file_full_path, PATH_MAX, "%s/%s", frame->path, dir_name);

        struct stat fs;
        validate(stat(file_full_path, &fs) == 0, "Failed to stat file '%s'.", file_full_path);

        free(frame->dir_entries[frame->current_dir_index]);
        frame->dir_entries[frame->current_dir_index] = nullptr;
        frame->current_dir_index++;

        if (S_ISREG(fs.st_mode))
        {
            bool result = append_blob_entry(frame->data_stream, dir_name, fs.st_mode, file_full_path);
            validate(result, "Failed to write blob entry for '%s'.", file_full_path);
            free(file_full_path);
        }
        else if (S_ISDIR(fs.st_mode))
        {
            bool result = push_dir_for_processing(dirs, file_full_path);
            validate(result, "Failed to push subdir '%s' on stack.", file_full_path);

            break;
        }
    }

    return true;

error:
    destroy_dir_processing_frame(frame);

    return false;
}

static bool is_dir_fully_processed(const dir_processing_frame *frame)
{
    return frame->current_dir_index >= frame->dir_entries_count;
}

int write_tree()
{
    char *repo_root_path = malloc(PATH_MAX);
    validate(repo_root_path, "Failed to allocate memory");

    char *root = find_repository_root_dir(repo_root_path, PATH_MAX);
    validate(root, "Not a git repository.");

    Stack *dirs = Stack_create();

    bool result = push_dir_for_processing(dirs, root);
    validate(result, "Failed to push subdir '%s' on stack.", root);

    dir_processing_frame *curr = nullptr;
    while (!Stack_is_empty(dirs))
    {
        curr = Stack_peek(dirs);
        validate(curr, "Failed to obtain data from the stack.");

        if (is_dir_fully_processed(curr))
        {
            fclose(curr->data_stream);
            curr->data_stream = nullptr;

            (void)Stack_pop(dirs);

            const dir_processing_frame *parent = Stack_peek(dirs);

            if (parent)
            {
                append_tree_entry(parent->data_stream, get_dir_name(curr->path), curr->buffer);
            }
        }
        else
        {
            result = process_dir(dirs, curr);
            validate(result, "Failed to process directory entry.");
        }
    }

    char hash_hex[40];
    printf("%s", curr->buffer->data);
    char *hash = write_tree_object(curr->buffer, hash_hex);
    validate(hash, "Failed to write tree.");

    printf("%s", hash_hex);

    destroy_dir_processing_frame(curr);
    Stack_destroy(dirs, (StackElemCleaner)destroy_dir_processing_frame);

    return 0;

error:
    if (repo_root_path) free(repo_root_path);
    if (dirs) Stack_destroy(dirs, (StackElemCleaner)destroy_dir_processing_frame);

    return 1;
}
