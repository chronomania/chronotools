#include <cstdio>
#include <set>

#include "rommap.hh"
#include "strload.hh"
#include "logfiles.hh"
#include "rangeset.hh"

using namespace std;

#define LOADP_DEBUG      1
#define LOADZ_DEBUG      1
#define LOADZ_EXTRASPACE 0
#define LOADP_EXTRASPACE 0

namespace
{
    class TableDumper
    {
        static const unsigned maxco = 4;
        unsigned col;
        FILE *log;
        
        rangeset<unsigned> ranges;
        
    public:
        TableDumper(const string& type, unsigned offset)
            : col(maxco), log(GetLogFile("mem", "log_addrs"))
        {
            if(log)
                fprintf(log, "-- %s at %06X\n", type.c_str(), offset);
        }
        void AddPtr(unsigned ptrnum, unsigned where, unsigned target, unsigned bytes)
        {
            if(log)
            {
                if(col==maxco){fprintf(log, "ptr%2u ", ptrnum);col=0;}
                else if(!col)fprintf(log, "%5u ", ptrnum);
                if(!col)
                {
                    const unsigned noffs = where;
                    for(unsigned k=62*62*62; ; k/=62)
                    {
                        unsigned dig = (noffs/k)%62;
                        if(dig < 10) fputc('0' + dig, log);
                        else if(dig < 36) fputc('A' + (dig-10), log);
                        else fputc('a' + (dig-36), log);
                        if(k==1)break;
                    }
                }
                if(col == maxco/2)fputc(' ', log);
                fprintf(log, " $%04X-%04X", target, target+bytes-1);
                
                ranges.set(target, target+bytes);
                
                if(++col == maxco) { fprintf(log, "\n"); col=0; }
            }
        }
        void Finish(unsigned where)
        {
            if(log)
            {
                ranges.compact();
                
                if(col)fprintf(log, "\n");
                fprintf(log, "-- Table ends at %06X\n", where);

                list<rangeset<unsigned>::const_iterator> rangelist;
                ranges.find_all_coinciding(0,0x10000, rangelist);

                for(list<rangeset<unsigned>::const_iterator>::const_iterator
                    j = rangelist.begin();
                    j != rangelist.end();
                    ++j)
                {
                    fprintf(log, "--  Uses memory range $%04X-%04X\n",
                        (*j)->lower,
                        (*j)->upper-1);
                }
                
                fprintf(log, "\n");

                fflush(log);
            }
        }
    };
    
    const ctstring LoadPString(unsigned offset,
                               unsigned& bytes,
                               const string& what)
    {
        ctstring foundstring;

        bytes = ROM[offset++];

        foundstring.reserve(bytes);
        for(unsigned a=0; a<bytes; ++a)
        {
            unsigned int byte = ROM[offset++];
            if(byte == 1 || byte == 2)
            {
                ++a; ++bytes;
                byte = byte*256 + ROM[offset++];
            }
            foundstring += (ctchar)byte;
        }
        ++bytes; // the length-byte
        MarkFree(offset-bytes, bytes, what);
        return foundstring;
    }
}

const vector<ctstring> LoadPStrings(unsigned offset, unsigned count,
                                    const string& what
                                   )
{
    unsigned segment = offset & 0xFF0000;
    vector<ctstring> result;
    result.reserve(count);
    const string what_p = what+" pointers";
    const string what_d = what+" data";
    
#if LOADP_DEBUG
    TableDumper logger(what_p, offset);
#endif
    
    MarkProt(offset, count*2, what_p);
    for(unsigned a=0; a<count; ++a)
    {
        unsigned stringptr = ROM[offset] + 256*ROM[offset + 1];
        
        unsigned bytes;
        ctstring foundstring = LoadPString(stringptr+segment, bytes, what_d);
        
#if LOADP_DEBUG
        logger.AddPtr(a, offset, stringptr, bytes);
#endif
        
        result.push_back(foundstring);
        offset += 2;
        
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
    logger.Finish(offset);
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
    set<unsigned> offsetlist;
    
    const string what_p = what+" pointers";
    const string what_d = what+" data";

#if LOADZ_DEBUG
    TableDumper logger(what_p, offset);
#endif
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
        
        unsigned bytes;
        ctstring foundstring = LoadZString(stringptr+base, bytes, what_d, extrasizes);

#if LOADZ_DEBUG
        logger.AddPtr(a, offset, stringptr, bytes);
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
    logger.Finish(offset);
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
