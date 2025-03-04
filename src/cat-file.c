#include "cat-file.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "zlib.h"

#define INFLATE_BUFFER 65536

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

    int opt;
    while ((opt = getopt(argc, argv, "tps")) != -1)
    {
        switch (opt)
        {
            case 't':
                if (!try_set_cat_file_opt(&show_type, "-t", pretty_print, "-p", show_size, "-s"))
                {
                    return false;
                }
                break;
            case 'p':
                if (!try_set_cat_file_opt(&pretty_print, "-p", show_type, "-t", show_size, "-s"))
                {
                    return false;
                }
                break;
            case 's':
                if (!try_set_cat_file_opt(&show_size, "-s", show_type, "-t", pretty_print, "-p"))
                {
                    return false;
                }
                break;
            case '?':
                printf("Invalid switch: '%c'\n", optopt);
                break;
            default:
                fprintf(stderr, "Unrecognized option: %c\n", optopt);
        }
    }

    return true;
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

    for (j = 0; j < 18; j++, i++)
    {
        obj_path.name[j] = obj_hash[i];
    }

    obj_path.name[j] = '\0';

    return obj_path;
}

int cat_file(const int argc, char *argv[])
{
    const char *obj_hash = argv[3];

    if (!try_resolve_cat_file_opts(argc, argv))
    {
        return 1;
    }

    struct object_path obj_path = get_obj_path(obj_hash);

    char full_obj_path[PATH_MAX];
    sprintf(full_obj_path, ".git/objects/%s/%s", obj_path.subdir, obj_path.name);

    z_stream infstream = {
        .zalloc = Z_NULL,
        .zfree = Z_NULL,
        .opaque = Z_NULL,
    };

    FILE *obj_file = fopen(full_obj_path, "r");

    if (!obj_file)
    {
        return 1;
    }

    if (inflateInit(&infstream) != Z_OK)
    {
        fclose(obj_file);
        return 1;
    }

    int inflate_result = 0;
    char *in = malloc(INFLATE_BUFFER);
    char *out = nullptr;
    int out_factor = 1;

    do
    {
        const auto new_out = realloc(out, INFLATE_BUFFER * out_factor++);

        if (!new_out)
        {
            free(out);
            fclose(obj_file);
            inflateEnd(&infstream);
            return 1;
        }

        out = new_out;

        const size_t n = fread(in, sizeof(unsigned char), INFLATE_BUFFER, obj_file);
        infstream.avail_in = n;
        infstream.next_in = (Bytef *)in;

        infstream.avail_out = INFLATE_BUFFER;
        infstream.next_out = (Bytef *)out;

        inflate_result = inflate(&infstream, Z_NO_FLUSH);
    } while (inflate_result != Z_STREAM_END);

    // const int term_idx = INFLATE_BUFFER * out_factor + (INFLATE_BUFFER - infstream.avail_out);
    // out[term_idx] = '\0';

    fclose(obj_file);

    if (inflateEnd(&infstream) != Z_OK)
    {
        return 1;
    }

    printf("%s\n", out);

    return 0;
}
