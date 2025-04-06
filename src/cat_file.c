#include "cat_file.h"
#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

bool pretty_print = false;
bool show_type = false;
bool show_size = false;

static bool try_set_cat_file_opt(
    bool *opt,
    const char *opt_name,
    const bool is_other1_set,
    const char *other1_name,
    const bool is_other2_set,
    const char *other2_name)
{
    const auto msg = "error: %s is incompatible with %s";

    if (is_other1_set)
    {
        printf(msg, other1_name, opt_name);
        return false;
    }

    if (is_other2_set)
    {
        printf(msg, other2_name, opt_name);
        return false;
    }

    *opt = true;
    return true;
}

static bool try_resolve_cat_file_opts(const int argc, char **argv)
{
    opterr = 0;
    bool set_result;

    int opt;
    while ((opt = getopt(argc, argv, "tps")) != -1)
    {
        switch (opt)
        {
            case 't':
                set_result = try_set_cat_file_opt(&show_type, "-t", pretty_print, "-p", show_size, "-s");
                validate(set_result, "Failed to set 't' option.");
                break;
            case 'p':
                set_result = try_set_cat_file_opt(&pretty_print, "-p", show_type, "-t", show_size, "-s");
                validate(set_result, "Failed to set 'p' option.");
                break;
            case 's':
                set_result = try_set_cat_file_opt(&show_size, "-s", show_type, "-t", pretty_print, "-p");
                validate(set_result, "Failed to set 's' option.");
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

int cat_file(const int argc, char *argv[])
{
    validate(try_resolve_cat_file_opts(argc, argv), "Failed to resolve options.");

    const char *obj_hash = argv[3];

    if (pretty_print)
    {
        struct object_path obj_path = get_object_path(obj_hash);

        char full_obj_path[PATH_MAX];
        sprintf(full_obj_path, ".git/objects/%s/%s", obj_path.subdir, obj_path.name);

        FILE *obj_file = fopen(full_obj_path, "r");

        validate(obj_file, "Failed to open object file: %s", full_obj_path);

        inflate_object(obj_file, stdout, CONTENT);

        fclose(obj_file);
    }

    return 0;

error:
    return 1;
}
