#include <cstdio>
#include <set>

#include "rommap.hh"
#include "strload.hh"

using namespace std;

#define LOADP_DEBUG      0
                         #define LOADZ_DEBUG      0
#define LOADZ_EXTRASPACE 0
#define LOADP_EXTRASPACE 0

const vector<ctstring> LoadPStrings(unsigned offset, unsigned count)
{
    unsigned segment = offset & 0xFF0000;
    vector<ctstring> result;
    result.reserve(count);
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
        
        unsigned length = ROM[stringptr];
        ctstring foundstring;
        foundstring.reserve(length);
        for(unsigned a=0; a<length; ++a) foundstring += (ctchar)ROM[stringptr+a+1];
        
        result.push_back(foundstring);
        offset += 2;
        
        MarkFree(stringptr, length + 1);

#if LOADP_EXTRASPACE
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

const ctstring LoadZString(unsigned offset, const extrasizemap_t& extrasizes)
{
    ctstring foundstring;
    
    for(unsigned p=offset; ; ++p)
    {
        if(ROM[p] == 0) break;
        unsigned int byte = ROM[p];
        
        /* FIXME: Invent a better way to see this */
        if(extrasizes.size() == 1)
        {
            if(byte == 1 || byte == 2)
                byte = byte*256 + ROM[++p];
        }
        
        foundstring += (ctchar)byte;
        
        extrasizemap_t::const_iterator i = extrasizes.find(byte);
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

const vector<ctstring> LoadZStrings(unsigned offset, unsigned count,
                                    const extrasizemap_t& extrasizes)
{
    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    vector<ctstring> result;
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
        
        ctstring foundstring = LoadZString(stringptr+base, extrasizes);
        
        if(stringptr < firstoffs) firstoffs = stringptr;
        if(stringptr > lastoffs)
        {
            lastoffs=  stringptr;
            lastlen = foundstring.size();
        }
        
#if LOADZ_EXTRASPACE
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

const vector<ctstring> LoadFStrings(unsigned offset, unsigned len, unsigned maxcount)
{
    ctstring str;
    for(unsigned a=0; ROM[offset+a]; ++a)
        str += (ctchar) ROM[offset+a];
    
    unsigned count = str.size() / len + 1;
    if(maxcount && count > maxcount)count = maxcount;
    vector<ctstring> result(count);
    for(unsigned a=0; a<count; ++a)
        result[a] = str.substr(a*len, len);
    return result;
}
