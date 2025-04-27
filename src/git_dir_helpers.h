#ifndef OBJECT_FILE_HELPERS_H
#define OBJECT_FILE_HELPERS_H

#include <dirent.h>

struct object_path
{
    char subdir[3];
    char name[39];
};

struct object_path get_object_path(const char *obj_hash);

char *find_repository_root_dir(char *root_path, size_t root_path_len);

bool dir_exists(const char *path);

const char *get_dir_name(const char *path);

bool is_excluded_dir(const struct dirent *dir_entry);

#endif //OBJECT_FILE_HELPERS_H
