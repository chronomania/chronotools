#include <set>
#include <map>
#include <vector>

#include "rommap.hh"
#include "msgdump.hh"
#include "scriptfile.hh"
#include "logfiles.hh"
#include "rangemap.hh"

using namespace std;

namespace
{
    const bool TryFindExtraSpace = false;
    const unsigned ExtraSpaceMinLen = 128;
    
    vector<unsigned char> space, protect;
    
    typedef rangemap<unsigned, string> reasonmap;
    reasonmap reasons;
}

unsigned char *ROM;

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

namespace
{
    void MarkingMessage(const char* msgtype, const set<unsigned>& errlist, const char* type)
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
                fprintf(stderr, "%s: %06X-%06X already marked %s\n",
                                msgtype, first,prev, type);
                
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
    void MarkingError(const set<unsigned>& errlist, const char *type)
    {
        MarkingMessage("Error", errlist, type);
    }
    void MarkingWarning(const set<unsigned>& errlist, const char *type)
    {
        MarkingMessage("Warning", errlist, type);
    }
    
    void SetReasons(unsigned begin, unsigned length, const string& what)
    {
        reasons.set(begin, begin+length, what);
    }
    
    void ExplainProtMap(FILE *log,
                        unsigned begin, unsigned length,
                        const char* type,
                        unsigned num_ref)
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
            
            if(num_ref > 1)
                fprintf(log, " %3u", num_ref);
            else
                fprintf(log, "    ");
            
            fprintf(log, "%s\n", i->second.c_str());
            fflush(log);
        }
    }
}

void ShowProtMap()
{
    static const char *const types[4] = {"?","free","protected","ERROR"};
    
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
        "   begin-end     size/bytes type       use\n");
    unsigned romsize = space.size();
    
    bool begun=false;
    unsigned first=0;
    unsigned lasttype=0;
    unsigned lastcount=0;
    
    for(unsigned a=0; a<=romsize; ++a)
    {
        unsigned type = 0, count = 0;
        
        if(a < romsize)
        {
            type = (space[a] ? 1 : 0)
                 | (protect[a] ? 2 : 0);
            count = /*space[a] +*/ protect[a];
        }
        
        if(a == romsize
        || (a > 0 && !(a&0xFFFF))
        || (begun && (lasttype != type || lastcount != count))
          )
        {
            ExplainProtMap(log,
                           first, a-first,
                           types[lasttype],
                           lastcount);
            
            begun = false;
            if(a == romsize) break;
        }
        if(!begun) { begun = true; first=a; }
        lasttype  = type;
        lastcount = count;
    }
    
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

void MarkFree(unsigned begin, unsigned length, const string& reason)
{
    set<unsigned> error;
    //set<unsigned> warning;
    //fprintf(stderr, "Marking %u bytes free at %06X\n", length, begin);
    for(unsigned n=0; n<length; ++n)
    {
        if(protect[begin+n]) error.insert(begin+n);
        //if(space[begin+n]) warning.insert(begin+n);
        ++space[begin+n];
    }
    
    // refreeing is not dangerous. It's common when substrings are reused.
    
    if(!error.empty()) MarkingError(error, "protected, attempted to free");
    //if(!warning.empty()) MarkingWarning(warning, "free, attempted to refree");
    
    if(length > 0) SetReasons(begin, length, reason);
}

void MarkProt(unsigned begin, unsigned length, const string& reason)
{
    set<unsigned> error;
    set<unsigned> warning;
    
    //fprintf(stderr, "Marking %u bytes protected at %06X\n", length, begin);
    for(unsigned n=0; n<length; ++n)
    {
        if(space[begin+n]) error.insert(begin+n);
        if(protect[begin+n]) warning.insert(begin+n);
        ++protect[begin+n];
    }

    if(!error.empty()) MarkingError(error, "free, attempted to protect");
    if(!warning.empty()) MarkingWarning(warning, "protected, attempted to reprotect");
    
    if(length > 0) SetReasons(begin, length, reason);
}

void UnProt(unsigned begin, unsigned length)
{
    for(; length>0; --length)
        protect[begin++] = 0;
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
    fprintf(stderr, " %u bytes...", romsize);
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
    space.resize(romsize, 0);
    protect.clear();
    protect.resize(romsize, 0);
    reasons.clear();
    
    fprintf(stderr, "\n");
}

void FindEndSpaces(void)
{
    if(!TryFindExtraSpace) return;
    
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
                        MarkFree(blapos-(ExtraSpaceMinLen-1), ExtraSpaceMinLen, "end space");
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

void ListSpaces(void)
{
    //fprintf(stderr, "Dumping free space list...");
    unsigned pagecount = (space.size()+0xFFFF) >> 16;
    StartBlock("", "free space");
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
                    char Buf[64];
                    sprintf(Buf, "*s%02X\n", page);
                    PutAscii(Buf);
                    freehere = true;
                }
                char Buf[64];
                sprintf(Buf, "$%04X:%04X ; %u\n", freebegin, p, p-freebegin);
                
                PutAscii(Buf);
            }
        }
    }
    EndBlock();
    //fprintf(stderr, " done\n");
}
