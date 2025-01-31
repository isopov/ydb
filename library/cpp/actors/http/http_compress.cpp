#include "http.h"

#include <zlib.h>

namespace NHttp {

TString THttpOutgoingResponse::CompressDeflate(TStringBuf source) {
    int compressionlevel = Z_BEST_COMPRESSION;
    z_stream zs = {};

    if (deflateInit(&zs, compressionlevel) != Z_OK) {
        throw yexception() << "deflateInit failed while compressing";
    }

    zs.next_in = (Bytef*)source.data();
    zs.avail_in = source.size();

    int ret;
    char outbuffer[32768];
    TString result;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (result.size() < zs.total_out) {
            result.append(outbuffer, zs.total_out - result.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw yexception() << "Exception during zlib compression: (" << ret << ") " << zs.msg;
    }
    return result;
}

}
