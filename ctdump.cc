#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <map>

/* I am sorry for the spaghettiness of this small program... */

#ifdef linux
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

#define LOADP_DEBUG      1
#define LOADZ_DEBUG      1
//#define LOADZ_EXTRASPACE 1
//#define LOADP_EXTRASPACE 1

using namespace std;

#include "ctcset.hh"
#include "miscfun.hh"
#include "settings.hh"

static unsigned char *ROM;
static vector<bool> protect;
static vector<bool> space;
static vector<wstring> substrings;

static const bool TryFindExtraSpace = false;
static const unsigned ExtraSpaceMinLen = 128;

static void MarkingError(const set<unsigned> &errlist, const char *type)
{
    set<unsigned>::const_iterator i;
    
    bool begun=false;
    unsigned first=0;
    unsigned prev=0;
    
    for(i=errlist.begin(); ; ++i)
    {
        if(i == errlist.end() || (begun && prev < *i-1))
        {
            fprintf(stderr, "Error: %06X-%06X already marked %s\n", first,prev, type);
            begun = false;
            if(i == errlist.end()) break;
        }
        
        if(!begun) { begun = true; first = *i; }
        prev = *i;
    }
}
static void ShowProtMap()
{
    static const char *const types[4] = {"?","free","protected","ERROR"};
    
    fprintf(stderr, "Prot/free map:\n");
    unsigned romsize = space.size();
    
    bool begun=false;
    unsigned first=0;
    unsigned lasttype=0;
    
    for(unsigned a=0; a<=romsize; ++a)
    {
        unsigned type = 0;
        if(a == romsize
        || (begun && lasttype != (type = (space[a]?1:0) + (protect[a]?2:0)))
          )
        {
            fprintf(stderr, "  %06X-%06X: %s (%u bytes)\n",
                first,a-1, types[lasttype], a-first);
            begun = false;
            if(a == romsize) break;
        }
        if(!begun) { begun = true; first=a; }
        lasttype = type;
    }
}

static void MarkFree(unsigned begin, unsigned length)
{
    set<unsigned> error;
    //fprintf(stderr, "Marking %u bytes free at %06X\n", length, begin);
    for(; length>0; --length)
    {
        if(protect[begin]) error.insert(begin);
        space[begin++] = true;
    }
    
    if(error.size()) MarkingError(error, "protected, attempted to free");
}

static void MarkProt(unsigned begin, unsigned length)
{
    set<unsigned> error;
    
    //fprintf(stderr, "Marking %u bytes protected at %06X\n", length, begin);
    for(; length>0; --length)
    {
        if(space[begin]) error.insert(begin);
        protect[begin++] = true;
    }

    if(error.size()) MarkingError(error, "free, attempted to protect");
}

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
    protect.clear();
    protect.resize(romsize);
    fprintf(stderr, "\n");
}

// Load an array of pascal style strings
static const vector<string> LoadPStrings(unsigned offset, unsigned count)
{
    unsigned segment = offset & 0xFF0000;
    vector<string> result(count);
#if LOADP_DEBUG
    const unsigned maxco=10;
    unsigned col=maxco;
#endif
    MarkProt(offset, count*2);
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
        result[a] = string((const char *)&ROM[stringptr] + 1, ROM[stringptr]);
        offset += 2;
        
        MarkFree(stringptr, result[a].size()+1);

#ifdef LOADP_EXTRASPACE
        unsigned freebytepos=stringptr + result[a].size()+1;
        unsigned freebytecount=0;
        for(unsigned freebyte=freebytepos;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; ++freebyte,++freebytecount);
        MarkFree(freebytepos, freebytecount);
#endif
    }
#if LOADP_DEBUG
    if(col)printf("\n");
#endif
    return result;
}

static string LoadZString(unsigned offset, const map<unsigned,unsigned> &extrasizes)
{
    string foundstring;
    
    for(unsigned p=offset; ; ++p)
    {
        if(ROM[p] == 0) break;
        unsigned byte = ROM[p];
        foundstring += (char)byte;
        
        map<unsigned,unsigned>::const_iterator i;
        i = extrasizes.find(byte);
        if(i != extrasizes.end())
        {
            unsigned extra = i->second;
            while(extra-- > 0)
                foundstring += (char)ROM[++p];
        }
    }
    
    //printf("\n;Loaded %u bytes from $%06X\n", foundstring.size()+1, offset);
    
    MarkFree(offset, foundstring.size() + 1);
    
    return foundstring;
}

// Load an array of C style strings
static const vector<string> LoadZStrings(unsigned offset, unsigned count,
                                         const map<unsigned,unsigned> &extrasizes)
{
    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    vector<string> result;
    result.reserve(count);
#if LOADZ_DEBUG
    const unsigned maxco=10;
    unsigned col=maxco;
#endif
    set<unsigned> offsetlist;
    
    unsigned firstoffs = 0x10000, lastoffs = 0, lastlen = 0;
    for(unsigned a=0; !count || a<count; ++a)
    {
        const unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        // Jos tämä osoite on listattu jo kertaalleen, break.
        if(offsetlist.find(offset & 0xFFFF) != offsetlist.end())
            break;

        MarkProt(offset, 2);
        offsetlist.insert(stringptr);
        
#if LOADZ_DEBUG
        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(maxco==10 && col==5)putchar(' ');
        printf(" $%04X", stringptr);
        if(++col == maxco) { printf("\n"); col=0; }
#endif
        
        string foundstring = LoadZString(stringptr+base, extrasizes);
        
        if(stringptr < firstoffs) firstoffs = stringptr;
        if(stringptr > lastoffs)
        {
            lastoffs=  stringptr;
            lastlen = foundstring.size();
        }
        
#ifdef LOADZ_EXTRASPACE
        unsigned freebytepos=stringptr + base + foundstring.size();
        unsigned freebytecount=0;
        for(unsigned freebyte=freebytepos;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; ++freebyte,++freebytecount);
        MarkFree(freebytepos, freebytecount);
#endif
        result.push_back(foundstring);
        
        offset += 2;
    }
#if LOADZ_DEBUG
    if(col)printf("\n");
#endif
    return result;
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
#if 0
            vector<char> BlankBuf(64, blanks[blank]);
            
            const char *ptr = mempos(beginptr, size, &BlankBuf[0], BlankBuf.size());
            if(!ptr)continue;
            
            for(unsigned blapos = (ptr-beginptr);
                ROM[pagebegin+blapos] == blanks[blank]
             && blapos < pageend; ++blapos)
            {
                space[pagebegin+blapos] = true;
            }
#endif
        }
    }
    fprintf(stderr, " done\n");
}

static wstring Disp8Char(unsigned char k)
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
        
        /*
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
        */
        
        default:
        {
            ucs4 tmp = getucs4(k);
            if(tmp == ilseq)
            {
                char Buf[32];
                sprintf(Buf, "[%02X]", k);
                return AscToWstr(Buf);
            }
            else
            {
                wstring result;
                result += tmp;
                return result;
            }
        }
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
    
    map<unsigned,unsigned> extrasizes;
    extrasizes[0x12] = 1;
    vector<string> strings = LoadZStrings(offs, len, extrasizes);

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
            switch(s[b])
            {
                case 0x03:
                {
                    char Buf[64];
                    sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 0x12:
                {
                    char Buf[64];
                    ++b;
                    if(s[b] == 0x00)
                        strcpy(Buf, "[tech]");
                    else if(s[b] == 0x01)
                        strcpy(Buf, "[monster]");
                    else
                        sprintf(Buf, "[12][%02X]", (unsigned char)s[b]);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                {
                    unsigned char k = s[b];
                    
                    if(substrings[k].size())
                        line += conv.puts(substrings[k]);
                    else
                    {
                        ucs4 tmp = getucs4(k);
                        if(tmp == ilseq)
                        {
                            char Buf[32];
                            sprintf(Buf, "[%02X]", k);
                            line += conv.puts(AscToWstr(Buf));
                        }
                        else
                        {
                            line += conv.putc(tmp);
                        }
                    }
                }
            }
        }
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void Dump8Strings(const unsigned offs, unsigned len=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    map<unsigned,unsigned> extrasizes;
    extrasizes[2] = 2;
    extrasizes[3] = 2+2;
    extrasizes[4] = 3;
    extrasizes[5] = 2;
    extrasizes[6] = 2;
    extrasizes[7] = 2;
    extrasizes[8] = 1;
    extrasizes[9] = 1;
    extrasizes[10] = 1;
    extrasizes[11] = 2+2;
    extrasizes[12] = 1+2;
    vector<string> strings = LoadZStrings(offs, len, extrasizes);

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
        
        string s = strings[a];

        unsigned attr=0;
        for(unsigned b=0; b<s.size(); ++b)
        {
            char Buf[64];
Retry:
            switch(s[b])
            {
                case 1:
                {
                    line += conv.puts(AscToWstr("[next]"));
                    break;
                }
                case 2:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[goto,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 3:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func1,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 4:
                {
                    unsigned c1, c2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c1 = (unsigned char)s[++b];
                    
                    if(c1 >= 0xC0)
                    {
                        unsigned addr = ((c1&0x3F) << 16) + c2;
                        
                        string str = LoadZString(addr, extrasizes);
                        
                        //line += conv.putc(AscToWchar('{')); //}
                        
                        b -= 3;
                        s.erase(b, 4);
                        s.insert(b, str);
                        goto Retry;
                    }
                    
                    sprintf(Buf, "[substr,%02X%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 5:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[member,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 6:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[attrs,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 7:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[out,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 8:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[spc,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 9:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[len,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 10:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[attr,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    attr = c;
                    break;
                }
                case 11:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func2,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 12:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[++b];
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[stat,%02X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                    if(attr & 0x03)
                    {
                        line += conv.puts(AscToWstr("[gfx"));
                        for(;;)
                        {
                            unsigned char byte = s[b];
                            if(byte <= 12) { --b; break; }

                            char Buf[32];
                            sprintf(Buf, ",%02X", byte);
                            line += conv.puts(AscToWstr(Buf));
                            ++b;
                        }
                        line += conv.putc(AscToWchar(']'));
                    }
                    else
                    {
                        line += conv.puts(Disp8Char(s[b]));
                    }
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
            line += conv.puts(Disp8Char(s[b]));
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void LoadDict(unsigned offs, unsigned len)
{
    vector<string> strings = LoadPStrings(offs, len);
    printf("*d%u ;%u substrings in dictionary\n",
        strings.size(), strings.size());

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
                printf("$%04X:%04X ; %u\n", freebegin, p, p-freebegin);
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
    TgaPutP(fp, 128,128,128);
    // colours 0..3
    TgaPutP(fp, 192,192,192);
    TgaPutP(fp,  32, 48,128);
    TgaPutP(fp,  40, 72,192);
    TgaPutP(fp,  60, 90,255);
    // filler
    TgaPutP(fp, 255,255,255);
    
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
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    
    vector<char> pixels (xpixdim * ypixdim, bordercolor);
    
    MarkProt(spriteoffs, count*2);
    
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

static void DumpFont(unsigned skip, unsigned offs1, unsigned offs2, unsigned sizeoffs)
{
    fprintf(stderr, "Dumping 12pix font...");
    const unsigned count = 0x100 - skip;
    
    unsigned maxwidth = 12;
    
    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*maxwidth + (xdim+1);
    const unsigned ypixdim = ydim*maxwidth + (ydim+1);
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    const char fillercolor=5;
    
    vector<char> pixels (xpixdim * ypixdim, bordercolor);
    
    for(unsigned a=skip; a<0x100; ++a)
    {
        unsigned hioffs = offs1 + 24 * a;
        unsigned looffs = offs2 + 24 * (a >> 1);
        
        unsigned width = ROM[sizeoffs + a];
        
        if(width > maxwidth)width = maxwidth;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned xpos = ((a-skip)%xdim) * (maxwidth+1) + 1;
            unsigned ypos = ((a-skip)/xdim) * (maxwidth+1) + 1+y;
            
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

static void PatchSubStrings()
{
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
}

static void Dump12Font()
{
    unsigned char A0 = ROM[FirstChar_Address];
    
    unsigned WidthPtr = ROM[WidthTab_Address_Ofs+0]
                     + (ROM[WidthTab_Address_Ofs+1]<<8)
                     + ((ROM[WidthTab_Address_Seg] & 0x3F) << 16)
                     - ROM[WidthTab_Offset_Addr];
    
    unsigned FontSeg = ROM[Font12_Address_Seg] & 0x3F;
    unsigned FontPtr1 = ROM[Font12a_Address_Ofs+0]
                     + (ROM[Font12a_Address_Ofs+1] << 8)
                     + (FontSeg << 16);
    unsigned FontPtr2 = ROM[Font12b_Address_Ofs+0]
                     + (ROM[Font12b_Address_Ofs+1] << 8)
                     + (FontSeg << 16);
    
    if(FontPtr2 != FontPtr1 + 0x1800)
    {
        fprintf(stderr, "Error: We probably have wrong ROM.\n"
                        "%06X != %06X+1800\n", FontPtr2, FontPtr1);
    }
    
    DumpFont(A0, FontPtr1, FontPtr2, WidthPtr);
    
    MarkFree(FontPtr1, 256*24);
    MarkFree(FontPtr2, 256*12);
    
    MarkFree(WidthPtr+A0, 0x100-A0);
}

static void DoLoadDict()
{
    unsigned DictPtr = ROM[DictAddr_Ofs+0]
                    + (ROM[DictAddr_Ofs+1] << 8)
                   + ((ROM[DictAddr_Seg_1] & 0x3F) << 16);

    unsigned char A0 = ROM[WidthTab_Offset_Addr];
    
    unsigned n = A0-0x21;  // For A0, that is 127.
    
    LoadDict(DictPtr, n);

    // unprotect the dictionary because it will be relocated.
    for(unsigned a=0; a<n*2; ++a)protect[DictPtr+a]=false;
    
    MarkFree(DictPtr, n*2);
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
    
    DoLoadDict();
    
    PatchSubStrings();
    
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
    Dump8Strings(0x3FB310, 242);

    puts(";Status screen string");
    
    Dump8Strings(0x3FC457, 61);
    
    puts(";Misc prompts");
    DumpZStrings(0x3FCF3B, 7);
    
    puts(";Configuration");
    DumpZStrings(0x3FD3FE, 50);
    
    puts(";Era list");
    Dump8Strings(0x3FD396, 8);
    
    puts(";Episode list");
    Dump8Strings(0x3FD03E, 27);
    
    Dump8x8sprites(Font8_Address, 256);
    
    Dump12Font();
    
    if(TryFindExtraSpace)
        FindEndSpaces();

    ListSpaces();
    
    ShowProtMap();
    
    fprintf(stderr, "Done\n");
}
