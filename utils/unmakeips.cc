#include <cstdio>
#include <cstring>
#include <vector>
#include <map>

using namespace std;

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02

static int Improvize(char* buf, unsigned n)
{
    static bool warned = false;
    if(!warned)
    {
        fprintf(stderr, "Warning: Output file will be larger than original.\n");
        warned = true;
    }
    for(unsigned a=0; a<n; ++a)
        buf[a] = 0;
    return n;
}

int main(int argc, const char *const *argv)
{
    if(argc != 1+3)
    {
        fprintf(stderr, "unmakeips: A simple IPS patcher with lots of error checks\n"
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
    
    FILE *resultfile = fopen(resfn, "wb");
    if(!resultfile) { perror(resfn); }
    
    if(!fp || !original || !resultfile)
        return -1;
    
    setbuf(fp, NULL);
    setbuf(original, NULL);
    setbuf(resultfile, NULL);
    
    unsigned char Buf[5];
    fread(Buf, 1, 5, fp);
    
    if(strncmp((const char *)Buf, "PATCH", 5))
    {
        fprintf(stderr, "This isn't a patch!\n"); arf:fclose(fp);
        arf2:
        fclose(original);
        fclose(resultfile);
        return -1;
    }
    
    multimap<unsigned, vector<char> > lumps;
    
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
            lumps.insert(pair<unsigned, vector<char> > (pos, Buf2));
        }
    }
    if(col) fprintf(stderr, "\n");
    fclose(fp);
    
    unsigned bytes, curpos=0;
    int c, c2;
    char tmp[4096];
    multimap<unsigned, vector<char> >::const_iterator i;
    for(i=lumps.begin(); i!=lumps.end(); ++i)
    {
        unsigned newpos = i->first;
        if(newpos < curpos)
          { fprintf(stderr, "Malformed patch (overlapping hunks)!\n"); goto arf2; }
        unsigned plainlen = newpos-curpos;
        while(plainlen > 0)
        {
            bytes = plainlen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
            c = fread(tmp, 1, bytes, original);
            if(c < 0 && ferror(original)) goto inferr;
            if(!c) c = Improvize(tmp, bytes);
            c2 = fwrite(tmp, 1, c, resultfile);
            if(c2 < 0 && ferror(resultfile)) goto outferr;
            if(c2 != c) goto outfeof;
            plainlen -= c;
        }
        unsigned patchlen = i->second.size();
        unsigned skiplen = patchlen;
        while(skiplen > 0) // original is not necessarily seekable, thus just read it
        {
            bytes = skiplen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
            c = fread(tmp, 1, bytes, original);
            if(c < 0 && ferror(original)) goto inferr;
            if(!c) c = Improvize(tmp, bytes);
            skiplen -= c;
        }
        int c = fwrite(&i->second[0], 1, patchlen, resultfile);
        if(c < 0 && ferror(resultfile)) goto outferr;
        if(c != (int)patchlen)goto outfeof;
        curpos = newpos + patchlen;
    }
    while(!feof(original))
    {
        c = fread(tmp, 1, sizeof tmp, original);
        if(c < 0 && ferror(original)) { inferr: perror("fread"); goto arf2; }
        if(!c) break;
        c2 = fwrite(tmp, 1, c, resultfile);
        if(c2 < 0 && ferror(resultfile)) { outferr: perror("fwrite"); goto arf2; }
        if(c2 != c) { outfeof: fprintf(stderr, "Error writing to resultfile\n"); goto arf2; }
    }
    
    fprintf(stderr, "%s created successfully. %s not modified.\n", resfn, origfn);
    
    fclose(original);
    fclose(resultfile);
    
    return 0;
}
