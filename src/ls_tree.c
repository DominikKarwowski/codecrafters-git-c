#include "ls_tree.h"

#include <getopt.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

#define GIT_OBJ_HEADER_SIZE 64

bool name_only = false;

static bool try_resolve_ls_tree_opts(const int argc, char **argv)
{
    opterr = 0;
    int opt;

    const struct option long_opts[] = {
        { "name-only", no_argument, nullptr, 'n' },
        { nullptr, 0, nullptr, 0 }
    };

    while ((opt = getopt_long(argc, argv, "", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'n':
                name_only = true;
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: %c\n", optopt);
        }
    }

    return true;

error:
    return false;
}

int ls_tree(const int argc, char *argv[])
{
    validate(try_resolve_ls_tree_opts(argc, argv), "Failed to resolve options.");

    const char* tree_hash = argv[argc - 1];

    struct object_path obj_path = get_object_path(tree_hash);

    char repo_root[PATH_MAX];
    find_repository_root_dir(repo_root, PATH_MAX);

    char git_obj_path[PATH_MAX];
    sprintf(git_obj_path, "%s/.git/objects/%s/%s", repo_root, obj_path.subdir, obj_path.name);

    FILE *obj_file = fopen(git_obj_path, "r");
    validate(obj_file, "Failed to open object file: %s", git_obj_path);

    FILE *header = fmemopen(NULL, GIT_OBJ_HEADER_SIZE, "r+");
    validate(header, "Failed to allocate memory for header.");

    inflate_object(obj_file, header);

    rewind(header);
    rewind(obj_file);

    unsigned char obj_type[5];
    for (int i = 0; i < 4; i++)
    {
        obj_type[i] = fgetc(header);
    }

    obj_type[4] = '\0';

    validate(strcmp(obj_type, "tree") == 0, "Invalid object type: %s", (char *)obj_type);

    char *content_buffer;
    size_t content_buffer_size;
    FILE *content = open_memstream(&content_buffer, &content_buffer_size);
    validate(content != NULL, "Failed to allocate memory for object content.");

    inflate_object(obj_file, content);

    fclose(header);
    fclose(obj_file);
    fclose(content);


    if (content_buffer) free(content_buffer);

    return 0;

error:
    if (obj_file) fclose(obj_file);
    if (header) fclose(header);
    if (content) fclose(content);
    if (content_buffer) free(content_buffer);

    return 1;
}
