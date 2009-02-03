#include <set>
#include <map>
#include <vector>

#include "rommap.hh"
#include "msgdump.hh"
#include "scriptfile.hh"
#include "logfiles.hh"

#include "rangemap.hh"
#include "rangeset.hh"

#include "config.hh"

namespace
{
    typedef rangeset<size_t> rommap;
    typedef rangemap<size_t, std::wstring> reasonmap;

    rommap space, protect;
    reasonmap reasons;
    
    size_t known_romsize = 0;
}

unsigned char *ROM;

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

namespace
{
    void MarkingMessage(const char* msgtype,
                        const std::set<size_t>& errlist,
                        const std::wstring& why,
                        const char* type)
    {
        std::set<size_t>::const_iterator i;
        
        bool begun=false;
        size_t first=0;
        size_t prev=0;
        
        for(i=errlist.begin(); ; ++i)
        {
            if(i == errlist.end() || (begun && prev < *i-1))
            {
                fprintf(stderr, "%s: %06X-%06X already marked %s by %ls\n",
                                msgtype, (unsigned)first, (unsigned)prev, type, why.c_str());
                
                std::set<std::wstring> users;
                
                /* Find the first element that begins after the given address */

                reasonmap::const_iterator j = reasons.lower_bound(first);
                if(j->lower > first)
                {
                    if(j != reasons.begin())
                    {
                        reasonmap::const_iterator prev = j; --prev;
                        if(prev->upper > first) --j;
                    }
                }
                while(j != reasons.end() && j->lower <= prev)
                {
                    users.insert(j->value);
                    ++j;
                }
                if(!users.empty())
                {
                    fprintf(stderr, "- previously defined by:");
                    int c=0;
                    for(std::set<std::wstring>::const_iterator j = users.begin(); j != users.end(); ++j)
                    {
                        if(c++)fprintf(stderr, ",");
                        fprintf(stderr, " %s", WstrToAsc(*j).c_str());
                    }
                    fprintf(stderr, "\n");
                }
                
                begun = false;
                if(i == errlist.end()) break;
            }
            
            if(!begun) { begun = true; first = *i; }
            prev = *i;
        }
    }
    void MarkingError(const std::set<size_t>& errlist,
                      const std::wstring& why, const char *type)
    {
        MarkingMessage("Error", errlist, why, type);
    }
    void MarkingWarning(const std::set<size_t>& errlist,
                        const std::wstring& why, const char *type)
    {
        MarkingMessage("Warning", errlist, why, type);
    }
    
    void SetReasons(size_t begin, size_t length, const std::wstring& what)
    {
        reasons.set(begin, begin+length, what);
    }
    
    void ExplainProtMap(FILE *log,
                        size_t begin, size_t length,
                        const char* type)
    {
        size_t endpos = begin+length;
        
        reasonmap contents;
        
        contents.set(begin, endpos, L"");
        
        reasonmap::const_iterator j = reasons.lower_bound(begin);
        if(j->lower > begin)
        {
            if(j != reasons.begin())
            {
                reasonmap::const_iterator prev = j; --prev;
                if(prev->upper > begin) --j;
            }
        }
        while(j != reasons.end() && j->lower <= endpos)
        {
            contents.set(std::max(j->lower, begin),
                         std::min(j->upper, endpos),
                         j->value);
            ++j;
        }

        for(reasonmap::const_iterator
            i = contents.begin();
            i != contents.end();
            ++i)
        {
            fprintf(log, "  %06X-%06X: %10u %-10s",
                (unsigned) (i->lower),
                (unsigned) (i->upper - 1),
                (unsigned) (i->upper - i->lower),
                type);
            
            fprintf(log, "%s\n", WstrToAsc(i->value).c_str());
            fflush(log);
        }
    }
    
    size_t GetRomSize()
    {
        size_t romsize = known_romsize;
        if(!space.empty())
        {
            rommap::const_iterator tmp = space.end(); --tmp;
            if(tmp->upper > romsize) romsize = tmp->upper;
        }
        if(!protect.empty())
        {
            rommap::const_iterator tmp = protect.end(); --tmp;
            if(tmp->upper > romsize) romsize = tmp->upper;
        }
        return romsize;
    }
    
    void Do_ShowProtMap(FILE *log,
                        const char *const *prot_types)
    {
        bool begun=false;
        
        size_t romsize = GetRomSize();
        
        std::vector<unsigned char> types(romsize);
        
        for(rommap::const_iterator i = space.begin(); i != space.end(); ++i)
            for(size_t a = i->lower; a < i->upper; ++a)
                types[a] |= 1;
        for(rommap::const_iterator i = protect.begin(); i != protect.end(); ++i)
            for(size_t a = i->lower; a < i->upper; ++a)
                types[a] |= 2;
        
        size_t first=0;
        unsigned lasttype=0;
        for(size_t a=0; a<=romsize; ++a)
        {
            unsigned type = types[a];

            if(a == romsize
            || (a > 0 && !(a&0xFFFF))
            || (begun && lasttype != type)
              )
            {
                ExplainProtMap(log, first, a-first, prot_types[lasttype]);

                begun = false;
                if(a == romsize) break;
            }
            if(!begun) { begun = true; first=a; }
            lasttype  = type;
        }
    }
}

void ShowProtMap()
{
    FILE *log = GetLogFile("dumper", "log_map");
    if(!log)
    {
        return;
    }
    
    fprintf(log,
        "This file lists the memory map of your\n"
        "Chrono Trigger ROM, as detected by ctdump.\n"
        "\n"
        "Prot/free map:\n"
        "   begin-end     size/bytes type     use\n");

    static const char *const types[4] = {"?","free","protected","ERROR"};
    
    Do_ShowProtMap(log, types);
    
    fprintf(log,
        "\n"
        "%-10s: will be used by insertor\n"
        "%-10s: will never be marked \"free\"\n"
        "%-10s: is code/data/undetermined - not free.\n",
        types[1],
        types[2],
        types[0]
           );
}

void ShowProtMap2()
{
    FILE *log = GetLogFile("mem", "log_addrs");
    if(!log)
    {
        return;
    }
    
    fprintf(log,
        "This file lists the memory map of your\n"
        "Chrono Trigger ROM, as created by ctinsert.\n"
        "\n"
        "Prot/free map:\n"
        "   begin-end     size/bytes type     use\n");

    static const char *const types[4] = {"protected","free","patched","ERROR"};
    
    Do_ShowProtMap(log, types);
    
    fprintf(log,
        "\n"
        "%-10s: was unused.\n"
        "%-10s: was used.\n"
        "%-10s: is not touched.\n",
        types[1],
        types[2],
        types[0]
           );
}

void MarkFree(size_t begin, size_t length, const std::wstring& reason)
{
    std::set<size_t> error;
    //fprintf(stderr, "Marking %u bytes free at %06X\n", length, begin);
    
    for(size_t n=0; n<length; ++n)
    {
        if(protect.find(begin+n) != protect.end())
            error.insert(begin+n);
    }
    space.set(begin, begin+length);
    
    // refreeing is not dangerous. It's common when subwstrings are reused.
    
    if(!error.empty()) MarkingError(error, reason, "protected, attempted to free");
    
    if(length > 0) SetReasons(begin, length, reason);
}

void MarkProt(size_t begin, size_t length, const std::wstring& reason)
{
    std::set<size_t> error, warning;
    
    //fprintf(stderr, "Marking %u bytes protected at %06X\n", length, begin);
    for(size_t n=0; n<length; ++n)
    {
        if(space.find(begin+n) != space.end())
            error.insert(begin+n);
        if(protect.find(begin+n) != protect.end())
            warning.insert(begin+n);
    }
    protect.set(begin, begin+length);

    if(!error.empty()) MarkingError(error, reason, "free, attempted to protect");
    if(!warning.empty()) MarkingWarning(warning, reason, "protected, attempted to reprotect");
    
    if(length > 0) SetReasons(begin, length, reason);
}

void UnProt(size_t begin, size_t length)
{
    protect.erase(begin, begin+length);
}

void FindEndSpaces(void)
{
    /* Not used */
}

void ListSpaces(void)
{
    std::map<size_t, rommap> freemap;

    /* Divide the "free" map to pages */
    for(rommap::const_iterator i = space.begin(); i != space.end(); ++i)
    {
        size_t begin = i->lower;
        size_t end   = i->upper;
        
        while(begin < end)
        {
            size_t page = begin & 0xFF0000;
            size_t max  = page + 0x10000;
            if(max > end) max = end;
            freemap[page >> 16].set(begin - page, max - page);
            begin = max+1;
        }
    }

    //fprintf(stderr, "Dumping free space list...");
    StartBlock(L"", L"free space");
    
    for(map<size_t, rommap>::const_iterator
        i = freemap.begin(); i != freemap.end(); ++i)
    {
        PutAscii(wformat(L"*s%02X\n", i->first));
         
        for(rommap::const_iterator
            j = i->second.begin(); j != i->second.end(); ++j)
        {
            PutAscii(wformat(L"$%04X:%04X ; %u\n",
                j->lower,
                j->upper,
                j->upper - j->lower));
        }
    }
    EndBlock();
    //fprintf(stderr, " done\n");
}

void LoadROM(FILE *fp)
{
    fprintf(stderr, "Loading ROM...");
    unsigned hdrskip = 0;

    char HdrBuf[512];
    fread(HdrBuf, 1, 512, fp);
    if(HdrBuf[1] == 2)
    {
        hdrskip = 512;
    }
    fseek(fp, 0, SEEK_END);
    size_t romsize = ftell(fp) - hdrskip;
    fprintf(stderr, " $%X bytes...", (unsigned)romsize);
    ROM = NULL;
#ifdef USE_MMAP
    /* This takes about 0.0001s on my computer over nfs */
    ROM = (unsigned char *)mmap(NULL, romsize, PROT_READ, MAP_PRIVATE, fileno(fp), hdrskip);
#endif
    /* mmap could have failed, so revert to reading */
    if(ROM == NULL || ROM == (unsigned char*)(-1))
    {
        /* This takes about 5s on my computer over nfs */
        ROM = new unsigned char [romsize];
        fseek(fp, hdrskip, SEEK_SET);
        fread(ROM, 1, romsize, fp);
    }
    else
        fprintf(stderr, " memmap succesful (%p),", (const void *)ROM);
    fprintf(stderr, " done");
    
    space.clear();
    protect.clear();
    reasons.clear();
    
    known_romsize = romsize;
    
    fprintf(stderr, "\n");
}

namespace
{
    struct ROMinfo
    {
        unsigned romsize;
        ROMinfo() : romsize(GetConf("general", "romsize"))
        {
            if(romsize != 32
            && romsize != 48
            && romsize != 64)
            {
                fprintf(stderr, "ROM size may be 32, 48 or 64 Mbits. Set to 32.\n");
                romsize = 32;
            }
        }
    } ROMinfo;
}

size_t GetROMsize()
{
    return 0x100000 /* 1 Megabyte */
         * (ROMinfo.romsize / 8);
}
