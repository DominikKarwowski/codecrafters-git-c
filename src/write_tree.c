#include "write_tree.h"

#include <dirent.h>
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
    validate(create_tree(tree_data_buffer, &tree_data, hash), "Failed to create a blob object.");

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

typedef struct proc_dir
{
    char *path;
    DIR *dir_to_process;
    FILE *data_stream;
    buffer *buffer;
} proc_dir;

static void destroy_proc_dir(proc_dir *proc_dir)
{
    if (!proc_dir) return;

    if (proc_dir->path) free(proc_dir->path);
    if (proc_dir->dir_to_process) closedir(proc_dir->dir_to_process);
    if (proc_dir->data_stream) fclose(proc_dir->data_stream);

    if (proc_dir->buffer)
    {
        if (proc_dir->buffer->data) free(proc_dir->buffer->data);
        free(proc_dir->buffer);
    }

    free(proc_dir);
}

static bool push_dir_for_processing(Stack *dirs, char *path)
{
    DIR *dir = opendir(path);
    validate(dir, "Failed to open directory '%s'.", path);

    proc_dir *proc_dir = malloc(sizeof(struct proc_dir));
    validate(proc_dir, "Failed to allocate memory.");

    proc_dir->buffer = malloc(sizeof(buffer));
    validate(proc_dir->buffer, "Failed to allocate memory.");

    FILE *tree_content = open_memstream(&proc_dir->buffer->data, &proc_dir->buffer->size);
    validate(tree_content, "Failed to open memory stream.");

    proc_dir->path = path;
    proc_dir->dir_to_process = dir;
    proc_dir->data_stream = tree_content;

    Stack_push(dirs, proc_dir);

    return true;

error:
    if (dir) closedir(dir);
    if (tree_content) fclose(tree_content);
    destroy_proc_dir(proc_dir);

    return false;
}

static bool process_dir(Stack *dirs, proc_dir *proc_dir)
{
    struct dirent *dir_entry;
    while ((dir_entry = readdir(proc_dir->dir_to_process)) != NULL)
    {
        char *file_full_path = malloc(sizeof(char) * PATH_MAX);
        validate(file_full_path, "Failed to allocate memory");

        (void)snprintf(file_full_path, PATH_MAX, "%s/%s", proc_dir->path, dir_entry->d_name);

        struct stat fs;
        validate(stat(file_full_path, &fs) == 0, "Failed to stat file '%s'.", file_full_path);

        if (S_ISREG(fs.st_mode))
        {
            bool result = append_blob_entry(proc_dir->data_stream, dir_entry->d_name, fs.st_mode, file_full_path);
            validate(result, "Failed to write blob entry for '%s'.", file_full_path);
            free(file_full_path);
        }
        else if (S_ISDIR(fs.st_mode) && !is_excluded_dir(dir_entry->d_name))
        {
            Stack_push(dirs, proc_dir);

            bool result = push_dir_for_processing(dirs, file_full_path);
            validate(result, "Failed to push subdir '%s' on stack.", file_full_path);

            break;
        }
    }

    if (!dir_entry)
    {
        closedir(proc_dir->dir_to_process);
        proc_dir->dir_to_process = nullptr;

        fclose(proc_dir->data_stream);
        proc_dir->data_stream = nullptr;
    }

    return true;

error:
    destroy_proc_dir(proc_dir);

    return false;
}

static bool is_dir_fully_processed(const proc_dir *proc_dir)
{
    return proc_dir->buffer->size != 0;
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

    proc_dir *curr = nullptr;
    while (!Stack_is_empty(dirs))
    {
        curr = Stack_pop(dirs);
        validate(curr, "Failed to obtain data from the stack.");

        result = process_dir(dirs, curr);
        validate(result, "Failed to process directory entry.");

        if (is_dir_fully_processed(curr))
        {
            const proc_dir *parent = Stack_peek(dirs);

            if (parent)
            {
                append_tree_entry(parent->data_stream, get_dir_name(curr->path), curr->buffer);
            }
        }
    }

    char hash_hex[40];
    char *hash = write_tree_object(curr->buffer, hash_hex);
    validate(hash, "Failed to write tree.");

    printf("%s", hash_hex);

    destroy_proc_dir(curr);
    Stack_destroy(dirs, (StackElemCleaner)destroy_proc_dir);

    return 0;

error:
    if (repo_root_path) free(repo_root_path);
    if (dirs) Stack_destroy(dirs, (StackElemCleaner)destroy_proc_dir);

    return 1;
}
