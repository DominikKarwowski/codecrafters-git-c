#include "git_dir_helpers.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "debug_helper.h"

struct object_path get_object_path(const char *obj_hash)
{
    struct object_path obj_path;

    int i;
    int j;

    for (i = 0; i < 2; i++)
    {
        obj_path.subdir[i] = obj_hash[i];
    }

    obj_path.subdir[i] = '\0';

    for (j = 0; j < 38; j++, i++)
    {
        obj_path.name[j] = obj_hash[i];
    }

    obj_path.name[j] = '\0';

    return obj_path;
}

static bool is_root_dir(const char *path)
{
    const size_t len = strlen(path);
    return len == 1 && path[0] == '/';
}

static char *get_parent_dir(char *path, const size_t path_len)
{
    validate(path_len > 0, "Invalid path length provided. Must be a non-negative value.");
    validate(!is_root_dir(path), "Provided directory is already a root directory.");

    char *p = strrchr(path, '/');
    validate(p, "Failed to find parent directory.");
    *p = '\0';

    return path;

error:
    return nullptr;
}

static bool has_git_subdir(const char *path)
{
    bool found = false;

    DIR *dir = opendir(path);
    validate(dir, "Failed to open directory.");

    struct dirent *entry ;

    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".git") == 0)
        {
            found = true;
            break;
        }
    }

    closedir(dir);

    return found;

error:
    if (dir) closedir(dir);
    return false;
}

char *find_repository_root_dir(char *root_path, const size_t root_path_len)
{
    char *curr = getcwd(root_path, root_path_len);
    validate(curr, "Failed to get current working directory.");

    do
    {
        if (has_git_subdir(root_path))
        {
            break;
        }

        printf("%s\n", root_path);

        const size_t curr_len = strlen(root_path);
        const char *p = get_parent_dir(root_path, curr_len);
        validate(p, "Failed to get parent directory.");

    } while (true);

    return root_path;

error:
    return nullptr;
}

bool dir_exists(const char *path)
{
    struct stat fs;
    const int result = stat(path, &fs);

    if (result == 0 && S_ISDIR(fs.st_mode))
    {
        return true;
    }

    return false;
}
