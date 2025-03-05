#include "cat-file.h"
#include "debug_helper.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#define CHUNK 65536

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

bool try_resolve_cat_file_opts(const int argc, char **argv)
{
    opterr = 0;
    bool pretty_print = false;
    bool show_type = false;
    bool show_size = false;
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

static struct object_path get_obj_path(const char *obj_hash)
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

void print_inflate_result(FILE *source, FILE *dest)
{
    bool header_skipped = false;

    z_stream infstream = {
        .zalloc = Z_NULL,
        .zfree = Z_NULL,
        .opaque = Z_NULL,
        .avail_in = 0,
        .next_in = Z_NULL,
    };

    int ret = inflateInit(&infstream);
    validate(ret == Z_OK, "Failed to initialize inflate.");

    do
    {
        unsigned char in[CHUNK];
        infstream.avail_in = fread(in, 1, CHUNK, source);

        validate(ferror(source) == 0, "Failed to read source data.");

        if (infstream.avail_in == 0)
        {
            break;
        }

        infstream.next_in = in;

        do
        {
            unsigned char out[CHUNK];
            infstream.avail_out = CHUNK;
            infstream.next_out = out;
            ret = inflate(&infstream, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);

            // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
            switch (ret) // NOLINT(*-multiway-paths-covered)
            {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR; /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    validate(false, "Failed to inflate with Z error code: %d.", ret);
            }

            const unsigned have = CHUNK - infstream.avail_out;

            if (!header_skipped)
            {
                do
                {
                    // loop to advance through stream
                } while (fgetc(dest) != '\0');

                (void)fgetc(dest);

                header_skipped = true;
            }

            const size_t write_size = fwrite(out, 1, have, dest);

            validate(write_size == have, "Failed writing to output stream.");

        } while (infstream.avail_out == 0);

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&infstream);

    validate(ret == Z_STREAM_END, "Failed to inflate with Z error code: %d", Z_DATA_ERROR);
    return;

error:
    (void)inflateEnd(&infstream);
}

int cat_file(const int argc, char *argv[])
{
    const char *obj_hash = argv[3];

    validate(try_resolve_cat_file_opts(argc, argv), "Failed to resolve options.");

    struct object_path obj_path = get_obj_path(obj_hash);

    char full_obj_path[PATH_MAX];
    sprintf(full_obj_path, ".git/objects/%s/%s", obj_path.subdir, obj_path.name);

    FILE *obj_file = fopen(full_obj_path, "r");

    validate(obj_file, "Failed to open object file: %s", full_obj_path);

    print_inflate_result(obj_file, stdout);

    fclose(obj_file);

    return 0;

error:
    return 1;
}
