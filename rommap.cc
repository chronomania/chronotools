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

using namespace std;

namespace
{
    typedef rangeset<unsigned> rommap;
    typedef rangemap<unsigned, string> reasonmap;

    rommap space, protect;
    reasonmap reasons;
    
    unsigned known_romsize = 0;
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
                        const set<unsigned>& errlist,
                        const string& why,
                        const char* type)
    {
        set<unsigned>::const_iterator i;
        
        bool begun=false;
        unsigned first=0;
        unsigned prev=0;
        
        reasons.find(5);
        
        for(i=errlist.begin(); ; ++i)
        {
            if(i == errlist.end() || (begun && prev < *i-1))
            {
                fprintf(stderr, "%s: %06X-%06X already marked %s by %s\n",
                                msgtype, first,prev, type, why.c_str());
                
                set<string> users;
                
                list<reasonmap::const_iterator> userlist;
                reasons.find_all_coinciding(first, prev+1, userlist);
                for(list<reasonmap::const_iterator>::const_iterator
                    j = userlist.begin();
                    j != userlist.end();
                    ++j)
                {
                    users.insert((*j)->second);
                }
                if(!users.empty())
                {
                    fprintf(stderr, "- previously defined by:");
                    int c=0;
                    for(set<string>::const_iterator j = users.begin(); j != users.end(); ++j)
                    {
                        if(c++)fprintf(stderr, ",");
                        fprintf(stderr, " %s", j->c_str());
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
    void MarkingError(const set<unsigned>& errlist, const string& why, const char *type)
    {
        MarkingMessage("Error", errlist, why, type);
    }
    void MarkingWarning(const set<unsigned>& errlist, const string& why, const char *type)
    {
        MarkingMessage("Warning", errlist, why, type);
    }
    
    void SetReasons(unsigned begin, unsigned length, const string& what)
    {
        reasons.set(begin, begin+length, what);
    }
    
    void ExplainProtMap(FILE *log,
                        unsigned begin, unsigned length,
                        const char* type)
    {
        unsigned endpos = begin+length;
        
        reasonmap contents;
        
        contents.set(begin, endpos, "");
        
        list<reasonmap::const_iterator> userlist;
        reasons.find_all_coinciding(begin, endpos, userlist);
        for(list<reasonmap::const_iterator>::const_iterator
            i = userlist.begin();
            i != userlist.end();
            ++i)
        {
            contents.set((*i)->first.lower,
                         (*i)->first.upper,
                         (*i)->second);
        }
        contents.compact();
        
        for(reasonmap::const_iterator
            i = contents.begin();
            i != contents.end();
            ++i)
        {
            fprintf(log, "  %06X-%06X: %10u %-10s",
                i->first.lower,
                i->first.upper-1,
                i->first.upper - i->first.lower,
                type);
            
            fprintf(log, "%s\n", i->second.c_str());
            fflush(log);
        }
    }
    
    unsigned GetRomSize()
    {
        unsigned romsize = known_romsize;
        if(!space.empty())
        {
            rommap::iterator tmp = space.end(); --tmp;
            if(tmp->upper > romsize) romsize = tmp->upper;
        }
        if(!protect.empty())
        {
            rommap::iterator tmp = protect.end(); --tmp;
            if(tmp->upper > romsize) romsize = tmp->upper;
        }
        return romsize;
    }
    
    void Do_ShowProtMap(FILE *log,
                       const char *const *prot_types)
    {
        space.compact();
        protect.compact();
        
        bool begun=false;
        
        unsigned romsize = GetRomSize();
        
        vector<unsigned char> types(romsize);
        
        for(rommap::const_iterator i = space.begin(); i != space.end(); ++i)
            for(unsigned a = i->lower; a < i->upper; ++a)
                types[a] |= 1;
        for(rommap::const_iterator i = protect.begin(); i != protect.end(); ++i)
            for(unsigned a = i->lower; a < i->upper; ++a)
                types[a] |= 2;
        
        for(unsigned first=0,lasttype=0, a=0; a<=romsize; ++a)
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

void MarkFree(unsigned begin, unsigned length, const string& reason)
{
    set<unsigned> error /* , warning */;
    //fprintf(stderr, "Marking %u bytes free at %06X\n", length, begin);
    
    for(unsigned n=0; n<length; ++n)
    {
        if(protect.find(begin+n) != protect.end())
            error.insert(begin+n);
        /*
        if(space.find(begin+n) != space.end())
            warning.insert(begin+n);
        */
    }
    space.set(begin, begin+length);
    
    // refreeing is not dangerous. It's common when substrings are reused.
    
    if(!error.empty()) MarkingError(error, reason, "protected, attempted to free");
    //if(!warning.empty()) MarkingWarning(warning, reason, "free, attempted to refree");
    
    if(length > 0) SetReasons(begin, length, reason);
    
    space.compact();
}

void MarkProt(unsigned begin, unsigned length, const string& reason)
{
    set<unsigned> error, warning;
    
    //fprintf(stderr, "Marking %u bytes protected at %06X\n", length, begin);
    for(unsigned n=0; n<length; ++n)
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
    
    protect.compact();
}

void UnProt(unsigned begin, unsigned length)
{
    protect.erase(begin, begin+length);
    
    protect.compact();
}

void FindEndSpaces(void)
{
    /* Not used */
}

void ListSpaces(void)
{
    map<unsigned, rommap> freemap;
    space.compact();
    for(rommap::const_iterator i = space.begin(); i != space.end(); ++i)
    {
        unsigned begin = i->lower;
        unsigned end   = i->upper;
        
        while(begin < end)
        {
            unsigned page = begin >> 16;
            unsigned max  = (page+1) << 16;
            if(max > end) max = end;
            freemap[page].set(begin & 0xFFFF, max & 0xFFFF);
            begin = max+1;
        }
    }

    //fprintf(stderr, "Dumping free space list...");
    StartBlock("", "free space");
    
    for(map<unsigned, rommap>::const_iterator
        i = freemap.begin(); i != freemap.end(); ++i)
    {
        char Buf[64];
        sprintf(Buf, "*s%02X\n", i->first);
        PutAscii(Buf);
         
        for(rommap::const_iterator
            j = i->second.begin(); j != i->second.end(); ++j)
        {
            sprintf(Buf, "$%04X:%04X ; %u\n",
                j->lower,
                j->upper,
                j->upper - j->lower);
            PutAscii(Buf);
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
    unsigned romsize = ftell(fp)-hdrskip;
    fprintf(stderr, " $%X bytes...", romsize);
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


unsigned char ROM2SNESpage(unsigned char page)
{
    if(page < 0x40) return page | 0xC0;
    
    /* Pages 00..3F have only their high part mirrored */
    /* Pages 7E..7F can not be used (they're RAM) */
    
    if(page >= 0x7E) return page & 0x3F;
    return (page - 0x40) + 0x40;
}

unsigned char SNES2ROMpage(unsigned char page)
{
    if(page >= 0x80) return page & 0x3F;
    return (page & 0x3F) + 0x40;
}

unsigned long ROM2SNESaddr(unsigned long addr)
{
    return (addr & 0xFFFF) | (ROM2SNESpage(addr >> 16) << 16);
}

unsigned long SNES2ROMaddr(unsigned long addr)
{
    return (addr & 0xFFFF) | (SNES2ROMpage(addr >> 16) << 16);
}

unsigned long GetROMsize()
{
    return 0x100000 /* 1 Megabyte */
         * (ROMinfo.romsize / 8);
}

bool IsSNESbased(unsigned long addr)
{
    return addr >= 0xC00000;
}
