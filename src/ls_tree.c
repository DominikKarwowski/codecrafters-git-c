#include "ls_tree.h"

#include <getopt.h>

#include "debug_helpers.h"

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



    return 0;

error:
    return 1;
}
