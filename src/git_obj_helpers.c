#include "git_obj_helpers.h"

int get_header_size(const char *content)
{
    int i = 0;

    while (content[i] != '\0') i++;

    return i;
}
