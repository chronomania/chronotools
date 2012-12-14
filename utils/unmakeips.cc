#include <cstdio>
#include <cstring>
#include <vector>
#include <map>

#include <unistd.h> // for ftruncate and fileno

#include "../crc32.h"

using namespace std;

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02

static size_t Improvize(char* buf, size_t n, size_t at)
{
    static bool warned = false;
    if(!warned)
    {
        fprintf(stderr, "Warning: Output file will be larger than original.\n");
        warned = true;
    }

    // Instead of filling the content with zero, fill with a 01234..pattern.
    // This helps make BPS patch creation a lot faster than without.
    for(size_t a=0; a<n; ++a)
        buf[a] = (at+a) % 0x100;

    return n;
}

static size_t ReadV(FILE* fp)
{
    size_t result = 0, shift = 0;
    for(;; shift += 7)
    {
        int c = fgetc(fp), low = c & 0x7F;
        if(shift) ++low;
        if(c == EOF) break;
        if(c & 0x80) { result += low << shift; break; }
        result += low << shift;
    }
    return result;
}

int main(int argc, const char *const *argv)
{
    if(argc != 1+3)
    {
        fprintf(stderr, "unmakeips: A simple IPS/UPS patcher with lots of error checks\n"
               "Copyright (C) 1992,2008 Bisqwit (http://iki.fi/bisqwit/)\n"
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

    unsigned char Buf[5];
    fread(Buf, 1, 5, fp);

    if(!strncmp((const char *)Buf, "UPS1", 4))
    {
        fseek(fp, 0, SEEK_END);
        size_t patchsize = ftell(fp);
        fseek(fp, 4, SEEK_SET);
        size_t oldsize = ReadV(fp);
        size_t newsize = ReadV(fp);
        size_t oldpos = 0;
        if(oldsize != newsize)
        {
            // Figure out whether we're reverse patching
            fseek(original, 0, SEEK_END);
            size_t origsize = ftell(original);
            rewind(original);

            if(origsize == newsize)
                { fprintf(stderr, "Reverse patch detected\n");
                  std::swap(oldsize, newsize); }
            else if(origsize != oldsize)
                { fprintf(stderr, "What are you patching? Input is %lu bytes, should be %lu or %lu\n",
                    origsize, oldsize, newsize); }
        }
        while( (size_t)ftell(fp) < patchsize - 12)
        {
            /*size_t inpos = ftell(fp);*/
            size_t patchpos = ReadV(fp) + oldpos;
            if(oldpos > 0) ++patchpos;

            size_t plainlen = patchpos-oldpos;
            /*fprintf(stderr, "patchpos: %lX @ %lX,%lX (plain %lX); old(%lX)new(%lX)\n",
                patchpos, inpos,oldpos, plainlen,
                oldsize, newsize);*/
            while(plainlen > 0)
            {
                char tmp[4096];
                size_t bytes = plainlen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
                size_t c = fread(tmp, 1, bytes, original);
                if(c <= 0 && ferror(original)) goto inferr;
                if(!c) c = Improvize(tmp, bytes, ftell(resultfile));
                size_t c2 = fwrite(tmp, 1, c, resultfile);
                if(c2 <= 0 && ferror(resultfile)) goto outferr;
                if(c2 != c) goto outfeof;
                plainlen -= c;
            }
            oldpos = ftell(resultfile);
            while(oldpos < newsize)
            {
                int c = fgetc(fp);
                if(c == 0) break;
                { int e = fgetc(original); if(e != EOF) c ^= e; }
                fputc(c, resultfile); ++oldpos;
            }
            if(oldpos == newsize)
            {
                fseek(fp, -12, SEEK_END);
                break;
            }
        }
        fflush(resultfile);
        ftruncate(fileno(resultfile), newsize);
        goto Created;
    }

    setbuf(fp, NULL);
    setbuf(original, NULL);
    setbuf(resultfile, NULL);


    if(strncmp((const char *)Buf, "PATCH", 5))
    {
        fprintf(stderr, "This isn't a patch!\n"); arf:fclose(fp);
        goto arf2;
    inferr: perror("fread"); goto arf2;
    outferr: perror("fwrite"); goto arf2;
    outfeof: fprintf(stderr, "Error writing to resultfile\n"); goto arf2;
       arf2:
        fclose(original);
        fclose(resultfile);
        return -1;
    }

   {multimap<size_t, vector<char> > lumps;
   {size_t col=0;
    for(;;)
    {
        bool rle=false;
        size_t wanted, c = fread(Buf, 1, 3, fp);
        if(c <= 0 && ferror(fp)) { ipserr: perror("fread"); goto arf; }
        if(c < (wanted=3)) { ipseof:
                    fprintf(stderr, "Unexpected end of file (%s) - wanted %d, got %d\n", patchfn, (int)wanted, (int)c);
                    goto arf; }
        if(!strncmp((const char *)Buf, "EOF", 3))break;
        size_t pos = (((size_t)Buf[0]) << 16)
                     |(((size_t)Buf[1]) << 8)
                     | ((size_t)Buf[2]);
        c = fread(Buf, 1, 2, fp);
        if(c <= 0 && ferror(fp)) { fprintf(stderr, "Got pos %lX\n", pos); goto ipserr; }
        if(c < (wanted=2)) { goto ipseof; }
        size_t len = (((size_t)Buf[0]) << 8)
                    | ((size_t)Buf[1]);

        if(len==0)
        {
            rle=true;
            c = fread(Buf, 1, 2, fp);
            if(c <= 0 && ferror(fp)) { fprintf(stderr, "Got pos %lX\n", pos); goto ipserr; }
            if(c < (wanted=2)) { goto ipseof; }
            len = (((size_t)Buf[0]) << 8)
                 | ((size_t)Buf[1]);

            fprintf(stderr, "%06lX <= %-5lu ", pos, len);
        }
        else
            fprintf(stderr, "%06lX <- %-5lu ", pos, len);

        if(++col == 5) { fprintf(stderr, "\n"); col=0; }

        vector<char> Buf2(len);
        if(rle)
        {
            c = fread(&Buf2[0], 1, 1, fp);
            if(c <= 0 && ferror(fp)) { goto ipserr; }
            if(c != (wanted=1)) { goto ipseof; }
            for(size_t c=1; c<len; ++c)
                Buf2[c] = Buf2[0];
        }
        else
        {
            c = fread(&Buf2[0], 1, len, fp);
            if(c <= 0 && ferror(fp)) { goto ipserr; }
            if(c != (wanted = len)) { goto ipseof; }
        }

        if(pos == IPS_ADDRESS_EXTERN || pos == IPS_ADDRESS_GLOBAL)
        {
            /* Ignore these */
        }
        else
        {
            lumps.insert(pair<size_t, vector<char> > (pos, Buf2));
        }
    }
    if(col) fprintf(stderr, "\n");
   }fclose(fp);

    size_t curpos=0;
    char tmp[4096];
    multimap<size_t, vector<char> >::const_iterator i;
    for(i=lumps.begin(); i!=lumps.end(); ++i)
    {
        size_t newpos = i->first;
        if(newpos < curpos)
          { fprintf(stderr, "Malformed patch (overlapping chunks)!\n"); goto arf2; }
        size_t plainlen = newpos-curpos;
        while(plainlen > 0)
        {
            size_t bytes = plainlen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
            size_t c = fread(tmp, 1, bytes, original);
            if(c <= 0 && ferror(original)) goto inferr;
            if(!c) c = Improvize(tmp, bytes, ftell(resultfile));
            size_t c2 = fwrite(tmp, 1, c, resultfile);
            if(c2 <= 0 && ferror(resultfile)) goto outferr;
            if(c2 != c) goto outfeof;
            plainlen -= c;
        }
        size_t patchlen = i->second.size();
        size_t skiplen = patchlen;
        while(skiplen > 0) // original is not necessarily seekable, thus just read it
        {
            size_t bytes = skiplen; if(bytes > sizeof(tmp)) bytes = sizeof(tmp);
            size_t c = fread(tmp, 1, bytes, original);
            if(c <= 0 && ferror(original)) goto inferr;
            if(!c) c = Improvize(tmp, bytes, ftell(resultfile));
            skiplen -= c;
        }
        size_t c = fwrite(&i->second[0], 1, patchlen, resultfile);
        if(c <= 0 && ferror(resultfile)) goto outferr;
        if(c != patchlen)goto outfeof;
        curpos = newpos + patchlen;
    }
    while(!feof(original))
    {
        size_t c = fread(tmp, 1, sizeof tmp, original);
        if(c <= 0 && ferror(original)) goto inferr;
        if(!c) break;
        size_t c2 = fwrite(tmp, 1, c, resultfile);
        if(c2 <= 0 && ferror(resultfile)) goto outferr;
        if(c2 != c) goto outfeof;
   }}

Created:
    fprintf(stderr, "%s created successfully. %s not modified.\n", resfn, origfn);

    fclose(original);
    fclose(resultfile);

    return 0;
}
