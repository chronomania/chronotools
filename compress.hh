#ifndef bqtCTcompressHH
#define bqtCTcompressHH

#include <vector>

using std::vector;
using std::size_t;

/* Chrono Trigger data compression functions
 * Copyright (C) 1992,2004 Bisqwit (http://iki.fi/bisqwit/)
 */

size_t Uncompress                 /* Return value: compressed size */
    (const unsigned char *Memory,   /* Pointer to the compressed data */
     vector<unsigned char>& Target, /* Where to decompress to */
     const unsigned char *End = 0
    );

const vector<unsigned char> Compress /* Return value: compressed data */
    (const unsigned char *data, size_t length, /* Data to be compressed */
     unsigned Depth      /* 11 or 12 */
     );

const vector<unsigned char> Compress /* Return value: compressed data */
    (const unsigned char *data, size_t length  /* Data to be compressed */
    );

#endif
