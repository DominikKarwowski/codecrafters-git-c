#include "compression.h"

#include <assert.h>
#include <zlib.h>

#include "debug_helpers.h"
#include "git_dir_helpers.h"

static int get_header_size(const unsigned char *out)
{
    int i = 0;

    while (out[i] != '\0')
    {
        i++;
    }

    return i;
}

void inflate_object(FILE *source, FILE *dest, const section sect)
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

            unsigned have = CHUNK - infstream.avail_out;

            if (sect == CONTENT)
            {
                if (!header_skipped)
                {
                    const int header_size_terminated = get_header_size(out) + 1;

                    header_skipped = true;

                    have = have - header_size_terminated;
                    const size_t write_size = fwrite(&out[header_size_terminated], 1, have, dest);
                    validate(write_size == have || ferror(dest) == 0, "Failed writing to output stream.");
                }
                else
                {
                    const size_t write_size = fwrite(out, 1, have, dest);
                    validate(write_size == have || ferror(dest) == 0, "Failed writing to output stream.");
                }
            }
            else if (sect == HEADER)
            {
                have = get_header_size(out);

                const size_t write_size = fwrite(out, 1, have, dest);
                validate(write_size == have || ferror(dest) == 0, "Failed writing to output stream.");

                break;
            }

        } while (infstream.avail_out == 0);

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&infstream);

    validate(ret == Z_STREAM_END, "Failed to inflate with Z error code: %d", Z_DATA_ERROR);
    return;

error:
    (void)inflateEnd(&infstream);
}
