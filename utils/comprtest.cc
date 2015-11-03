#include <vector>
#include <cstdio>

#include "compress.hh"

using namespace std;

typedef unsigned short Word;
typedef unsigned char Byte;
typedef unsigned int Ptr;

unsigned char *ROM;

static vector<Byte> Data(65536);
static unsigned lastsize;

void MessageWorking() {}

static void LoadROM(FILE *fp)
{
    fprintf(stderr, "Loading ROM...");
    unsigned hdrskip = 0;

    char HdrBuf[512];
    fread(HdrBuf, 1, 512, fp);
    if(HdrBuf[1] == 2)
    {
        hdrskip = 512;
    }
    fseek(fp, 0, SEEK_END);
    unsigned romsize = ftell(fp)-hdrskip;
    fprintf(stderr, " $%X bytes...", romsize);
    ROM = NULL;
#ifdef USE_MMAP
    /* This takes about 0.0001s on my computer over nfs */
    ROM = (unsigned char *)mmap(NULL, romsize, PROT_READ, MAP_PRIVATE, fileno(fp), hdrskip);
#endif
    /* mmap could have failed, so revert to reading */
    if(ROM == NULL || ROM == (unsigned char*)(-1))
    {
        /* This takes about 5s on my computer over nfs */
        ROM = new unsigned char [romsize];
        fseek(fp, hdrskip, SEEK_SET);
        fread(ROM, 1, romsize, fp);
    }
    else
        fprintf(stderr, " memmap succesful (%p),", (const void *)ROM);
    fprintf(stderr, " done");
    
    fprintf(stderr, "\n");
}


static unsigned Decompress(unsigned addr)
{
    unsigned origsize = Uncompress(ROM+(addr&0x3FFFFF), Data, ROM+0x400000);
    unsigned resultsize = Data.size();
    fprintf(stderr, "Original size: %u\n", origsize);
    lastsize = origsize;
    return resultsize;
}

static void Try(const vector<unsigned char>& Orig, unsigned n,
                unsigned bits)
{
    fprintf(stderr, "Bits %u:\n", bits);
    vector<unsigned char> result = Compress(&Orig[0], n, bits);
    unsigned comsize = result.size();

    fprintf(stderr, "Generated %u bytes of compressed data.\n", comsize);
    fprintf(stderr, "Attempting decompressing:\n");
    
    result.resize(65536, 0);
    
    unsigned origsize = Uncompress(&result[0], Data);
    unsigned resultsize = Data.size();
    
    unsigned errors = 0;
    if(origsize != comsize)
    {
        fprintf(stderr, "- *ERROR*: Compressor ate %u bytes, should be %u\n",
            origsize, comsize);
        ++errors;
    }
    if(resultsize != n)
    {
        fprintf(stderr, "- *ERROR*: Compressor made %u bytes, should be %u\n",
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
        fprintf(stderr, "- *ERROR*: %u bytes of difference\n", diffs);
        ++errors;
    }
    if(!errors)
        fprintf(stderr, "- OK!\n");
}

static void RecompressTests(unsigned n)
{
    vector<unsigned char> Orig = Data;
    
    Try(Orig, n, 11);

    Try(Orig, n, 12);
}

static const vector<unsigned char> Compress(unsigned n)
{
    return Compress(&Data[0], n);
}

static void PerformanceTest(unsigned n)
{
    RecompressTests(n);
    vector<unsigned char> result = Compress(n);
    fprintf(stderr, "Recompressed as %zu bytes (from %u bytes)", result.size(), n);

    vector<unsigned char> data;
    size_t origsize = Uncompress(&result[0], data);
    char valid[128] = "???";
    if(data.size() == Data.size())
    {
        std::sprintf(valid, "OK");
        for(size_t a=0; a<Data.size(); ++a)
            if(data[a] != Data[a])
            {
                std::sprintf(valid, "Recompress fail at %zu", a);
                break;
            }
    }

    if(origsize != result.size() || data.size() != n)
        fprintf(stderr, " - fail\n");
    else if(lastsize > result.size())
    {
        fprintf(stderr, " - %s - improved %.2f%%\n",
            valid,
            (int)(lastsize - result.size()) * 100.0 / lastsize
               );
    }
    else if(lastsize < result.size())
    {
        fprintf(stderr, " - %s - declined %.2f%%\n",
            valid,
            (int)(result.size() - lastsize) * 100.0 / lastsize
               );
    }
    else
    {
        fprintf(stderr, " - %s\n", valid);
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
    
/*
    vector<unsigned char> data2(65536);
    
    for(unsigned n=0; n<0x40; ++n)
    {
        memcpy(&data2[n*32], &Data[ROM[0x027321+n]*32], 32);
    }
    
    for(unsigned n=0; n<0x40; ++n)
    {
        fwrite(&data2[ROM[0x03E7D0+n]*32],1,32, fp);
    }
*/
    fwrite(&Data[0], 1, n, fp);
    fclose(fp);
}

/*
int main(void)
{
    FILE *fp = fopen("FIN/chrono-nohdr.smc", "rb");
    LoadROM(fp);
    fclose(fp);
    
    unsigned addr, n;

#if 1
    addr = 0xDB0000; // Settings
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xFE6002; // Title screen stuff
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC40EB8; // more map gfx
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC59A56; // players on map
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC40000; // map gfx, perhaps
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC6FB00; // tile indices in time gauge ?
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC62000; // no idea about this one
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC5D80B; // "?" time label
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC5DA88; // Time label boxes and "PONTPO"
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xFD0000; // It's a font
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
    
    addr = 0xC38000; // time gauge gfx titles
    n = Decompress(addr); DumpGFX(addr);
    PerformanceTest(n);
#endif
    
#if 0
    //     Time gauge:
    //      C38000 (pointed from C27220 (offset) and C2738C (page)) = gfx
    //        - read to 9000
    //      C6FB00 (pointed from C27224 (offset) and C273DB (page)) = ?
    //        - read to 7F:8C00
    //        - copied to 7E:C000
    //        - copied to 00:0920 = PALETTE
    //      C62000 (pointed from C27228 (offset) and C27407 (page)) = ?
    //        - read to 7F:9000
    //        - looks like tile table.
    //        - of the bg image.
    //      At C2:7361
    //       - loads bytes from $C6F700+x
    //         stores them to $7F9000
    //           hi nibble first, then lo nibble
    //          totalling $400 bytes to $800 bytes.
    //       - could be the bg image..
    DumpGFX(0xC38000);
#endif

    // Character names and other miscellaneous data:
    //DumpGFX(0xDB0000);

#if 0
    fprintf(stderr, "Uncompressed %u bytes.\n", n);
    
    for(unsigned a=0; a<n; ++a) putchar(Data[a]);
#endif

    return 0;
}
*/

int main(int argc, const char *const *argv)
{
    if(argc != 2 && argc != 3)
    {
        fprintf(stderr, "Usage: comprtest <input-uncompressedfile> <result-compressedfile>\n");
        return 0;
    }
    FILE *fp = fopen(argv[1], "rb");
    if(!fp) { perror(argv[1]); return -1; }
    fseek(fp,0,SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    ROM = new unsigned char[size];
    fread(ROM,1,size, fp);
    fclose(fp);
    vector<unsigned char> data(ROM, ROM+size);

    if(argc == 2)
    {
        std::vector<unsigned char> buf;
        Uncompress(&data[0], buf, &data[0]+data.size());

        fprintf(stderr, "Decompressed to %u bytes\n", (unsigned) buf.size());
        fwrite(&buf[0], 1, buf.size(), stdout);
    }
    else
    {
        std::vector<unsigned char> data1 = Compress(&data[0], data.size(), 11);
        std::vector<unsigned char> data2 = Compress(&data[0], data.size(), 12);

        fprintf(stderr, "%u bytes with setting 11, %u bytes with setting 12\n",
            (unsigned) data1.size(),
            (unsigned) data2.size() );

        if(data1.size() < data2.size()) data.swap(data1); else data.swap(data2);

        fp = fopen(argv[2], "wb");
        if(!fp) { perror(argv[2]); return -1; }
        fwrite(&data[0], 1, data.size(), fp);
        fclose(fp);
    }
    return 0;
}
