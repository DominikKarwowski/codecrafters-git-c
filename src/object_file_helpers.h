#ifndef OBJECT_FILE_HELPERS_H
#define OBJECT_FILE_HELPERS_H

#define CHUNK 65536

struct object_path
{
    char subdir[3];
    char name[39];
};

struct object_path get_object_path(const char* obj_hash);

#endif //OBJECT_FILE_HELPERS_H
