#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
using namespace std;

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02

int main(int argc, const char *const *argv)
{
    if(argc != 1+3)
    {
        fprintf(stderr, "fixchecksum: Fixes SNES patch checksum.\n"
               "Copyright (C) 1992,2004 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: unmakeips ipsfile.ips oldfile newfile\n");
        return -1;
    }
    const char *patchfn = argv[1];
    const char *origfn = argv[2];
    const char *resfn = argv[3];
    
    FILE *fp = fopen(patchfn, "rb");
    if(!fp) { perror(patchfn); }
    
    FILE *original = fopen(origfn, "rb");
    if(!original) { if(!strcmp(origfn, "-")) original = stdin;
                    else perror(origfn); }
    
    if(!fp || !original)
        return -1;
    
    setbuf(fp, NULL);
    setbuf(original, NULL);
    
    unsigned char Buf[5];
    fread(Buf, 1, 5, fp);
    
    if(strncmp((const char *)Buf, "PATCH", 5))
    {
        fprintf(stderr, "This isn't a patch!\n"); arf:fclose(fp);
        arf2:
        if(original)fclose(original);
        return -1;
    }
    
    vector<unsigned char> ROM;
    while(!feof(original))
    {
        char tmp[4096];
        int c = fread(tmp, 1, sizeof tmp, original);
        if(c < 0 && ferror(original)) { inferr: perror("fread"); goto arf2; }
        if(!c) break;
        
        ROM.insert(ROM.end(), tmp, tmp+c);
    }
    fclose(original); original=NULL;
    vector<bool> Touched(ROM.size(), false);
    
    unsigned col=0;
    for(;;)
    {
        int wanted,c = fread(Buf, 1, 3, fp);
        if(c < 0 && ferror(fp)) { ipserr: perror("fread"); goto arf; }
        if(c < (wanted=3)) { ipseof:
                    fprintf(stderr, "Unexpected end of file (%s) - wanted %d, got %d\n", patchfn, wanted, c);
                    goto arf; }
        if(!strncmp((const char *)Buf, "EOF", 3))break;
        unsigned pos = (((unsigned)Buf[0]) << 16)
                      |(((unsigned)Buf[1]) << 8)
                      | ((unsigned)Buf[2]);
        c = fread(Buf, 1, 2, fp);
        if(c < 0 && ferror(fp)) { fprintf(stderr, "Got pos %X\n", pos); goto ipserr; }
        if(c < (wanted=2)) { goto ipseof; }
        unsigned len = (((unsigned)Buf[0]) << 8)
                      | ((unsigned)Buf[1]);
        
        fprintf(stderr, "%06X <- %-5u ", pos, len);
        if(++col == 5) { fprintf(stderr, "\n"); col=0; }
        
        vector<char> Buf2(len);
        c = fread(&Buf2[0], 1, len, fp);
        if(c < 0 && ferror(fp)) { goto ipserr; }
        if(c != (wanted=(int)len)) { goto ipseof; }
        
        if(pos == IPS_ADDRESS_EXTERN || pos == IPS_ADDRESS_GLOBAL)
        {
            /* Ignore these */
        }
        else
        {
            for(unsigned a=0; a<len; ++a)
            {
                if(pos+a >= ROM.size())
                {
                    fprintf(stderr, "Got too big pos %u+%u (ROM size only %u)\n",
                        pos,len, ROM.size());
                    goto arf;
                }
                ROM[pos+a] = Buf2[a];
                Touched[pos+a] = true;
            }
        }
    }
    if(col) fprintf(stderr, "\n");
    fclose(fp);
    
    unsigned CalculatedSize = (ROM.size() / 0x2000) * 0x2000;
    unsigned size = CalculatedSize; 
    for(unsigned power2=0; ; ++power2)
        if(!(size >>= 1)) { size = 1 << power2; break; }
    unsigned sum1=0, sum2=0, remainder=CalculatedSize-size;
    unsigned offset = ROM.size() - CalculatedSize;
    for(unsigned a=0; a<size; ++a) sum1 += ROM[offset+a];
    for(unsigned a=0; a<remainder; ++a) sum2 += ROM[offset+size+a];
    if(remainder) sum1 += sum2 * (size / remainder);
    sum1 &= 0xFFFF;
    sum2 = sum1 ^ 0xFFFF;
    ROM[offset+0xFFDC] = sum2 & 255; Touched[offset+0xFFDC] = true;
    ROM[offset+0xFFDD] = sum2 >> 8;  Touched[offset+0xFFDD] = true;
    ROM[offset+0xFFDE] = sum1 & 255; Touched[offset+0xFFDE] = true;
    ROM[offset+0xFFDF] = sum1 >> 8;  Touched[offset+0xFFDF] = true;

    fprintf(stderr, "ROM size $%X (calculated $%X, 2pow $%X, remainder $%X, offset $%X)\n",
        ROM.size(), CalculatedSize, size, remainder, offset);
    
    FILE *resultfile = fopen(resfn, "wb");
    if(!resultfile) { perror(resfn); return -1; }
    
    fprintf(resultfile, "PATCH");

    const unsigned MaxHunkSize = 20000;

    /* Format:   24bit offset, 16-bit size, then data; repeat */
    for(unsigned a=0; a<ROM.size(); ++a)
    {
        if(!Touched[a])continue;

        putc((a >>16)&255, resultfile);                        
        putc((a >> 8)&255, resultfile);
        putc((a     )&255, resultfile);
        
        unsigned offs=a, c=0;
        while(a < ROM.size() && Touched[a] && c < MaxHunkSize)
            ++c, ++a;
        
        putc((c>> 8)&255, resultfile);
        putc((c    )&255, resultfile);
        int ret = fwrite(&ROM[offs], 1, c, resultfile);
        if(ret < 0 || ret != (int)c)
        {
            fprintf(stderr,
                " fwrite failed: %d != %d - this patch will be broken.\n",
                ret, (int)c);
            perror("fwrite");
        }
    }
    fwrite("EOF",   1, 3, resultfile);
    fclose(resultfile);
    
    return 0;
}
