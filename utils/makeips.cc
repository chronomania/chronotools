#include <cstdio>
#include <unistd.h>
#include <cstring>

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif


using namespace std;

int main(int argc, const char *const *argv)
{
    if(argc != 3)
    {
        printf("unmakeips: A simple IPS patch maker with next to no error checks\n"
               "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: makeips oldfile newfile >patch.ips\n"
               "(Do not forget > !)\n");
        return -1;
    }
    FILE *f1 = fopen(argv[1], "rb");
    FILE *f2 = fopen(argv[2], "rb");
    if(!f1) { perror(argv[1]); }
    if(!f2) { perror(argv[2]); }
    if(!f1 || !f2) { if(f1)fclose(f1); if(f2)fclose(f2); return -1; }
    
#if USE_MMAP
    fseek(f1, 0, SEEK_END); char *d1 = (char *)
      mmap(NULL, ftell(f1), PROT_READ, MAP_PRIVATE, fileno(f1), 0);

    fseek(f2, 0, SEEK_END); char *d2 = (char *)
      mmap(NULL, ftell(f2), PROT_READ, MAP_PRIVATE, fileno(f2), 0);
#else
    fseek(f1, 0, SEEK_END); unsigned d1size = ftell(f1);
    char *d1 = new char[d1size]; rewind(f1);
    fread(d1, d1size, 1, f1);

    fseek(f2, 0, SEEK_END); unsigned d2size = ftell(f2);
    char *d2 = new char[d2size]; rewind(f2);
    fread(d2, d2size, 1, f1);
#endif
    
    printf("PATCH");
    
    const unsigned thres = 5;
    
    unsigned size = ftell(f1);
    for(unsigned a=0; a<size; )
    {
        unsigned c;
        
        /*if(a==0)
        {
            c=a=512;
            goto redo;
        }*/
        
        if(d1[a] == d2[a]) { ++a; continue; }
        
        c=0;

    redo:
        while(a < size && d1[a] != d2[a] && c < 20000)
            ++c, ++a;
        if(a+thres < size && c < 20000 && memcmp(d1+a, d2+a, thres) != 0)
        {
            a += thres;
            c += thres;
            goto redo;
        }
        
        if(c)
        {
            a -= c;
            
            if(a == 0x454F46) /* "EOF" */
            {
                --a; ++c;
            }
            
            putchar( (a >> 16) & 255 );
            putchar( (a >> 8) & 255 );
            putchar( (a) & 255 );
            
            putchar( (c >> 8) & 255 );
            putchar( (c) & 255 );
            
            fwrite(d2+a, c, 1, stdout);
            
            fprintf(stderr, "%u bytes hunk at %u\n", c, a);

            a += c;
        }
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