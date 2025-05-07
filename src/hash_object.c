#include "hash_object.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "git_obj_helpers.h"

#include "debug_helpers.h"

bool write_opt = false;

static bool try_resolve_hash_object_opts(const int argc, char *argv[])
{
    opterr = 0;

    int opt;
    while ((opt = getopt(argc, argv, "w")) != -1)
    {
        switch (opt)
        {
            case 'w':
                write_opt = true;
                break;
            case '?':
                validate(false, "Invalid switch: '%c'\n", optopt);
            default:
                validate(false, "Unrecognized option: '%c'\n", optopt);
        }
    }

    return true;

error:
    return false;
}

int hash_object(const int argc, char *argv[])
{
    char *filename = argv[3];

    bool opt_result = try_resolve_hash_object_opts(argc, argv);
    validate(opt_result, "Failed to resolve options.");

    if (write_opt)
    {
        char hash_hex[SHA_HEX_LENGTH];
        char *hash = write_blob_object(filename, hash_hex);
        validate(hash, "Failed to write blob.");

        printf("%s", hash_hex);
    }

    return 0;

error:
    return 1;
}
