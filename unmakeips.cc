#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
using namespace std;

int main(int argc, const char *const *argv)
{
    if(argc != 2)
    {
        printf("unmakeips: A simple IPS patcher with lots of error checks\n"
               "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n"
               "Usage: unmakeips ipsfile.ips <oldfile >newfile\n"
               "(Do not forget < and > !)\n");
        return -1;
    }
    const char *patchfn = argv[1];
    
    FILE *fp = fopen(patchfn, "rb");
    
    setbuf(fp, NULL);
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    
    if(!fp) { perror(patchfn); return -1; }
    unsigned char Buf[5];
    fread(Buf, 1, 5, fp);
    
    if(strncmp((const char *)Buf, "PATCH", 5))
    {
        fprintf(stderr, "This isn't a patch!\n"); arf:fclose(fp);
        arf2: return -1;
    }
    
    multimap<unsigned, vector<char> > lumps;
    
    unsigned col=0;
    for(;;)
    {
        int c = fread(Buf, 1, 3, fp);
        if(c < 0 && ferror(fp)) { ipserr: perror("fread"); goto arf; }
        if(c < 3) { ipseof: fprintf(stderr, "Unexpected end of file (%s)\n", patchfn);
                    goto arf; }
        if(!strncmp((const char *)Buf, "EOF", 3))break;
        unsigned pos = (((unsigned)Buf[0]) << 16)
                      |(((unsigned)Buf[1]) << 8)
                      | ((unsigned)Buf[2]);
        c = fread(Buf, 1, 2, fp);
        if(c < 0 && ferror(fp)) { fprintf(stderr, "Got pos %X\n", pos); goto ipserr; }
        if(c < 2) { goto ipseof; }
        unsigned len = (((unsigned)Buf[0]) << 8)
                      | ((unsigned)Buf[1]);
        
        fprintf(stderr, "%06X <- %-5u ", pos, len);
        if(++col == 5) { fprintf(stderr, "\n"); col=0; }
        
        vector<char> Buf2(len);
        c = fread(&Buf2[0], 1, len, fp);
        if(c < 0 && ferror(fp)) { goto ipserr; }
        if(c != (int)len) { goto ipseof; }
        lumps.insert(pair<unsigned, vector<char> > (pos, Buf2));
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
            c = fread(tmp, 1, bytes, stdin);
            if(c < 0 && ferror(stdin)) goto inferr;
            if(!c) { infeof: fprintf(stderr, "Unexpected end of stdin\n"); goto arf2; }
            c2 = fwrite(tmp, 1, c, stdout);
            if(c2 < 0 && ferror(stdout)) goto outferr;
            if(c2 != c) goto outfeof;
            plainlen -= c;
        }
        unsigned patchlen = i->second.size();
        unsigned skiplen = patchlen;
        while(skiplen > 0) // stdin is not necessarily seekable, thus just read it
        {
            bytes = skiplen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
            c = fread(tmp, 1, bytes, stdin);
            if(c < 0 && ferror(stdin)) goto inferr;
            if(!c) goto infeof;
            skiplen -= c;
        }
        int c = fwrite(&i->second[0], 1, patchlen, stdout);
        if(c < 0 && ferror(stdout)) goto outferr;
        if(c != (int)patchlen)goto outfeof;
        curpos = newpos + patchlen;
    }
    while(!feof(stdin))
    {
        c = fread(tmp, 1, sizeof tmp, stdin);
        if(c < 0 && ferror(stdin)) { inferr: perror("fread"); goto arf2; }
        if(!c) break;
        c2 = fwrite(tmp, 1, c, stdout);
        if(c2 < 0 && ferror(stdout)) { outferr: perror("fwrite"); goto arf2; }
        if(c2 != c) { outfeof: fprintf(stderr, "Error writing to stdout\n"); goto arf2; }
    }
    
    return 0;
}
