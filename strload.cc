#include <cstdio>
#include <set>

#include "rommap.hh"
#include "strload.hh"

using namespace std;

#define LOADP_DEBUG      0
#define LOADZ_DEBUG      0
#define LOADZ_EXTRASPACE 0
#define LOADP_EXTRASPACE 0

const vector<ctstring> LoadPStrings(unsigned offset, unsigned count,
                                    const string& what
                                   )
{
    unsigned segment = offset & 0xFF0000;
    vector<ctstring> result;
    result.reserve(count);
#if LOADP_DEBUG
    const unsigned maxco=10;
    unsigned col=maxco;
#endif
    
    const string what_p = what+" pointers";
    const string what_d = what+" data";

    MarkProt(offset, count*2, what_p);
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
        
        MarkFree(stringptr, length + 1, what_d);

#if LOADP_EXTRASPACE
        unsigned freebytepos=stringptr + result[a].size()+1;
        unsigned freebytecount=0;
        for(unsigned freebyte=freebytepos;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; ++freebyte,++freebytecount);
        MarkFree(freebytepos, freebytecount, "extra space");
#endif
    }
#if LOADP_DEBUG
    if(col)printf("\n");
    fflush(stdout);
#endif
    return result;
}

const ctstring LoadZString(unsigned offset,
                           unsigned &bytes,
                           const string& what,
                           const extrasizemap_t& extrasizes)
{
    ctstring foundstring;

    const unsigned beginoffs = offset;
    unsigned endoffs=beginoffs;
    
    for(unsigned p=offset; ; ++p)
    {
        endoffs = p+1;
        
        if(ROM[p] == 0) break;
        unsigned int byte = ROM[p];
        
        /* FIXME: Invent a better way to see this */
        if(extrasizes.size() == 1)
        {
            if(byte == 1 || byte == 2)
            {
                ++endoffs;
                byte = byte*256 + ROM[++p];
            }
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
    
    bytes = endoffs - beginoffs;
    
    //printf("\n;Loaded %u bytes from $%06X\n", foundstring.size()+1, offset);
    
    MarkFree(offset, bytes, what);
    
    return foundstring;
}

const vector<ctstring> LoadZStrings(unsigned offset, unsigned count,
                                    const string& what,
                                    const extrasizemap_t& extrasizes)
{
    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    vector<ctstring> result;
    result.reserve(count);
#if LOADZ_DEBUG
    const unsigned maxco=6;
    unsigned col=maxco;
#endif
    set<unsigned> offsetlist;
    
    const string what_p = what+" pointers";
    const string what_d = what+" data";

    unsigned first_offs = offset;
    unsigned last_offs  = first_offs;
    for(unsigned a=0; !count || a<count; ++a, offset += 2)
    {
        const unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        // Jos tämä osoite on listattu jo kertaalleen, break.
        if(offsetlist.find(offset & 0xFFFF) != offsetlist.end())
            break;
        
        last_offs = offset+2;
        offsetlist.insert(stringptr);
        
#if LOADZ_DEBUG
        if(col==maxco){printf(";ptr%2u ", a);col=0;}
        else if(!col)printf(";%5u ", a);
        if(!col)
        {
            const unsigned noffs = offset;
            for(unsigned k=62*62*62; ; k/=62)
            {
                unsigned dig = (noffs/k)%62;
                if(dig < 10) putchar('0' + dig);
                else if(dig < 36) putchar('A' + (dig-10));
                else putchar('a' + (dig-36));
                if(k==1)break;
            }
        }
        if(maxco==6 && col==3)putchar(' ');
        printf(" $%04X", stringptr);
#endif
        
        unsigned bytes;
        ctstring foundstring = LoadZString(stringptr+base, bytes, what_d, extrasizes);

#if LOADZ_DEBUG
        printf("-%04X", stringptr+bytes);
        if(++col == maxco) { printf("\n"); col=0; }
#endif
        
#if LOADZ_EXTRASPACE
        unsigned freebytepos=stringptr + base + bytes;
        unsigned freebytecount=0;
        for(unsigned freebyte=freebytepos;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; ++freebyte,++freebytecount);
        MarkFree(freebytepos, freebytecount, "extra space");
#endif
        result.push_back(foundstring);
    }

    MarkProt(first_offs, last_offs-first_offs, what_p);

#if LOADZ_DEBUG
    if(col)printf("\n");
    fflush(stdout);
#endif
    return result;
}

const vector<ctstring> LoadFStrings(unsigned offset, unsigned len,
                                    const string& what,
                                    unsigned maxcount)
{
    ctstring str;
    for(unsigned a=0; ROM[offset+a] && a<len*maxcount; ++a)
        str += (ctchar) ROM[offset+a];
    
    unsigned count = str.size() / len + 1;
    if(maxcount && count > maxcount)count = maxcount;
    vector<ctstring> result(count);
    for(unsigned a=0; a<count; ++a)
        result[a] = str.substr(a*len, len);
    
    MarkProt(offset, len*maxcount, what);
    
    return result;
}
