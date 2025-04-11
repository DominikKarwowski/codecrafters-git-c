#include "write_tree.h"

#include <dirent.h>
#include <limits.h>
#include <bits/struct_stat.h>
#include <openssl/sha.h>
#include <sys/stat.h>

#include "debug_helpers.h"
#include "git_dir_helpers.h"
#include "git_obj_helpers.h"
#include "stack.h"

static bool is_executable(const mode_t filemode)
{
    return S_IXOTH & filemode || S_IXGRP & filemode || S_IXGRP & filemode;
}

static bool write_blob_entry(
    FILE *tree_content,
    const char *dir_entry_name,
    const mode_t filemode,
    char file_full_path[PATH_MAX])
{
    FILE *blob_data = nullptr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    validate(create_blob(file_full_path, blob_data, hash), "Failed to create a blob object.");

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

static bool process_dir_entry(FILE *tree_content, Stack *dirs, char *path)
{
    DIR *dir = opendir(path);
    validate(dir, "Failed to open '%s' directory.", path);

    struct dirent *dir_entry;
    while ((dir_entry = readdir(dir)) != NULL)
    {
        char file_full_path[PATH_MAX];
        snprintf(file_full_path, PATH_MAX, "%s/%s", path, dir_entry->d_name);

        struct stat fs;
        validate(stat(file_full_path, &fs) == 0, "Failed to stat file '%s'.", file_full_path);

        if (S_ISREG(fs.st_mode))
        {
            bool result = write_blob_entry(tree_content, dir_entry->d_name, fs.st_mode, file_full_path);
            validate(result, "Failed to write blob entry for '%s'.", file_full_path);
        }
        else if (S_ISDIR(fs.st_mode))
        {
            Stack_push(dirs, file_full_path);
        }
    }

    closedir(dir);
    return true;

error:
    if (dir) closedir(dir);

    return false;
}

int write_tree(int argc, char *argv[])
{
    char repo_root_path[PATH_MAX];
    char *root = find_repository_root_dir(repo_root_path, PATH_MAX);
    validate(root, "Not a git repository.");

    char *tree_content_buffer = nullptr;
    size_t tree_content_buffer_size;
    FILE *tree_content = open_memstream(&tree_content_buffer, &tree_content_buffer_size);

    Stack *dirs = Stack_create();
    Stack_push(dirs, root);

    while (!Stack_is_empty(dirs))
    {
        char *path = Stack_pop(dirs);
        validate(path, "Failed to obtain path from the queue.");

        bool result = process_dir_entry(tree_content, dirs, path);
        validate(result, "Failed to process directory entry '%s'.", path);
    }

    char git_objects_path[PATH_MAX];
    (void)sprintf(git_objects_path, "%s/%s", root, repo_root_path);

    fclose(tree_content);

    // deflate tree obj
    //

    Stack_destroy(dirs);

    // write tree object

    return 0;

error:
    if (dirs) Stack_destroy(dirs);
    if (tree_content) fclose(tree_content);

    return 1;
}
