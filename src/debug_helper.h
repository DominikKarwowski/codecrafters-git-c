#ifndef DEBUG_HELPER_H
#define DEBUG_HELPER_H

#include <errno.h>
#include <string.h>

#define errno_desc() \
    (errno == 0 ? "No error number" : strerror(errno))

#define log_error(M, ...) \
    fprintf(stderr,\
        "%s:%d: errno: %s; " M "\n", \
        __FILE__, __LINE__, errno_desc(), ##__VA_ARGS__)

#define validate(A, M, ...) \
({  __typeof__ (A) a__type_of = (A); \
    if(!a__type_of) { \
        log_error(M, ##__VA_ARGS__); \
        errno = 0; \
        goto error; } })

#endif //DEBUG_HELPER_H
