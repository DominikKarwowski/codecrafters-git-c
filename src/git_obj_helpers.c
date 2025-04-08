#include "git_obj_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

int get_header_size(const char *content)
{
    int i = 0;

    while (content[i] != '\0') i++;

    return i;
}

size_t get_object_content(const char *obj_hash, char **inflated_buffer)
{
    struct object_path obj_path = get_object_path(obj_hash);

    char repo_root[PATH_MAX];
    find_repository_root_dir(repo_root, PATH_MAX);

    char git_obj_path[PATH_MAX];
    sprintf(git_obj_path, "%s/.git/objects/%s/%s", repo_root, obj_path.subdir, obj_path.name);

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
