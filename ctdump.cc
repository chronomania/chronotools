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

static void LoadROM()
{
    unsigned hdrskip = 0;
    FILE *fp = fopen("chrono-uncompressed.smc", "rb");
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
static const vector<string> LoadFStrings(unsigned offset, unsigned len)
{
    string str((const char *)&ROM[offset]);
    unsigned count = str.size() / len + 1;
    vector<string> result(count);
    for(unsigned a=0; a<count; ++a)
        result[a] = str.substr(a*len, len);
    return result;
}

static void Disp16Char(unsigned char k)
{
    if(k == 0x05) { printf("[nl]\n"); return; }
    if(k == 0x06) { printf("[nl]\n   "); return; }
    
    // Pausing cls (used by Magus in his ending)
    if(k == 0x09) { printf("\n[cls1]\n"); return; }

    // Nonpause cls (used by gaspar in blah blah etc etc)
    if(k == 0x0A) { printf("\n[cls2]\n"); return; }
    
    if(k == 0x0B) { printf("\n[pause]\n"); return; }
    if(k == 0x0C) { printf("\n[pause]\n   "); return; }
    
    if(k == 0x0D) { printf("[num8]"); return; }
    if(k == 0x0E) { printf("[num16]"); return; }
    
    // 13..19 are the character names

    // 1A is the name Ayla calls Crono?
    if(k ==0x1A) { printf("[crononick]"); return; }
    
    
    if(k ==0x1B) { printf("[member1]"); return; }
    if(k ==0x1C) { printf("[member2]"); return; }
    if(k ==0x1D) { printf("[member3]"); return; }
    
    // 1E is Nadia (princess)

    if(k ==0x1F) { printf("[item]"); return; }
    
    // 20 is Epoch
    // 21..9F are substrings.
    // A0..EF are the character set.
    // F0..FF are ???

    if(k == 0xEE) { printf("[musicsymbol]"); return; }
    if(k == 0xF0) { printf("[heartsymbol]"); return; }
    
    if(substrings[k].size())
    {
        const string &s = substrings[k].c_str();
        for(unsigned a=0; a<s.size(); ++a)
            Disp16Char((unsigned char)s[a]);
        return;
    }
    
    char cset = characterset[k];
    if(cset == '¶')
        printf("[%u]", k);
    else
        putchar(cset);
}

static void DispFChar(unsigned char k)
{
    if(k == 0x20) { printf("[bladesymbol]"); return; }
    if(k == 0x21) { printf("[bowsymbol]"); return; }
    if(k == 0x22) { printf("[gunsymbol]"); return; }
    if(k == 0x23) { printf("[armsymbol]"); return; }
    if(k == 0x24) { printf("[swordsymbol]"); return; }
    if(k == 0x25) { printf("[fistsymbol]"); return; }
    if(k == 0x26) { printf("[scythesymbol]"); return; }
    if(k == 0x27) { printf("[helmsymbol]"); return; }
    if(k == 0x28) { printf("[armorsymbol]"); return; }
    if(k == 0x29) { printf("[ringsymbol]"); return; }
    if(k == 0x2F) { printf("[starsymbol]"); return; }
    
    char cset = characterset[k];
    if(cset == '¶')
        printf("[%u]", k);
    else
        putchar(cset);
}

static void Disp8Char(unsigned char k)
{
    char cset = characterset[k];
    if(cset == '¶')
        printf("[%u]", k);
    else
        putchar(cset);
}

static void DumpScript(const vector<string> &tab, bool dolf)
{
    for(unsigned a=0; a<tab.size(); ++a)
    {
        const string &s = tab[a];
        printf("$%u", a);
        if(dolf)putchar('\n');else printf(":\t");

        for(unsigned b=0; b<s.size(); ++b)
        {
            if(s[b] == 3)
                printf("[delay %02X]", (unsigned char)s[++b]);
            else
                Disp16Char(s[b]);
        }
        printf("\n");
    }
}

static void DumpTable(const vector<string> &tab, void (*Disp)(unsigned char))
{
    for(unsigned a=0; a<tab.size(); ++a)
    {
        const string &s = tab[a];
        printf("$%u:\t", a);
        for(unsigned b=0; b<s.size(); ++b)
            Disp(s[b]);
        printf("\n");
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

static void DumpFStrings(unsigned offs, unsigned len)
{
    vector<string> strings = LoadFStrings(offs, len);
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

    substrings[0xF1] = hmm[127];
    for(unsigned a=0; a<hmm.size()-1; ++a)
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
        
        static const char blanks[3] = {0x00, (char)0xEF, (char)0xFF};
        
        for(unsigned blank=0; blank<3; ++blank)
        {
            unsigned blanklen=0;
            for(unsigned blapos=pagebegin; blapos<pageend; ++blapos)
            {
                if(ROM[blapos] == blanks[blank])
                {
                    ++blanklen;
                    if(blanklen == 80)
                    {
                        for(unsigned a=0; a<80; ++a)
                            space[blapos-63+a] = true;
                    }
                    if(blanklen >= 80)
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
                printf("$%u:\t%04X;%04X\n", p-freebegin, freebegin, p);
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
    {
        const char *c = strchr(characterset, *s);
        if(!c)result += '?';
        else result += (char)(c - characterset);
        ++s;
    }
    return result;
}

int main(void)
{
    LoadROM();
    substrings.resize(256);
    
    printf("; Note: There is a one byte sequence for [nl] and three spaces.\n"
           ";       Don't attempt to save space by removing those spaces,\n"
           ";       you will only make things worse...\n"
           ";       Similar for [pause]s.\n"
           ";\n"
          );
    
    LoadDict(0x1EFA00, 128);
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

    // 
    DumpFStrings(0x0C0B5E, 11); // items
    DumpFStrings(0x0C6500, 11); // monsters

    // Weapon Helmet Armor Accessory
    DumpFStrings(0x02A3BA, 10);

    // Location titles
    DumpZStrings(0x06F400, 112, false);
    
    // Techniques descriptions
    DumpZStrings(0x0C3A09, 121, false);
    
    // Battle messages
    DumpZStrings(0x0CCBC9, 227, false);
    
    // ?
    DumpZStrings(0x18D000, 78);
    
    // ?
    DumpZStrings(0x18DD80, 254);
    
    // ?
    DumpZStrings(0x1EC000, 187);
    
    // ?
    DumpZStrings(0x1EE300, 145);
    
    // ?
    DumpZStrings(0x1EFF00, 3);

    // ?
    DumpZStrings(0x36A000, 106);

    // ?
    DumpZStrings(0x36B230, 144);

    // ?
    DumpZStrings(0x370000, 456);

    // ?
    DumpZStrings(0x374900, 1203);

    // ?
    DumpZStrings(0x384650, 678);

    // ?
    DumpZStrings(0x39B000, 444);

    // ?
    DumpZStrings(0x3CBA00, 399);

    // Dreamteam etc
    DumpZStrings(0x3F4460, 81);
    
    // Enhasa
    DumpZStrings(0x3F5860, 85);

    // ?
    DumpZStrings(0x3F6B00, 186);

    // Battle tutorials, Zeal stuff, party stuff
    DumpZStrings(0x3F8400, 39);
    
    DumpStrings(0x3FB310, 242);

    // Status screen string
    //FIXME: These are not correctly dumped yet!
    //DumpStrings(0x3FC457, 64);
    //DumpStrings(0x3F158D, 1);
    //DumpStrings(0x3F0D67, 1);
    
    DumpZStrings(0x3FCF3B, 7);
    
    // Era list
    DumpStrings(0x3FD396);
    
    // Episode list
    DumpStrings(0x3FD03E);
    
    FindEndSpaces();
    ListSpaces();
}
