#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <set>

#ifdef linux
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

#define LOADP_DEBUG      1
//#define LOADZ_EXTRASPACE 1
//#define LOADP_EXTRASPACE 1

using namespace std;

#include "ctcset.hh"
#include "miscfun.hh"

static unsigned char *ROM;
static vector<bool> space;
static vector<wstring> substrings;

static const bool TryFindExtraSpace = false;
static const unsigned ExtraSpaceMinLen = 128;

static void LoadROM()
{
    fprintf(stderr, "Loading ROM...");
    unsigned hdrskip = 0;
    const char *fn = "chrono-dumpee.smc";
    FILE *fp = fopen(fn, "rb");
    if(!fp)
    {
        perror(fn);
        return;
    }
    char HdrBuf[512];
    fread(HdrBuf, 1, 512, fp);
    if(HdrBuf[1] == 2)
    {
        hdrskip = 512;
    }
    fseek(fp, 0, SEEK_END);
    unsigned romsize = ftell(fp)-hdrskip;
    ROM = NULL;
#ifdef USE_MMAP
    /* This takes about 0.0001s on my computer over nfs */
    ROM = (unsigned char *)mmap(NULL, romsize, PROT_READ, MAP_PRIVATE, fileno(fp), hdrskip);
#endif
    /* mmap could have failed, so revert to reading */
    if(ROM == (unsigned char*)(-1))
    {
        /* This takes about 5s on my computer over nfs */
        ROM = new unsigned char [romsize];
        fseek(fp, hdrskip, SEEK_SET);
        fread(ROM, 1, romsize, fp);
    }
    fclose(fp);
    fprintf(stderr, " done");
    space.clear();
    space.resize(romsize);
    fprintf(stderr, "\n");
}

static void markspace(unsigned offs, unsigned size)
{
    for(; size > 0; --size)
        space[offs++] = true;
}

// Load an array of pascal style strings
static const vector<string> LoadPStrings(unsigned offset, unsigned count)
{
    unsigned segment = offset & 0xFF0000;
    vector<string> strings(count);
#if LOADP_DEBUG
    const unsigned maxco=10;
    unsigned col=maxco;
#endif
    for(unsigned a=0; a<count; ++a)
    {
        unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
#if LOADP_DEBUG
        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(maxco==10 && col==5)putchar(' ');
        printf(" $%04X", stringptr);
        if(++col == maxco) { printf("\n"); col=0; }
#endif
        
        stringptr += segment;
        strings[a] = string((const char *)&ROM[stringptr] + 1, ROM[stringptr]);
        offset += 2;
        
        markspace(stringptr, strings[a].size()+1);

#ifdef LOADP_EXTRASPACE
        for(unsigned freebyte = stringptr + strings[a].size()+1;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; space[freebyte++]=true);
#endif
    }
#if LOADP_DEBUG
    if(col)printf("\n");
#endif
    return strings;
}

// Load an array of C style strings
static const vector<string> LoadZStrings(unsigned offset, unsigned count=0)
{
    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    vector<string> strings(count);
#if 0
    const unsigned maxco=10;
    unsigned col=maxco;
#endif
    set<unsigned> offsetlist;
    
    unsigned firstoffs = 0x10000, lastoffs = 0, lastlen = 0;
    for(unsigned a=0; !count || a<count; ++a)
    {
        const unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        if(offsetlist.find(offset & 0xFFFF) != offsetlist.end())
            break;
        offsetlist.insert(stringptr);

#if 0
        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(maxco==10 && col==5)putchar(' ');
        printf(" $%04X", stringptr);
        if(++col == maxco) { printf("\n"); col=0; }
#endif
        
        const string foundstring((const char *)&ROM[stringptr + base]);
        
        markspace(stringptr+base, foundstring.size()+1);
        
        if(stringptr < firstoffs) firstoffs = stringptr;
        if(stringptr > lastoffs)
        {
            lastoffs=  stringptr;
            lastlen = foundstring.size();
        }
#ifdef LOADZ_EXTRASPACE
        for(unsigned freebyte = stringptr + base + foundstring.size();
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; space[freebyte++]=true);
#endif
        if(count)
            strings[a] = foundstring;
        else
            strings.push_back(foundstring);
        
        offset += 2;
    }
#if 0
    if(col)printf("\n");
#endif
    return strings;
}

// Load an array of fixed length strings
static const vector<string> LoadFStrings(unsigned offset, unsigned len, unsigned maxcount=0)
{
    string str((const char *)&ROM[offset]);
    unsigned count = str.size() / len + 1;
    if(maxcount && count > maxcount)count = maxcount;
    vector<string> result(count);
    for(unsigned a=0; a<count; ++a)
        result[a] = str.substr(a*len, len);
    return result;
}

static void FindEndSpaces(void)
{
    fprintf(stderr, "Finding free space...");
    unsigned pagecount = (space.size()+0xFFFF) >> 16;
    for(unsigned page=0; page<pagecount; ++page)
    {
        unsigned pageend = (page+1) << 16;
        if(pageend >= space.size()) pageend = space.size();
        unsigned pagebegin = (page << 16);
        unsigned pagesize = pageend - pagebegin;
        
        //const char *beginptr = (const char *)&ROM[pagebegin];
        //const char *endptr   = (const char *)&ROM[pageend];
        unsigned size = pagesize;
        
        static const char blanks[] = {0x00, (char)0xEF, (char)0xFF};
        
        for(unsigned blank=0; blank < sizeof(blanks); ++blank)
        {
            unsigned blanklen=0;
            for(unsigned blapos=pagebegin; blapos<pageend; ++blapos)
            {
                if(ROM[blapos] == blanks[blank])
                {
                    ++blanklen;
                    if(blanklen == ExtraSpaceMinLen)
                    {
                        for(unsigned a=0; a<ExtraSpaceMinLen; ++a)
                            space[blapos-(ExtraSpaceMinLen-1)+a] = true;
                    }
                    if(blanklen >= ExtraSpaceMinLen)
                        space[blapos] = true;
                }
                else
                    blanklen=0;
            }
            /*
            vector<char> BlankBuf(64, blanks[blank]);
            
            const char *ptr = mempos(beginptr, size, &BlankBuf[0], BlankBuf.size());
            if(!ptr)continue;
            
            for(unsigned blapos = (ptr-beginptr);
                ROM[pagebegin+blapos] == blanks[blank]
             && blapos < pageend; ++blapos)
            {
                space[pagebegin+blapos] = true;
            }
            */
        }
    }
    fprintf(stderr, " done\n");
}

static wstring Disp16Char(unsigned char k)
{
    if(substrings[k].size())
        return substrings[k];

    ucs4 tmp = getucs4(k);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%u]", k);
        return AscToWstr(Buf);
    }
    else
    {
        wstring result;
        result += tmp;
        return result;
    }
}

static wstring Disp8Char(unsigned char k)
{
    ucs4 tmp = getucs4(k);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%u]", k);
        return AscToWstr(Buf);
    }
    else
    {
        wstring result;
        result += tmp;
        return result;
    }
}

static wstring DispFChar(unsigned char k)
{
    switch(k)
    {
        // 0x00..0x1F: blank
        case 0x20: return AscToWstr("[bladesymbol]");
        case 0x21: return AscToWstr("[bowsymbol]");
        case 0x22: return AscToWstr("[gunsymbol]");
        case 0x23: return AscToWstr("[armsymbol]");
        case 0x24: return AscToWstr("[swordsymbol]");
        case 0x25: return AscToWstr("[fistsymbol]");
        case 0x26: return AscToWstr("[scythesymbol]");
        case 0x27: return AscToWstr("[helmsymbol]");
        case 0x28: return AscToWstr("[armorsymbol]");
        case 0x29: return AscToWstr("[ringsymbol]");
        // 0x2A: "H"
        // 0x2B: "M"
        // 0x2C: "P"
        // 0x2D: ":"
        case 0x2D: return AscToWstr(":");
        case 0x2E: return AscToWstr("[shieldsymbol]");
        case 0x2F: return AscToWstr("[starsymbol]");
        // 0x30..: empty
        
        case 0x62: return AscToWstr("[handpart1]");
        case 0x63: return AscToWstr("[handpart1]");
        case 0x67: return AscToWstr("[hpmeter0]");
        case 0x68: return AscToWstr("[hpmeter1]");
        case 0x69: return AscToWstr("[hpmeter2]");
        case 0x6A: return AscToWstr("[hpmeter3]");
        case 0x6B: return AscToWstr("[hpmeter4]");
        case 0x6C: return AscToWstr("[hpmeter5]");
        case 0x6D: return AscToWstr("[hpmeter6]");
        case 0x6E: return AscToWstr("[hpmeter7]");
        case 0x6F: return AscToWstr("[hpmeter8]");
        
        default: return Disp8Char(k);
    }
}

static void DumpTable(const vector<string> &tab, wstring (*Disp)(unsigned char))
{
    for(unsigned a=0; a<tab.size(); ++a)
    {
        const string &s = tab[a];
        printf("$%u:", a);
        wstringOut conv;
        conv.SetSet(getcharset());

        string line;
        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(Disp(s[b]));
        puts(line.c_str());
    }
}

static void DumpZStrings(const unsigned offs, unsigned len, bool dolf=true)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<string> strings = LoadZStrings(offs, len);

    printf("*z;%u pointerstrings (12pix font)\n", strings.size());

    wstringOut conv;
    conv.SetSet(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*2;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        if(dolf) line += conv.putc('\n');
        
        const string &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
        {
            if(s[b] == 3)
            {
                char Buf[64];
                sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                line += conv.puts(AscToWstr(Buf));
            }
            else
                line += conv.puts(Disp16Char(s[b]));
        }
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void DumpStrings(const unsigned offs, unsigned len=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<string> strings = LoadZStrings(offs, len);

    printf("*r;%u pointerstrings (8pix font)\n", strings.size());

    wstringOut conv;
    conv.SetSet(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*2;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        
        const string &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
        {
            switch(s[b])
            {
                case 1:
                    line += conv.puts(AscToWstr("[nl]"));
                    break;
                case 5:
                {
                    char Buf[64];
                    unsigned c = (unsigned char)s[++b];
                    c += 256 * (unsigned char)s[++b];
                    sprintf(Buf, "[ptr %04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 8:
                {
                    char Buf[64];
                    sprintf(Buf, "[skip %u]", (unsigned char)s[++b]);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                    line += conv.puts(Disp8Char(s[b]));
            }
        }
        puts(line.c_str());
    }

    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void DumpFStrings(unsigned offs, unsigned len, unsigned maxcount=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<string> strings = LoadFStrings(offs, len, maxcount);
    printf("*l%u;%u fixed length strings (length: %u bytes)\n",
        len, strings.size(), len);

    wstringOut conv;
    conv.SetSet(getcharset());

    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*len;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        
        const string &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(DispFChar(s[b]));
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void LoadDict(unsigned offs, unsigned len)
{
    vector<string> strings = LoadPStrings(offs, len);
    printf("*d%06X:%u ;%u substrings in dictionary\n",
        offs, strings.size(), strings.size());

    wstringOut conv;
    conv.SetSet(getcharset());

    for(unsigned a=0; a<strings.size(); ++a)
    {
        char Buf[64];
        sprintf(Buf, "$%u:", a);
        
        string line = conv.puts(AscToWstr(Buf));
        
        const string &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(Disp8Char(s[b]));
        puts(line.c_str());
    }
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const string &s = strings[a];
        wstring tmp;
        for(unsigned b=0; b<s.size(); ++b)tmp += getucs4(s[b]);
        substrings[a + 0x21] = tmp;
    }

    printf("\n\n");
}

static void ListSpaces(void)
{
    fprintf(stderr, "Dumping free space list...");
    unsigned pagecount = (space.size()+0xFFFF) >> 16;
    for(unsigned page=0; page<pagecount; ++page)
    {
        unsigned pageend = (page+1) << 16;
        if(pageend >= space.size()) pageend = space.size();
        unsigned pagesize = pageend - (page << 16);
        
        bool freehere = false;
        unsigned ptr = (page<<16);
        for(unsigned p=0; p<pagesize; ++p, ++ptr)
        {
            unsigned freebegin = p;
            while(p < pagesize && space[ptr])
                ++ptr, ++p;
                
            if(freebegin < p)
            {
                if(!freehere)
                {
                    printf("*s%02X ;Free space in segment $%02X:\n", page, page);
                    freehere = true;
                }
                printf("$%u:%04X;%04X\n", p-freebegin, freebegin, p);
            }
        }
        if(freehere)
            printf("\n\n");
    }
    fprintf(stderr, " done\n");
}

static void TgaPutB(FILE *fp, unsigned c) { fputc(c, fp); }
static void TgaPutW(FILE *fp, unsigned c) { fputc(c&255, fp); fputc(c >> 8, fp); } 
static void TgaPutP(FILE *fp, unsigned r,unsigned g,unsigned b)
{ TgaPutB(fp,r);TgaPutB(fp,g);TgaPutB(fp,b); }
static void OutImage(const string &fntemplate,
                     unsigned xdim, unsigned ydim,
                     const vector<char> &pixels)
{
    string filename = fntemplate + ".tga";
    
    FILE *fp = fopen(filename.c_str(), "wb");
    if(!fp) { perror(filename.c_str()); return; }
    
    TgaPutB(fp, 0); // id field len
    TgaPutB(fp, 1); // color map type
    TgaPutB(fp, 1); // image type code
    TgaPutW(fp, 0); // palette start
    TgaPutW(fp, 6); // palette size
    TgaPutB(fp, 24);// palette bitness
    TgaPutW(fp, 0);    TgaPutW(fp, 0);
    TgaPutW(fp, xdim); TgaPutW(fp, ydim);
    TgaPutB(fp, 8); // pixel bitness
    TgaPutB(fp, 0); //misc
    
    // border color:
    TgaPutP(fp, 192,0,0);
    // colours 0..3
    TgaPutP(fp, 255,255,255);
    TgaPutP(fp, 192,128, 32);
    TgaPutP(fp, 255,255,128);
    TgaPutP(fp, 0,0,0);
    // filler
    TgaPutP(fp, 160,160,160);
    
    for(unsigned y=ydim; y-->0; )
        fwrite(&pixels[y*xdim], 1, xdim, fp);
    
    fclose(fp);
}

static void Dump8x8sprites(unsigned spriteoffs, unsigned count)
{
    fprintf(stderr, "Dumping 8pix font...");
    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*8 + (xdim+1);
    const unsigned ypixdim = ydim*8 + (ydim+1);
    
    const char palette[] = {4,2,3,1};
    const char bordercolor=0;
    
    vector<char> pixels (xpixdim * ypixdim, bordercolor);
    
    unsigned offs = spriteoffs;
    for(unsigned a=0; a<count; ++a)
    {
        for(unsigned y=0; y<8; ++y)
        {
            unsigned xpos = (a%xdim) * (8+1) + 1;
            unsigned ypos = (a/xdim) * (8+1) + 1+y;
            
            unsigned char byte1 = ROM[offs];
            unsigned char byte2 = ROM[offs+1];
            offs += 2;
            for(unsigned x=0; x<8; ++x)
                pixels[(xpos + x) + xpixdim * ypos] = 
                   palette
                    [((byte1 >> (7-x))&1)
                  | (((byte2 >> (7-x))&1) << 1)];
        }
    }

    OutImage("ct8fn", xpixdim, ypixdim, pixels);
    fprintf(stderr, " done\n");
}

static void DumpFont(unsigned spriteoffs, unsigned sizeoffs)
{
    fprintf(stderr, "Dumping 12pix font...");
    const unsigned count = 0x100 - 0xA0;
    
    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*12 + (xdim+1);
    const unsigned ypixdim = ydim*12 + (ydim+1);
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    const char fillercolor=5;
    
    vector<char> pixels (xpixdim * ypixdim, bordercolor);
    
    for(unsigned a=0xA0; a<0x100; ++a)
    {
        unsigned hioffs = spriteoffs + 24 * a;
        unsigned looffs = spriteoffs + 24 * 0x100 + (a>>1)*24;
        
        unsigned width = ROM[sizeoffs + a];
        for(unsigned y=0; y<12; ++y)
        {
            unsigned xpos = ((a-0xA0)%xdim) * (12+1) + 1;
            unsigned ypos = ((a-0xA0)/xdim) * (12+1) + 1+y;
            
            unsigned char byte1 = ROM[hioffs];
            unsigned char byte2 = ROM[hioffs+1];
            unsigned char byte3 = ROM[looffs];
            unsigned char byte4 = ROM[looffs+1];
            hioffs += 2;
            looffs += 2;
            
            for(unsigned x=0; x<width; ++x)
            {
                if(x == 8)
                {
                    byte1 = byte3;
                    byte2 = byte4;
                    if(a&1) { byte1 = (byte1 & 15) << 4;
                              byte2 = (byte2 & 15) << 4;
                            }
                }
                
                pixels[(xpos + x) + xpixdim * ypos] = 
                    palette
                     [((byte1 >> (7-(x&7)))&1)
                   | (((byte2 >> (7-(x&7)))&1) << 1)];
            }
            for(unsigned x=width; x<12; ++x)
                pixels[(xpos + x) + xpixdim * ypos] = fillercolor;
        }
    }
    
    OutImage("ct16fn", xpixdim, ypixdim, pixels);
    fprintf(stderr, " done\n");
}

int main(void)
{
    fprintf(stderr,
        "Chrono Trigger script dumper version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    LoadROM();

    substrings.resize(256);
    
    printf("; Note: There is a one byte sequence for [nl] and three spaces.\n"
           ";       Don't attempt to save space by removing those spaces,\n"
           ";       you will only make things worse...\n"
           ";       Similar for [pause]s, [pausenl]s and [cls]s.\n"
           ";\n"
          );
    
    // 127 instead of 128: Don't load "..." (ellipsis)
    LoadDict(0x1EFA00, 127);
    
    // 0x01 and 0x02 are doublebyte things.
    // They eat the next character and use it
    // as a pointer to somewhere. Seems like
    // in the English version this has absolutely
    // no use whatsoever. Very variable width lines
    // seen using this.
    // 0x03 is delay
    // 0x04 seems to do nothing
    substrings[0x05] = AscToWstr("[nl]\n");
    substrings[0x06] = AscToWstr("[nl]\n   ");
    substrings[0x07] = AscToWstr("[pausenl]\n");
    substrings[0x08] = AscToWstr("[pausenl]\n   ");
    substrings[0x09] = AscToWstr("\n[cls]\n");
    substrings[0x0A] = AscToWstr("\n[cls]\n   ");
    substrings[0x0B] = AscToWstr("\n[pause]\n");
    substrings[0x0C] = AscToWstr("\n[pause]\n   ");
    substrings[0x0D] = AscToWstr("[num8]");
    substrings[0x0E] = AscToWstr("[num16]");
    substrings[0x0F] = AscToWstr("[num32]");
    // 0x10 seems to do nothing
    substrings[0x11] = AscToWstr("[member]");
    substrings[0x12] = AscToWstr("[tech]");
    // These names are hardcoded because the ROM was quite
    // obfuscated here. Same for Nadia and Epoch later.
    substrings[0x13] = AscToWstr("Crono");
    substrings[0x14] = AscToWstr("Marle");
    substrings[0x15] = AscToWstr("Lucca");
    substrings[0x16] = AscToWstr("Robo");
    substrings[0x17] = AscToWstr("Frog");
    substrings[0x18] = AscToWstr("Ayla");
    substrings[0x19] = AscToWstr("Magus");
    // 0x1A is the nick Ayla calls Crono
    substrings[0x1A] = AscToWstr("[crononick]");
    substrings[0x1B] = AscToWstr("[member1]");
    substrings[0x1C] = AscToWstr("[member2]");
    substrings[0x1D] = AscToWstr("[member3]");
    substrings[0x1E] = AscToWstr("Nadia");
    substrings[0x1F] = AscToWstr("[item]");
    substrings[0x20] = AscToWstr("Epoch");
    // 21..9F are filled by LoadDict()
    // A0..FF are the character set
    substrings[0xEE] = AscToWstr("[musicsymbol]");
    substrings[0xF0] = AscToWstr("[heartsymbol]");
    substrings[0xF1] = AscToWstr("...");
    
    Dump8x8sprites(0x3F8C60, 256);
    DumpFont(0x3F2060, 0x26046);
    
    // 
    puts(";items");
    DumpFStrings(0x0C0B5E, 11, 242);
    puts(";item descriptions");
    DumpZStrings(0x0C2EB1, 242, false);
    
    puts(";techs");
    DumpFStrings(0x0C15C4, 11);
    puts(";tech descs");
    DumpZStrings(0x0C3A09, 117, false);
    
    puts(";tech related");
    DumpZStrings(0x0C3AF3, 4, false);
    
    puts(";monsters");
    DumpFStrings(0x0C6500, 11); // monsters

    puts(";Weapon Helmet Armor Accessory");
    DumpFStrings(0x02A3BA, 10);

    puts(";Location titles");
    DumpZStrings(0x06F400, 112, false);
    
    puts(";Battle announcements");
    DumpZStrings(0x0EEF11, 14, false);
    
    puts(";Battle messages");
    DumpZStrings(0x0CCBC9, 227, false);
    
    puts(";600ad (castle, masa+mune, naga-ette)");
    puts(";12kbc daltonstuff");
    DumpZStrings(0x18D000, 78);
    
    puts(";65Mbc");
    DumpZStrings(0x18DD80, 254);
    
    puts(";2300ad (factory, sewer, belthasar)");
    puts(";65Mbc azalastuff");
    puts(";slideshow-ending");
    DumpZStrings(0x1EC000, 187);
    
    puts(";1000ad (towns, castle)");
    puts(";600ad (towns)");
    puts(";2300ad (factory)");
    DumpZStrings(0x1EE300, 145);
    
    puts(";Treasure box messages");
    DumpZStrings(0x1EFF00, 3, false);

    puts(";1000ad (Lucca's home)");
    puts(";2300ad (factory)");
    puts(";1000ad (rainbow shell trial)    ");
    DumpZStrings(0x36A000, 106);

    puts(";no Crono -ending");
    puts(";happy ending (castle)");
    DumpZStrings(0x36B230, 144);

    puts(";1000ad (various indoors)");
    puts(";600ad (various indoors)");
    puts(";2300ad (various indoors)");
    DumpZStrings(0x370000, 456);

    puts(";2300ad (various indoors)");
    puts(";end of time (gaspar's stories, Spekkio etc)");
    puts(";600ad (Ozzie's scenes, Magus battle, castle)");
    puts(";1999ad Lavos scenes");
    puts(";12kbc various scenes");
    puts(";1000ad castle scenes");
    DumpZStrings(0x374900, 1203);

    puts(";1000ad jail scenes");
    puts(";600ad bridge battle stuff");
    puts(";1000ad (melchior, medina village)");
    puts(";65Mbc");
    puts(";12kbc");
    puts(";600ad (Toma stuff, Marco&Fiona stuff)");
    puts(";1000ad (Cyrus stuff)");
    puts(";Black Omen");
    DumpZStrings(0x384650, 678);

    puts(";600ad (Cathedral, other indoors)");
    puts(";12kbc (out- and indoors)");
    DumpZStrings(0x39B000, 444);

    puts(";1000ad (fair, the trial, castle)");
    puts(";600ad (Frog scenes)");
    puts(";12kbc");
    puts(";2300ad (death's peak)");
    DumpZStrings(0x3CBA00, 399);

    puts(";Dreamteam etc");
    puts(";Forest scene");
    DumpZStrings(0x3F4460, 81);
    
    puts(";12kbc cities");
    DumpZStrings(0x3F5860, 85);

    puts(";?");
    DumpZStrings(0x3F6B00, 186);

    puts(";Battle tutorials, Zeal stuff, party stuff");
    DumpZStrings(0x3F8400, 39);
    
    puts(";Item types");
    DumpStrings(0x3FB310, 242);

    puts(";Status screen string");
    //FIXME: These are not correctly dumped yet!
    //DumpStrings(0x3F158D, 1);
    //DumpStrings(0x3F0D67, 1);
    //DumpStrings(0x3FC457, 128);
    DumpStrings(0x3FC457+2* 2, 2);
    DumpStrings(0x3FC457+2* 7, 1);
    DumpStrings(0x3FC457+2*11, 2);
    DumpStrings(0x3FC457+2*16, 6);
    DumpStrings(0x3FC457+2*25, 2);
    //DumpStrings(0x3FC457+2*32, 3);
    DumpStrings(0x3FC457+2*33, 2);
    DumpStrings(0x3FC457+2*37, 18);
    DumpStrings(0x3FC457+2*57, 2);
    
    puts(";Misc prompts");
    DumpZStrings(0x3FCF3B, 7);
    
    puts(";Configuration");
    DumpZStrings(0x3FD3FE, 98);
    
    puts(";Era list");
    DumpStrings(0x3FD396, 8);
    
    puts(";Episode list");
    DumpStrings(0x3FD03E, 27);
    
    if(TryFindExtraSpace)
        FindEndSpaces();

    ListSpaces();
    
    fprintf(stderr, "Done\n");
}
