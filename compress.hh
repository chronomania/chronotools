#ifndef bqtCTcompressHH
#define bqtCTcompressHH

#include <vector>

using std::vector;

/* Chrono Trigger data compression functions
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 */

unsigned Uncompress                 /* Return value: compressed size */
    (const unsigned char *Memory,   /* Pointer to the compressed data */
     vector<unsigned char>& Target  /* Where to decompress to */
    );

const vector<unsigned char> Compress /* Return value: compressed data */
    (const unsigned char *data, unsigned length, /* Data to be compressed */
     unsigned Segment,   /* 7E or 7F */
     unsigned Depth      /* 11 or 12 */
     );

const vector<unsigned char> Compress /* Return value: compressed data */
    (const unsigned char *data, unsigned length, /* Data to be compressed */
     unsigned char seg   /* 7E or 7F */
    );

#endif