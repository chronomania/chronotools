#include <cstdio>
#include <set>

#include "rommap.hh"
#include "romaddr.hh"
#include "strload.hh"
#include "logfiles.hh"
#include "rangeset.hh"
#include "scriptfile.hh"

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
        TableDumper(const std::wstring& type, unsigned offset)
            : col(maxco), log(GetLogFile("mem", "log_addrs"))
        {
            if(log)
                fprintf(log, "-- %s at %06X\n", WstrToAsc(type).c_str(), offset);
        }
        void AddPtr(unsigned ptrnum, unsigned where, unsigned target, unsigned bytes)
        {
            if(log)
            {
                if(col==maxco){fprintf(log, "ptr%2u ", ptrnum);col=0;}
                else if(!col)fprintf(log, "%5u ", ptrnum);
                if(!col)
                {
                    std::wstring label = Base62Label(where);
                    fprintf(log, "%s", WstrToAsc(label).c_str());
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
                if(col)fprintf(log, "\n");
                fprintf(log, "-- Table ends at %06X\n", where);
                
                for(rangeset<unsigned>::const_iterator
                    i = ranges.begin();
                    i != ranges.end();
                    ++i)
                {
                    fprintf(log, "--  Uses memory range $%04X-%04X\n",
                        i->lower,
                        i->upper-1);
                }
                
                fprintf(log, "\n");

                fflush(log);
            }
        }
    
    private:
        TableDumper(const TableDumper&);
        TableDumper& operator=(const TableDumper&);
    };
    
    const ctstring LoadPString(unsigned offset,
                               unsigned& bytes,
                               const std::wstring& what)
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
                                    const std::wstring& what
                                   )
{
    unsigned segment = offset & 0xFF0000;
    vector<ctstring> result;
    result.reserve(count);
    const std::wstring what_p = what + L" pointers";
    const std::wstring what_d = what + L" data";
    
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
        MarkFree(freebytepos, freebytecount, L"extra space");
#endif
    }
#if LOADP_DEBUG
    logger.Finish(offset);
#endif
    return result;
}

const ctstring LoadZString(unsigned beginoffs,
                           unsigned &bytes,
                           const std::wstring& what,
                           const extrasizemap_t& extrasizes)
{
    ctstring foundstring;

    unsigned endoffs;
    for(unsigned p=beginoffs; ; ++p)
    {
        endoffs=p+1;
        
        if(ROM[p] == 0) break;
        
        unsigned byte = ROM[p];
        
        /* FIXME: Invent a better way to see this */
        if(extrasizes.size() == 1)
        {
            if(byte == 1 || byte == 2)
            {
                byte = byte*256 + ROM[++p];
            }
        }
        
        foundstring += (ctchar)byte;
        
        extrasizemap_t::const_iterator i = extrasizes.find( (unsigned short) byte);
        if(i != extrasizes.end())
        {
            unsigned extra = i->second;
            while(extra-- > 0)
                foundstring += (char)ROM[++p];
        }
    }
    
    bytes = endoffs - beginoffs;
    
#if 0
    printf("\n;Loaded %u bytes (%u characters) from $%06X\n",
        bytes, foundstring.size(), beginoffs);
#endif
    
    MarkFree(beginoffs, bytes, what);
    
    return foundstring;
}

const vector<ctstring> LoadZStrings(unsigned offset, unsigned count,
                                    const std::wstring& what,
                                    const extrasizemap_t& extrasizes)
{
    const std::wstring what_p = what + L" pointers";
    const std::wstring what_d = what + L" data";
    
    bool relocated = false;
    unsigned real_offset = offset;

    if(ROM[offset] == 255
    && ROM[offset+1] == 255)
    {
        /* relocated string table! */
        
        unsigned new_offset = ROM[offset+2] | (ROM[offset+3] << 8) | (ROM[offset+4] << 16);
        new_offset = SNES2ROMaddr(new_offset);
        
        FILE*log = GetLogFile("mem", "log_addrs");
        if(log)
            fprintf(log, "-- Table %06X seems to be relocated to %06X\n",
                offset, new_offset);
        
        relocated = true;
        
        MarkProt(offset, 5, what_p);
        
        offset = new_offset;
    }

    const unsigned segment = offset >> 16;
    const unsigned base = segment << 16;

    vector<ctstring> result;
    result.reserve(count);
    set<unsigned> offsetlist;
    
#if LOADZ_DEBUG
    TableDumper logger(what_p, offset);
#endif
    unsigned first_offs = offset;
    unsigned last_offs  = first_offs;
    for(unsigned a=0; !count || a<count; ++a, offset += 2, real_offset += 2)
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
        logger.AddPtr(a, real_offset, stringptr, bytes);
#endif
        
#if LOADZ_EXTRASPACE
        unsigned freebytepos=stringptr + base + bytes;
        unsigned freebytecount=0;
        for(unsigned freebyte=freebytepos;
            ROM[freebyte] == 0x00 //zero
         || ROM[freebyte] == 0xFF //space
         || ROM[freebyte] == 0xEF //also space
         ; ++freebyte,++freebytecount);
        MarkFree(freebytepos, freebytecount, L"extra space");
#endif
        result.push_back(foundstring);
    }

    if(relocated)
    {
        MarkFree(first_offs, last_offs-first_offs, what_p);
    }
    else
    {
        MarkProt(first_offs, last_offs-first_offs, what_p);
    }

#if LOADZ_DEBUG
    logger.Finish(offset);
#endif
    return result;
}

const vector<ctstring> LoadFStrings(unsigned offset, unsigned len,
                                    const std::wstring& what,
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
