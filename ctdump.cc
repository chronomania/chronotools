#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <set>

using namespace std;

#include "ctcset.hh"
#include "miscfun.hh"

static vector<unsigned char> ROM;
static vector<bool> space;
static vector<string> substrings;

static const bool TryFindExtraSpace = false;
static const unsigned ExtraSpaceMinLen = 128;

static void LoadROM()
{
    unsigned hdrskip = 0;
    const char *fn = "chrono-uncompressed.smc";
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
    ROM.resize(ftell(fp)-hdrskip);
    fseek(fp, hdrskip, SEEK_SET);
    fread(&ROM[0], 1, ROM.size(), fp);
    fclose(fp);
    space.clear();
    space.resize(ROM.size());
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
    const unsigned maxco=10;
    unsigned col=maxco;
    for(unsigned a=0; a<count; ++a)
    {
        unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(maxco==10 && col==5)putchar(' ');
        printf(" $%04X", stringptr);
        if(++col == maxco) { printf("\n"); col=0; }
        
        stringptr += segment;
        strings[a] = string((const char *)&ROM[stringptr] + 1, ROM[stringptr]);
        offset += 2;
        
        markspace(stringptr, strings[a].size()+1);

        for(unsigned freebyte = stringptr + strings[a].size()+1;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; space[freebyte++]=true);
    }
    if(col)printf("\n");
    return strings;
}

// Load an array of C style strings
static const vector<string> LoadZStrings(unsigned offset, unsigned count=0)
{
    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    const unsigned maxco=10;
    vector<string> strings(count);
    unsigned col=maxco;
    set<unsigned> offsetlist;
    
    unsigned firstoffs = 0x10000, lastoffs = 0, lastlen = 0;
    for(unsigned a=0; !count || a<count; ++a)
    {
        const unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        if(offsetlist.find(offset & 0xFFFF) != offsetlist.end())
            break;
        offsetlist.insert(stringptr);

        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(maxco==10 && col==5)putchar(' ');
        printf(" $%04X", stringptr);
        if(++col == maxco) { printf("\n"); col=0; }
        
        const string foundstring((const char *)&ROM[stringptr + base]);
        
        markspace(stringptr+base, foundstring.size()+1);
        
        if(stringptr < firstoffs) firstoffs = stringptr;
        if(stringptr > lastoffs)
        {
            lastoffs=  stringptr;
            lastlen = foundstring.size();
        }
        for(unsigned freebyte = stringptr + base + foundstring.size();
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; space[freebyte++]=true);
        
        if(count)
            strings[a] = foundstring;
        else
            strings.push_back(foundstring);
        
        offset += 2;
    }
    if(col)printf("\n");
    
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

static wstring Disp16Char(unsigned char k)
{
    switch(k)
    {
        case 0x05: return AscToWstr("[nl]\n");
        case 0x06: return AscToWstr("[nl]\n   ");
    
        // Pausing cls (used by Magus in his ending)
        case 0x09: return AscToWstr("\n[cls1]\n");

        // Nonpause cls (used by gaspar in blah blah etc etc)
        case 0x0A: return AscToWstr("\n[cls2]\n");
    
        case 0x0B: return AscToWstr("\n[pause]\n");
        case 0x0C: return AscToWstr("\n[pause]\n   ");
    
        case 0x0D: return AscToWstr("[num8]");
        case 0x0E: return AscToWstr("[num16]");
        case 0x0F: return AscToWstr("[num32]");
        case 0x11: return AscToWstr("[member]");
        case 0x12: return AscToWstr("[tech]");
    
        // 13..19 are the character names

        // 1A is the name Ayla calls Crono?
        case 0x1A: return AscToWstr("[crononick]");
    
    
        case 0x1B: return AscToWstr("[member1]");
        case 0x1C: return AscToWstr("[member2]");
        case 0x1D: return AscToWstr("[member3]");
    
        // 1E is Nadia (princess)

        case 0x1F: return AscToWstr("[item]");
    
        // 20 is Epoch
        // 21..9F are substrings.
        // A0..FF are the character set.

        // The character set contains some special symbols.
        case 0xEE: return AscToWstr("[musicsymbol]");
        case 0xF0: return AscToWstr("[heartsymbol]");
    }
    
    if(substrings[k].size())
    {
        wstring result;
        const string &s = substrings[k].c_str();
        for(unsigned a=0; a<s.size(); ++a)
            result += Disp16Char((unsigned char)s[a]);
        return result;
    }

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
        case 0x2E: return AscToWstr("[shieldsymbol]");
        case 0x2F: return AscToWstr("[starsymbol]");
        // 0x30..: empty
        default: return Disp8Char(k);
    }
}

static void DumpScript(const vector<string> &tab, bool dolf)
{
    for(unsigned a=0; a<tab.size(); ++a)
    {
        const string &s = tab[a];
        printf("$%u", a);
        putchar(dolf ? '\n' : ':');
        
        wstringOut conv;
        conv.SetSet(getcharset());

        string line;
        for(unsigned b=0; b<s.size(); ++b)
        {
            if(s[b] == 3)
            {
                char Buf[32];
                sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                for(unsigned a=0; Buf[a]; ++a)
                    line += conv.putc(Buf[a]);
            }
            else
                line += conv.puts(Disp16Char(s[b]));
        }
        puts(line.c_str());
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

static void DumpZStrings(unsigned offs, unsigned len, bool dolf=true)
{
    vector<string> strings = LoadZStrings(offs, len);
    printf("*%02X:%04X:Z16 ;%u 16pix %sstrings, table ends at $%04X\n",
        offs>>16, offs&0xFFFF, strings.size(),
        dolf?"dialog ":"single line ",
        (offs+len*2)&0xFFFF);
    DumpScript(strings, dolf);
    printf("\n\n");
}

static void DumpStrings(unsigned offs, unsigned len=0)
{
    vector<string> strings = LoadZStrings(offs, len);
    printf("*%02X:%04X:R8 ;%u dialog strings, table ends at $%04X\n",
        offs>>16, offs&0xFFFF, strings.size(),
        (offs+len*2)&0xFFFF);
    DumpTable(strings, Disp8Char);
    printf("\n\n");
}

static void DumpFStrings(unsigned offs, unsigned len, unsigned maxcount=0)
{
    vector<string> strings = LoadFStrings(offs, len, maxcount);
    printf("*%02X:%04X:L%u ;%u fixed length strings (length: %u bytes)\n",
        offs>>16, offs&0xFFFF, len, strings.size(), len);
    DumpTable(strings, DispFChar);
    printf("\n\n");
}

static void LoadDict(unsigned offs, unsigned len)
{
    vector<string> hmm = LoadPStrings(offs, len);
    printf("*%02X:%04X:D%u ;%u substrings in dictionary\n",
        offs>>16, offs&0xFFFF, hmm.size(), hmm.size());
    
    DumpTable(hmm, Disp8Char);
    
    for(unsigned a=0; a<hmm.size(); ++a)
        substrings[a + 0x21] = hmm[a];

    printf("\n\n");
}

static void FindEndSpaces(void)
{
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
}

static void ListSpaces(void)
{
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
                    printf("*%02X:S ;Free space in segment $%02X:\n", page, page);
                    freehere = true;
                }
                printf("$%u:%04X;%04X\n", p-freebegin, freebegin, p);
            }
        }
        if(freehere)
            printf("\n\n");
    }
}

/* ASCII to CT character set */
static const string revtrans(const char *s)
{
    string result;
    while(*s)
        result += getchronochar(*s++);
    return result;
}

static void Dump8x8sprites(unsigned spriteoffs, unsigned count)
{
    unsigned offs = spriteoffs;
    for(unsigned a=0; a<count; ++a)
    {
        for(unsigned y=0; y<8; ++y)
        {
            unsigned char byte1 = ROM[offs];
            unsigned char byte2 = ROM[offs+1];
            offs += 2;
            for(unsigned x=0; x<8; ++x)
                putchar(".coO"
                    [((byte1 >> (7-x))&1) | (((byte2 >> (7-x))&1) << 1)]
                );
            putchar('\n');
            fflush(stdout);
        }
        putchar('\n');
    }
}
static void DumpFont(unsigned spriteoffs, unsigned sizeoffs, unsigned count)
{
    unsigned offs = spriteoffs;
    for(unsigned a=0; a<count; ++a)
    {
        unsigned width = ROM[sizeoffs + a];
        printf("[%u]\n", width); fflush(stdout);
        for(unsigned y=0; y<12; ++y)
        {
            unsigned char byte1 = ROM[offs];
            unsigned char byte2 = ROM[offs+1];
            offs += 2;
            for(unsigned x=0; x<=width; ++x)
                putchar(".coO"
                    [((byte1 >> (7-x))&1) | (((byte2 >> (7-x))&1) << 1)]
                );
            putchar('\n');
        }
        putchar('\n');
        fflush(stdout);
    }
}

int main(void)
{
    fprintf(stderr,
        "Chrono Trigger script dumper version "VERSION"\n"
        "Copyright (C) 1992,2002 Bisqwit (http://bisqwit.iki.fi/)\n");
    
    LoadROM();

#if 1

    substrings.resize(256);
    
    printf("; Note: There is a one byte sequence for [nl] and three spaces.\n"
           ";       Don't attempt to save space by removing those spaces,\n"
           ";       you will only make things worse...\n"
           ";       Similar for [pause]s.\n"
           ";\n"
          );
    
    // Don't load "..."
    LoadDict(0x1EFA00, 127);

    // We hardcode these because the
    // ROM was a bit obfuscated here...
    substrings[0x13] = revtrans("Crono");
    substrings[0x14] = revtrans("Marle");
    substrings[0x15] = revtrans("Lucca");
    substrings[0x16] = revtrans("Robo");
    substrings[0x17] = revtrans("Frog");
    substrings[0x18] = revtrans("Ayla");
    substrings[0x19] = revtrans("Magus");

    substrings[0x1E] = revtrans("Nadia");
    substrings[0x20] = revtrans("Epoch");
    
    substrings[0xF1] = revtrans("...");

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
    //DumpStrings(0x3FC457, 64);
    //DumpStrings(0x3F158D, 1);
    //DumpStrings(0x3F0D67, 1);
    
    DumpZStrings(0x3FCF3B, 7);
    
    puts(";Era list");
    DumpStrings(0x3FD396);
    
    puts(";Episode list");
    DumpStrings(0x3FD03E);
    
    if(TryFindExtraSpace)
        FindEndSpaces();

#else

    // The font for the magic 0x60 characters
    // starts actually from 0x3F9660
    //Dump8x8sprites(0x3F9360, 142);
    // This is the beginning for the 0x60 chars
    DumpFont(0x3F2F60, 0x260E5, 64);

#endif

    ListSpaces();
}
