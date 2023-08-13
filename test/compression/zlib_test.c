

#include "zlib.h"
#include "stream.h"

uint8_t Wikipedia[] = {120, 156, 11, 207, 204, 206, 44, 72, 77, 201, 76, 4, 0, 17, 230, 3, 152};

int main()
{
    stream *compressed = stream_init(20);
    stream_write_bytes(compressed, Wikipedia, 17, false);
    stream *decompressed = stream_init(10);
    zlib_decompress(compressed, decompressed);
    stream_print(decompressed);

    stream_free(compressed);
    stream_free(decompressed);
    return 0;
}
