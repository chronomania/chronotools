#include <vector>

#include "rommap.hh"
#include "compress.hh"

using namespace std;

typedef unsigned short Word;
typedef unsigned char Byte;
typedef unsigned int Ptr;

FILE *scriptout = stdout;

static vector<Byte> Data(65536);
static unsigned lastsize;

static unsigned Decompress(unsigned addr)
{
    unsigned origsize = Uncompress(ROM+(addr&0x3FFFFF), Data);
    unsigned resultsize = Data.size();
    fprintf(stderr, "Original size: %u\n", origsize);
    lastsize = origsize;
    return resultsize;
}

static void Try(const vector<unsigned char>& Orig, unsigned n,
                unsigned seg, unsigned bits)
{
    fprintf(stderr, "Segment %X, bits %u:\n", seg, bits);
    vector<unsigned char> result = Compress(&Orig[0], n, seg, bits);
    unsigned comsize = result.size();

    fprintf(stderr, "Generated %u bytes of compressed data.\n", comsize);
    fprintf(stderr, "Attempting decompressing:\n");
    
    result.resize(65536, 0);
    
    unsigned origsize = Uncompress(&result[0], Data);
    unsigned resultsize = Data.size();
    
    unsigned errors = 0;
    if(origsize != comsize)
    {
        fprintf(stderr, "- Error: Compressor ate %u bytes, should be %u\n",
            origsize, comsize);
        ++errors;
    }
    if(resultsize != n)
    {
        fprintf(stderr, "- Error: Compressor made %u bytes, should be %u\n",
            resultsize, n);
        ++errors;
    }

    unsigned diffs = 0;
    for(unsigned a=0; a<resultsize; ++a)
    {
        if(a >= n) break;
        if(Orig[a] != Data[a]) ++diffs;
    }
    if(diffs)
    {
        fprintf(stderr, "- Error: %u bytes of difference\n", diffs);
        ++errors;
    }
    if(!errors)
        fprintf(stderr, "- OK!\n");
}

static void RecompressTests(unsigned n)
{
    vector<unsigned char> Orig = Data;
    
    Try(Orig, n, 0x7F, 11);

    Try(Orig, n, 0x7F, 12);

    Try(Orig, n, 0x7E, 11);

    Try(Orig, n, 0x7E, 12);
}

static const vector<unsigned char> Compress(unsigned char seg, unsigned n)
{
    vector<unsigned char> result1 = Compress(&Data[0], n, seg, 11);
    vector<unsigned char> result2 = Compress(&Data[0], n, seg, 12);
    
    return result1.size() < result2.size() ? result1 : result2;
}

static void PerformanceTest(unsigned char seg, unsigned n)
{
    vector<unsigned char> result = Compress(seg, n);
    fprintf(stderr, "Recompressed as %u bytes (from %u bytes)", result.size(), n);

    vector<unsigned char> data;
    unsigned origsize = Uncompress(&result[0], data);
    if(origsize != result.size() || data.size() != n)
        fprintf(stderr, " - fail\n");
    else if(lastsize > result.size())
    {
        fprintf(stderr, " - OK - improved %.2f%%\n",
            (int)(lastsize - result.size()) * 100.0 / lastsize
               );
    }
    else if(lastsize < result.size())
    {
        fprintf(stderr, " - OK - declined %.2f%%\n",
            (int)(result.size() - lastsize) * 100.0 / lastsize
               );
    }
    else
    {
        fprintf(stderr, " - OK\n");
    }
    fprintf(stderr, "\n");
}

int main(void)
{
    FILE *fp = fopen("chronofin-nohdr.smc", "rb");
    LoadROM(fp);
    fclose(fp);
    
    unsigned addr, n;

    addr = 0xFE6002; // Title screen stuff
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xC40EB8; // more map gfx
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xC59A56; // players on map
    n = Decompress(addr);
    PerformanceTest(0x7E, n);
    
    addr = 0xC40000; // map gfx, perhaps
    n = Decompress(addr);
    PerformanceTest(0x7E, n);
    
    addr = 0xC6FB00; // tile indices in time gauge ?
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xC62000; // no idea about this one
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xC5D80B; // "?" time label
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xC5DA88; // Time label boxes and "PONTPO"
    n = Decompress(addr);
    PerformanceTest(0x7F, n);
    
    addr = 0xFD0000; // It's a font
    n = Decompress(addr);
    PerformanceTest(0x7E, n);
    
    addr = 0xC38000; // time gauge gfx titles
    n = Decompress(addr);
    PerformanceTest(0x7F, n);

#if 0
    fprintf(stderr, "Uncompressed %u bytes.\n", n);
    
    //for(unsigned a=0; a<n; ++a) putchar(Data[a]);
    
    fprintf(stderr, "Attempting recompression.\n");
    
    RecompressTests(n);
#endif

    return 0;
}
