#include <vector>

#include "rommap.hh"
#include "compress.hh"
#include "scriptfile.hh"

using namespace std;

typedef unsigned short Word;
typedef unsigned char Byte;
typedef unsigned int Ptr;


//dummy, used by rommap.o
void StartBlock(const char*, unsigned){}
void PutAscii(const string&){}
void EndBlock(){}


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

static void DumpGFX(unsigned addr)
{
    unsigned n = Decompress(addr);
    fprintf(stderr, "Uncompressed %u bytes.\n", n);
    char Buf[64];
    sprintf(Buf, "ct-%06X.sd2", addr);
    
    FILE *fp = fopen(Buf, "wb");
    
    vector<unsigned char> data2(65536);
    
    for(unsigned n=0; n<0x40; ++n)
    {
        memcpy(&data2[n*32], &Data[ROM[0x027321+n]*32], 32);
    }
    
    for(unsigned n=0; n<0x40; ++n)
    {
        fwrite(&data2[ROM[0x03E7D0+n]*32],1,32, fp);
    }

    fwrite(&Data[0], 1, n, fp);
    fclose(fp);
}

int main(void)
{
    FILE *fp = fopen("chronofin-nohdr.smc", "rb");
    LoadROM(fp);
    fclose(fp);
    
    unsigned addr, n;

#if 0
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
#endif
    
#if 1
    /*
    
    Time gauge:
     C38000 (pointed from C27220 (offset) and C2738C (page)) = gfx
       - read to 9000
     C6FB00 (pointed from C27224 (offset) and C273DB (page)) = ?
       - read to 7F:8C00
       - copied to 7E:C000
       - copied to 00:0920 = PALETTE
     C62000 (pointed from C27228 (offset) and C27407 (page)) = ?
       - read to 7F:9000
       - looks like tile table.
       - of the bg image.
     At C2:7361
      - loads bytes from $C6F700+x
        stores them to $7F9000
          hi nibble first, then lo nibble
         totalling $400 bytes to $800 bytes.
      - could be the bg image..
    
    */
    DumpGFX(0xC38000);
#endif

#if 0
    fprintf(stderr, "Uncompressed %u bytes.\n", n);
    
    for(unsigned a=0; a<n; ++a) putchar(Data[a]);
#endif

    return 0;
}
