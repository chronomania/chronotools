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

#define PRECALC 0

#include "../crc32.h"

#ifdef __GNUC__
# define likely(x)       __builtin_expect(!!(x), 1)
# define unlikely(x)     __builtin_expect(!!(x), 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
#endif

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
    return VcostU( n<0 ? (-n*2+1) : (n*2) );
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

#define HASH_SIZE  0x100000
#define HASH_LIMIT 0x100000

static unsigned HASH(const unsigned char* buffer, std::size_t pos, std::size_t size)
{
  #if 1
    unsigned result = buffer[pos];
    if(unlikely(pos+1 >= size)) return result;
    result += (buffer[pos+1] << 8);
    if(unlikely(pos+2 >= size)) return result;
    result += ((buffer[pos+2] & 0xF) << 16);
    return result;
  #else
    unsigned char byte1 = buffer[pos];
    unsigned result     = byte1;
    if(pos+1 < size) result += 0x100 * buffer[pos+1];
    unsigned p=1, q=0;
    for(; pos+p < size && buffer[pos+p] == byte1; ++p) { }
    while(p > 1) { ++q; p >>= 1; }
    if(q > 15) q = 15;
    result += 0x10000 * q;
    return result;
  #endif
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
               "Usage: makebeat [-O<optimizationlevel>] oldfile newfile > patch.bps\n"
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

    Output("BPS1", 4);
    OutV(d1size);
    OutV(d2size);

    std::string Metadata = "<?xml version=\"1.0\"?><source>Chronotools</source>";
    OutV( Metadata.size() );
    Output(&Metadata[0], Metadata.size());

    uint_fast32_t crc1 = crc32_calc( (const unsigned char*) d1, d1size );
    uint_fast32_t crc2 = crc32_calc( (const unsigned char*) d2, d2size );

    /*
    #define HASH_SIZE 0x10000
    #define HASH(buffer,pos,size) \
        (buffer[pos] \
        + (((pos+1) < (size)) ? 256*buffer[(pos)+1] : 0) \
        )
    */

    static std::vector<std::size_t> d1map[HASH_SIZE];
    static std::vector<std::size_t> d2map[HASH_SIZE];

    long sourceRelativeOffset=0;
    long targetRelativeOffset=0;
    std::size_t targetReadLength=0;

    if(true)
    {
        // Precalculate hash counts, so we don't need to waste
        // time for reallocations during the main loop.
        unsigned limit = HASH_LIMIT;
        std::vector<std::size_t> counts(limit);

        for(std::size_t a=0; a<d1size; ++a)
        {
            const unsigned symbol = HASH(&d1[0],a,d1size);
            ++counts[symbol];
        }
        for(unsigned a=0; a<limit; ++a)
            d1map[a].reserve(counts[a]);

        if(optimize)
        {
            std::memset(&counts[0], 0, counts.size()*sizeof(counts[0]));

            for(std::size_t a=0; a<d2size; ++a)
            {
                const unsigned symbol = HASH(&d2[0],a,d2size);
                ++counts[symbol];
            }
            for(unsigned a=0; a<limit; ++a)
                d2map[a].reserve(counts[a]);
        }
    }

    for(std::size_t a=0; a<d1size; ++a)
    {
        const unsigned symbol = HASH(&d1[0],a,d1size);
        d1map[symbol].push_back(a);
    }

    enum { sourceRead, targetRead, sourceCopy, targetCopy } mode;

#if PRECALC
    fprintf(stderr, "Hashing 2...\n");
    for(std::size_t a=0; a<d2size; ++a)
    {
        const unsigned symbol = HASH(&d2[0],a,d2size);
        d2map[symbol].push_back(a);
    }

    std::vector< std::pair<std::size_t,std::size_t> > Best_D1match(d2size);
    std::vector< std::pair<std::size_t,std::size_t> > Best_D2match(d2size);
    fprintf(stderr, "Matching 1 and 2...\n");
    #pragma omp parallel for schedule(dynamic,256)
    for(std::size_t pos=0; pos<d2size; ++pos)
    {
        if(pos%256==0) fprintf(stderr, "\r%lu", (unsigned long)pos);
        unsigned symbol = HASH(&d2[0],pos,d2size);
        // Similarity against source file
        std::pair<std::size_t,std::size_t> BestMatch(0,0);
        for(std::size_t b=d1map[symbol].size(), a=b; a-- > 0; )
        {
            std::size_t offs = d1map[symbol][a];
            std::size_t limit = std::min(d2size-pos, d1size-offs);
            //if((offs>pos ? offs-pos : (pos-offs)) > 4096) limit = std::min(limit, std::size_t(32));
            std::size_t len =
                MatchingLength(&d2[pos], limit, &d1[offs]);
            if(len > BestMatch.first)
                BestMatch = std::pair<std::size_t,std::size_t>(len,offs);
        }
        Best_D1match[pos] = BestMatch;
        // Similarity against itself
        BestMatch.first = 0;
        for(std::size_t b=d2map[symbol].size(), a=b; a-- > 0; )
        {
            std::size_t offs = d2map[symbol][a];
            if(offs >= pos) continue;
            std::size_t limit = std::min(d2size-pos, d2size-offs);
            //if(pos-offs > 4096) limit = std::min(limit, std::size_t(32));
            std::size_t len =
                MatchingLength(&d2[pos], limit, &d2[offs]);
            if(len > BestMatch.first)
                BestMatch = std::pair<std::size_t,std::size_t>(len,offs);
        }
        Best_D2match[pos] = BestMatch;
    }
    fprintf(stderr, "Done...\n");
#endif

    for(std::size_t pos=0; pos<d2size; )
    {
        mode = targetRead;
        std::size_t maxLength = 1, maxOffset = 0;
        double best_cost = 1.0;

        const unsigned symbol = HASH(&d2[0],pos,d2size);
        std::size_t remains = d2size-pos;

        // sourceRead
        if(pos < d1size)
        do{ std::size_t len = MatchingLength(&d1[pos], std::min(d2size-pos, d1size-pos), &d2[pos]);
          if(len < 2) continue;
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
              //if(len < 2) continue; -- Because of hash, always matches at least some
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
              //if(len < 2) continue; -- Because of hash, always matches at least some
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
            const unsigned symbol = HASH(&d2[0],p,d2size);

            // This "if" condition avoids a pathological case where
            // long sequences of 0x00 will slow down the program by
            // a great deal. In such situations, only log the first
            // position.
            if(p==oldpos
            || p+1537 >= d2size
            || std::memcmp(&d2[p], &d2[p+1], 1536) != 0)
            {
                d2map[symbol].push_back(p);
            }

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
    munmap(d1, std::ftell(f1)); std::fclose(f1);
    munmap(d2, std::ftell(f2)); std::fclose(f2);
#else
    delete[] d1; std::fclose(f1);
    delete[] d2; std::fclose(f2);
#endif

    std::fflush(stdout);

    return 0;
}
