
#ifndef GIT_OBJ_HELPERS_H
#define GIT_OBJ_HELPERS_H
#include <stddef.h>

int get_header_size(const char *content);

size_t get_object_content(const char *obj_hash, char **inflated_buffer);

#endif //GIT_OBJ_HELPERS_H
