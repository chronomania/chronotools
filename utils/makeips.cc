#include <cstdio>
#include <unistd.h>
#include <cstring>

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

#define IPS_EOF_MARKER 0x454F46
#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02

using namespace std;

static void Put24(unsigned v)
{
    putchar( (v >> 16) & 255 );
    putchar( (v >> 8) & 255 );
    putchar( (v) & 255 );
}
static void Put16(unsigned v)
{
    putchar( (v >> 8) & 255 );
    putchar( (v) & 255 );
}

struct RLE { unsigned addr, len; };

static RLE FindRLE(const char* source, const unsigned len, unsigned addr)
{
/*
    In IPS format:
    Not-RLE: 3+2+N   (5+N)
    RLE:     3+2+2+1 (8)
    
    RLE will only be used, if:
      The length of preceding chunk > A
      The length of successor chunk > B
      The length of the RLE         > 8
    Where
      A = 0
      B = 0
*/

    RLE result;
    result.len=0;
    for(unsigned a=0; a<len; )
    {
        unsigned rle_len = 1;
        
        if(a+addr != IPS_EOF_MARKER
        && a+addr != IPS_ADDRESS_GLOBAL
        && a+addr != IPS_ADDRESS_EXTERN)
        {
            while(a+rle_len < len && source[a+rle_len] == source[a]
               && rle_len < 0xFFFF) ++rle_len;
        }
        
        //unsigned size_before = a;
        //unsigned size_after  = len-(a+rle_len);
        
        if((addr+a+rle_len == IPS_EOF_MARKER
        || addr+a+rle_len == IPS_ADDRESS_EXTERN
        || addr+a+rle_len == IPS_ADDRESS_GLOBAL) && rle_len > 1) { --rle_len; }
        
        if(rle_len <= 8) { a += rle_len; continue; }
        
        result.len  = rle_len;
        result.addr = a;
        break;
    }
    return result;
}

static void PutChunk(unsigned addr, unsigned nbytes, const char* source, bool recursed=false)
{
    if(!recursed)fprintf(stderr, "$%06X: $%X bytes: ", addr, nbytes);
    const char*sep = "";
    while(nbytes > 0)
    {
        RLE rle = FindRLE(source, nbytes, addr);
        if(rle.len)
        {
            if(rle.addr > 0)
            {
                fprintf(stderr, "%s", sep);
                PutChunk(addr, rle.addr, source, true); sep=", ";
                addr   += rle.addr;
                source += rle.addr;
                nbytes -= rle.addr;
            }

            fprintf(stderr, "%s%u*%02X", sep, rle.len, (unsigned char)*source); sep=", ";
            Put24(addr);
            Put16(0);
            Put16(rle.len);
            putchar(*source);
            source += rle.len;
            addr   += rle.len;
            nbytes -= rle.len;
            continue;
        }
        
        while(addr == IPS_EOF_MARKER
           || addr == IPS_ADDRESS_EXTERN
           || addr == IPS_ADDRESS_GLOBAL)
        {
            /* Avoid reserved tags, go 1 byte backward */
            --addr;
            --source;
            ++nbytes;
        }

        unsigned eat = 20000;
        if(eat > nbytes) eat = nbytes;
        
        Put24(addr);
        Put16(eat);
        fwrite(source, eat, 1, stdout);
        
        fprintf(stderr, "%s%u raw", sep,eat); sep=", ";

        addr   += eat;
        source += eat;
        nbytes -= eat;
    }
    if(!recursed) fprintf(stderr, "\n");
}

int main(int argc, const char *const *argv)
{
    if(argc != 3)
    {
        printf("unmakeips: A simple IPS patch maker with next to no error checks\n"
               "Copyright (C) 1992,2005 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: makeips oldfile newfile > patch.ips\n"
               "Don't forget the \">\"!\n");
        return -1;
    }
    FILE *f1 = fopen(argv[1], "rb");
    FILE *f2 = fopen(argv[2], "rb");
    if(!f1) { perror(argv[1]); }
    if(!f2) { perror(argv[2]); }
    if(!f1 || !f2) { if(f1)fclose(f1); if(f2)fclose(f2); return -1; }
    
    fseek(f1, 0, SEEK_END); unsigned d1size = ftell(f1);
    fseek(f2, 0, SEEK_END); unsigned d2size = ftell(f2);

#if USE_MMAP
    char *d1 = (char *)mmap(NULL, d1size, PROT_READ, MAP_PRIVATE, fileno(f1), 0);
    char *d2 = (char *)mmap(NULL, d2size, PROT_READ, MAP_PRIVATE, fileno(f2), 0);
#else
    char *d1 = new char[d1size];
    rewind(f1); fread(d1, d1size, 1, f1);

    char *d2 = new char[d2size];
    rewind(f2); fread(d2, d2size, 1, f1);
#endif
    
    printf("PATCH");
    
    const unsigned thres = 5;
    
    unsigned size = ftell(f1);
    for(unsigned a=0; a<size; )
    {
        unsigned diff_size = 0;
    FindMoreDiff:
        while(a+diff_size < size && d1[a+diff_size] != d2[a+diff_size])
        {
            ++diff_size;
        }
        if(!diff_size)
        {
            ++a;
            continue;
        }
        
        unsigned diff_end = a+diff_size;
        
        unsigned next_diff = diff_end;
        while(next_diff < size && d1[next_diff] == d2[next_diff]) ++next_diff;
        
        if(next_diff < size && (next_diff - diff_end) < 32)
        {
            diff_size = next_diff-a;
            goto FindMoreDiff;
        }
        
        PutChunk(a, diff_size, d2+a);
        a += diff_size;
    }
    if(size < d2size)
    {
        PutChunk(size, d2size-size, d2+size);
    }
    
#ifdef USE_MMAP
    munmap(d1, ftell(f1)); fclose(f1);
    munmap(d2, ftell(f2)); fclose(f2);
#else
    delete[] d1; fclose(f1);
    delete[] d2; fclose(f2);
#endif
    
    printf("EOF");
    
    fflush(stdout);
    
    return 0;
}
