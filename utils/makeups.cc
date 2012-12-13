#include <cstdio>
#include <unistd.h>
#include <cstring>

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

#include "../crc32.h"

static crc32_t out_crc = crc32_startvalue;

static void Output(const unsigned char* buf, size_t n)
{
    fwrite(buf, 1, n, stdout);
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
static void OutV(size_t n)
{
    const unsigned maxsize = (64 + 6) / 7;
    unsigned char Buf[maxsize], *o = Buf;
    while(n > 0x7F) { *o++ = (n & 0x7F); n >>= 7; n -= 1; }
    *o++ = n | 0x80;
    Output(Buf, o-Buf);
}

int main(int argc, const char *const *argv)
{
    if(argc != 3)
    {
        std::printf("makeups: A simple UPS patch maker with next to no error checks\n"
               "Copyright (C) 1992,2008 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: makeups oldfile newfile > patch.ups\n"
               "Don't forget the \">\"!\n");
        return -1;
    }
    FILE *f1 = std::fopen(argv[1], "rb");
    FILE *f2 = std::fopen(argv[2], "rb");
    if(!f1) { std::perror(argv[1]); }
    if(!f2) { std::perror(argv[2]); }
    if(!f1 || !f2) { if(f1) std::fclose(f1);
                     if(f2) std::fclose(f2); return -1; }

    std::fseek(f1, 0, SEEK_END); std::size_t d1size = std::ftell(f1);
    std::fseek(f2, 0, SEEK_END); std::size_t d2size = std::ftell(f2);

#if USE_MMAP
    char *d1 = (char *)mmap(NULL, d1size, PROT_READ, MAP_PRIVATE, fileno(f1), 0);
    char *d2 = (char *)mmap(NULL, d2size, PROT_READ, MAP_PRIVATE, fileno(f2), 0);
#else
    char *d1 = new char[d1size];
    std::rewind(f1); std::fread(d1, d1size, 1, f1);

    char *d2 = new char[d2size];
    std::rewind(f2); std::fread(d2, d2size, 1, f1);
#endif

    Output((const unsigned char*)"UPS1", 4);
    OutV(d1size);
    OutV(d2size);

    uint_fast32_t crc1 = crc32_calc( (const unsigned char*) d1, d1size);
    uint_fast32_t crc2 = crc32_calc( (const unsigned char*) d2, d2size);

    bool nach_specs = false;

    size_t lastpos = 0;
    size_t pos     = 0;
    bool out_mode = false;
    for(; pos < d1size || pos < d2size; ++pos)
    {
        unsigned char a = 0; if(pos < d1size) a = d1[pos];
        unsigned char b = 0; if(pos < d2size) b = d2[pos];

        unsigned char xorred = a ^ b;

        bool differs = xorred != 0 || (nach_specs && (pos >= d1size || pos >= d2size));
        if(!out_mode && differs)
        {
            if(!lastpos) OutV(pos); else OutV(pos - lastpos - 1);
            out_mode = true;
        }
        if(out_mode)
        {
            Output(&xorred, 1);
            if(!differs) out_mode = false;
            lastpos = pos;
        }
    }
    if(nach_specs && d1size == d2size) { unsigned char c=0; Output(&c, 1); }

#ifdef USE_MMAP
    munmap(d1, std::ftell(f1)); std::fclose(f1);
    munmap(d2, std::ftell(f2)); std::fclose(f2);
#else
    delete[] d1; std::fclose(f1);
    delete[] d2; std::fclose(f2);
#endif

    OutL32(crc1 ^ crc32_startvalue);
    OutL32(crc2 ^ crc32_startvalue);
    OutL32(out_crc ^ crc32_startvalue);

    std::fflush(stdout);

    return 0;
}
