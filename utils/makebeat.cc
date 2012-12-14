#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

#include "../crc32.h"

static crc32_t out_crc = crc32_startvalue;

static void Output(const void* p, size_t n)
{
    const unsigned char* buf = reinterpret_cast<const unsigned char*>(p);
    std::fwrite(buf, 1, n, stdout);
    while(n--) out_crc = crc32_update(out_crc, *buf++);
}

static void OutL32(size_t n)
{
    unsigned char Buf[4] =
        { (unsigned char) ( n & 0xFF ),
          (unsigned char) ( (n >> 8) & 0xFF ),
          (unsigned char) ( (n >> 16) & 0xFF ),
          (unsigned char) ( (n >> 24) & 0xFF ) };
    Output(Buf, 4);
}
static void OutV(size_t data)
{
    unsigned char Buf[ (sizeof(data)*8 + 6) / 7 ];
    unsigned c = 0;
    for(;;)
    {
        unsigned char x = data & 0x7F; data >>= 7;
        if(data == 0) { Buf[c++] = 0x80 | x; break; }
        Buf[c++] = x;
        --data;
    }
    Output(Buf, c);
}
static unsigned VcostU(size_t n) // # bytes consumed by unsigned number
{
    unsigned result = 1;
    for(;;)
    {
        n >>= 7;
        if(!n) break;
        ++result;
        --n;
    }
    return result;
}
static unsigned VcostI(long n) // # bytes consumed by signed number
{
    if(n < 0) n = -n;
    return VcostU(n*2+1);
}

static size_t MatchingLength
    (const unsigned char* in1,
     size_t size,
     const unsigned char* in2)
{
    // Trivial implementation would be:
    //   return std::mismatch(in1,in1+size, in2).second-in2;
    // However this function attempts to gain something
    // by comparing sizeof(unsigned) bytes at time instead
    // of 1 byte at time.
    size_t len = 0;
    typedef unsigned long inttype;
    const size_t intsize = sizeof(inttype);
    const inttype*const p1 = reinterpret_cast<const inttype*>(in1);
    const inttype*const p2 = reinterpret_cast<const inttype*>(in2);
    while(size >= intsize && p1[len] == p2[len]) { ++len; size -= intsize; }
    len *= intsize;
    while(size > 0 && in1[len] == in2[len]) { --size; ++len; }
    return len;
}

int main(int argc, char** argv)
{
    bool optimize = false;

    if(argv[1][0]=='-' && argv[1][1] == 'O')
    {
        optimize = true;
        for(int p=1; p<argc; ++p) argv[p]=argv[p+1];
        --argc;
    }

    if(argc != 3)
    {
        std::printf("makebps: A simple Beat patch maker with next to no error checks\n"
               "Copyright (C) 1992,2012 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: makebeat [-O<optimizationlevel>] oldfile newfile > patch.beat\n"
               "Don't forget the \">\"!\n");
        return -1;
    }

    std::FILE *f1 = std::fopen(argv[1], "rb");
    std::FILE *f2 = std::fopen(argv[2], "rb");

    if(!f1) { std::perror(argv[1]); }
    if(!f2) { std::perror(argv[2]); }
    if(!f1 || !f2)
    {
        if(f1)std::fclose(f1);
        if(f2)std::fclose(f2);
        return -1;
    }

    std::fseek(f1, 0, SEEK_END); std::size_t d1size = std::ftell(f1);
    std::fseek(f2, 0, SEEK_END); std::size_t d2size = std::ftell(f2);

#if USE_MMAP
    unsigned char *d1 = (unsigned char *)mmap(NULL, d1size, PROT_READ, MAP_PRIVATE, fileno(f1), 0);
    unsigned char *d2 = (unsigned char *)mmap(NULL, d2size, PROT_READ, MAP_PRIVATE, fileno(f2), 0);
#else
    unsigned char *d1 = new unsigned char[d1size];
    std::rewind(f1); std::fread(d1, d1size, 1, f1);

    unsigned char *d2 = new unsigned char[d2size];
    std::rewind(f2); std::fread(d2, d2size, 1, f1);
#endif

    Output((const unsigned char*)"BPS1", 4);
    OutV(d1size);
    OutV(d2size);

    std::string Metadata = "<?xml version=\"1.0\"?><source>Chronotools</source>";
    OutV( Metadata.size() );
    Output(&Metadata[0], Metadata.size());

    uint_fast32_t crc1 = crc32_calc( (const unsigned char*) d1, d1size );
    uint_fast32_t crc2 = crc32_calc( (const unsigned char*) d2, d2size );

    static std::vector<std::size_t> d1map[0x10000];
    static std::vector<std::size_t> d2map[0x10000];

    long sourceRelativeOffset=0;
    long targetRelativeOffset=0;
    std::size_t targetReadLength=0;

    if(true)
    {
        // Precalculate hash counts, so we don't need to waste
        // time for reallocations during the main loop.

        std::size_t counts[0x10000] = { 0 };

        for(std::size_t a=0; a<d1size; ++a)
        {
            const unsigned symbol = d1[a] + ((a+1) < d1size ? 256*d1[a+1] : 0);
            ++counts[symbol];
        }
        for(unsigned a=0; a<0x10000; ++a)
            d1map[a].reserve(counts[a]);

        if(optimize)
        {
            for(unsigned a=0; a<0x10000; ++a)
                counts[a] = 0;

            for(std::size_t a=0; a<d2size; ++a)
            {
                const unsigned symbol = d2[a] + ((a+1) < d2size ? 256*d2[a+1] : 0);
                ++counts[symbol];
            }
            for(unsigned a=0; a<0x10000; ++a)
                d2map[a].reserve(counts[a]);
        }
    }

    for(std::size_t a=0; a<d1size; ++a)
    {
        const unsigned symbol = d1[a] + ((a+1) < d1size ? 256*d1[a+1] : 0);
        d1map[symbol].push_back(a);
    }

    enum { sourceRead, targetRead, sourceCopy, targetCopy } mode;

    for(std::size_t pos=0; pos<d2size; )
    {
        mode = targetRead;
        std::size_t maxLength = 1, maxOffset = 0;
        double best_cost = 1.0;

        const unsigned symbol = d2[pos] + (pos+1 < d2size ? 256*d2[pos+1] : 0);
        std::size_t remains = d2size-pos;

        // sourceRead
        if(pos < d1size)
        do{ std::size_t len = MatchingLength(&d1[pos], std::min(d2size-pos, d1size-pos), &d2[pos]);
          std::size_t benefit = len;
          std::size_t cost    = VcostU( (len-1)*4 + sourceRead );
          if(cost > benefit) continue;
          double relative_cost = double(cost) / double(benefit);
          if(relative_cost < best_cost)
          //if(len > maxLength)
            { best_cost = relative_cost;
              maxLength = len;
              mode = sourceRead; }
        }while(0);

        // targetCopy
        { for(std::size_t b=d2map[symbol].size(), a=b; a-- > 0; )
          {
              std::size_t offs = d2map[symbol][a];
              std::size_t len = MatchingLength(&d2[offs], d2size-pos, &d2[pos]);
              std::size_t benefit = len;
              std::size_t cost    = VcostU( (len-1)*4 + targetCopy )
                                  + VcostI( long(offs)-targetRelativeOffset );
              if(cost > benefit) continue;
              double relative_cost = double(cost) / double(benefit);
              if(relative_cost < best_cost)
              //if(len > maxLength)
                { best_cost = relative_cost;
                  maxLength = len;
                  maxOffset = offs;
                  mode = targetCopy;
                  if(len >= remains) break;
                }
        } }

        // sourceCopy
        { for(std::size_t b=d1map[symbol].size(), a=b; a-- > 0; )
          {
              std::size_t offs = d1map[symbol][a];
              std::size_t len = MatchingLength(&d1[offs], std::min(d2size-pos, d1size-offs), &d2[pos]);
              std::size_t benefit = len;
              std::size_t cost    = VcostU( (len-1)*4 + sourceCopy )
                                  + VcostI( long(offs)-sourceRelativeOffset );
              if(cost > benefit) continue;
              double relative_cost = double(cost) / double(benefit);
              if(relative_cost < best_cost)
              //if(len > maxLength)
                { best_cost = relative_cost;
                  maxLength = len;
                  maxOffset = offs;
                  mode = sourceCopy;
                  if(len >= remains) break;
                }
        } }

        /*fprintf(stderr, "@%7u Best %.4f, len %u, offs %u, mode %u\n",
            (unsigned)pos, best_cost, (unsigned)maxLength, (unsigned)maxOffset, mode);*/

        //if(maxLength < 4 && mode != sourceRead) { maxLength = 1; mode = targetRead; }

        if(mode != targetRead && targetReadLength > 0)
        {
            OutV(targetRead + 4*(targetReadLength-1));
            Output(&d2[pos - targetReadLength], targetReadLength);
            targetReadLength=0;
        }

        long r;
        switch(mode)
        {
            case sourceRead:
                OutV(sourceRead + 4*(maxLength-1));
                break;
            case targetRead:
                targetReadLength += maxLength;
                break;
            case sourceCopy:
                OutV(sourceCopy + 4*(maxLength-1));
                r = maxOffset - sourceRelativeOffset;
                OutV(r < 0 ? (-r*2 + 1) : (r*2));
                sourceRelativeOffset += r + maxLength;
                break;
            case targetCopy:
                OutV(targetCopy + 4*(maxLength-1));
                r = maxOffset - targetRelativeOffset;
                OutV(r < 0 ? (-r*2 + 1) : (r*2));
                targetRelativeOffset += r + maxLength;
                break;
        }

        std::size_t oldpos = pos;
        pos += maxLength;

        if(pos < d2size)
        for(std::size_t p = oldpos; p < pos; ++p)
        {
            const unsigned symbol = d2[p] + ((p+1) < d2size ? 256*d2[p+1] : 0);
            d2map[symbol].push_back(p);
            if(!optimize)
            {
                break; // <--Remove this line to improve compression
            }
        }
    }

    if(targetReadLength > 0)
    {
        OutV(targetRead + 4*(targetReadLength-1));
        Output(&d2[d2size - targetReadLength], targetReadLength);
        targetReadLength=0;
    }

    OutL32(crc1 ^ crc32_startvalue);
    OutL32(crc2 ^ crc32_startvalue);
    OutL32(out_crc ^ crc32_startvalue);

#ifdef USE_MMAP
    munmap(d1, ftell(f1)); std::fclose(f1);
    munmap(d2, ftell(f2)); std::fclose(f2);
#else
    delete[] d1; std::fclose(f1);
    delete[] d2; std::fclose(f2);
#endif

    std::fflush(stdout);

    return 0;
}
