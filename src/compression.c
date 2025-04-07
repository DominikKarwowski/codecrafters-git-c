#include "compression.h"

#include <assert.h>
#include <zlib.h>

#include "debug_helpers.h"

void deflate_blob(FILE *source, FILE *dest)
{
    z_stream defstream = {
        .zalloc = Z_NULL,
        .zfree = Z_NULL,
        .opaque = Z_NULL,
    };

    int ret = deflateInit(&defstream, Z_DEFAULT_COMPRESSION);
    validate(ret == Z_OK, "Failed to initialize deflate.");

    int flush;

    do
    {
        unsigned char in[CHUNK];
        defstream.avail_in = fread(in, 1, CHUNK, source);

        validate(ferror(source) == 0, "Failed to read source data.");

        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        defstream.next_in = in;

        do
        {
            unsigned char out[CHUNK];
            defstream.avail_out = CHUNK;
            defstream.next_out = out;
            ret = deflate(&defstream, flush);
            assert(ret != Z_STREAM_ERROR);

            const unsigned have = CHUNK - defstream.avail_out;
            const size_t write_size = fwrite(out, 1, have, dest);
            validate(write_size == have, "Failed writing to output stream.");
            validate(ferror(dest) == 0, "Failed writing to output stream.");

        } while (defstream.avail_out == 0);

        assert(defstream.avail_in == 0);

    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);

    (void)deflateEnd(&defstream);
    return;

    error:
        (void)deflateEnd(&defstream);
}

void inflate_object(FILE *source, FILE *dest)
{
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
            const size_t write_size = fwrite(out, 1, have, dest);
            validate(write_size == have || ferror(dest) == 0, "Failed writing to output stream.");

        } while (infstream.avail_out == 0);

    } while (ret != Z_STREAM_END);

    validate(ret == Z_STREAM_END, "Failed to inflate with Z error code: %d", Z_DATA_ERROR);

    (void)inflateEnd(&infstream);

    return;

error:
    (void)inflateEnd(&infstream);
}
