#include "cat_file.h"
#include "compression.h"
#include "debug_helpers.h"
#include "git_dir_helpers.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "git_obj_helpers.h"

bool pretty_print_opt = false;
bool show_type_opt = false;
bool show_size_opt = false;

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
                set_result = try_set_cat_file_opt(&show_type_opt, "-t", pretty_print_opt, "-p", show_size_opt, "-s");
                validate(set_result, "Failed to set 't' option.");
                break;
            case 'p':
                set_result = try_set_cat_file_opt(&pretty_print_opt, "-p", show_type_opt, "-t", show_size_opt, "-s");
                validate(set_result, "Failed to set 'p' option.");
                break;
            case 's':
                set_result = try_set_cat_file_opt(&show_size_opt, "-s", show_type_opt, "-t", pretty_print_opt, "-p");
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
    char *inflated_buffer = nullptr;

    (void)get_object_content(obj_hash, &inflated_buffer);
    validate(inflated_buffer, "Failed to obtain object content.");

    const int header_size = get_header_size(inflated_buffer);

    if (pretty_print_opt)
    {
        printf("%s", &inflated_buffer[header_size + 1]);
    }

    if (show_type_opt)
    {
        int i = 0;
        while (inflated_buffer[i] != ' ')
        {
            putc(inflated_buffer[i++], stdout);
        }
        putc('\n', stdout);
    }

    free(inflated_buffer);

    return 0;

error:
    if (inflated_buffer) free(inflated_buffer);

    return 1;
}
