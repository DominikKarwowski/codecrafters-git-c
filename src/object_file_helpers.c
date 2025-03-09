#include "object_file_helpers.h"

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
